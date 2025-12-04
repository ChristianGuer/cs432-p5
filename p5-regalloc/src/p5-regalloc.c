/**
 * @file p5-regalloc.c
 * @brief Compiler phase 5: register allocation
 */
#include "p5-regalloc.h"


/**
 * @brief Replace a virtual register id with a physical register id
 *
 * Every virtual register operand with ID "vr" will be replaced by a physical
 * register operand with ID "pr" in the given instruction.
 *
 * @param vr Virtual register id that should be replaced
 * @param pr Physical register id that it should be replaced by
 * @param isnsn Instruction to modify
 */
void replace_register(int vr, int pr, ILOCInsn *insn)
{
    for (int i = 0; i < 3; i++)
    {
        if (insn->op[i].type == VIRTUAL_REG && insn->op[i].id == vr)
        {
            insn->op[i].type = PHYSICAL_REG;
            insn->op[i].id = pr;
        }
    }
}

/**
 * @brief Insert a store instruction to spill a register to the stack
 *
 * We need to allocate a new slot in the stack from for the current
 * function, so we need a reference to the local allocator instruction.
 * This instruction will always be the third instruction in a function
 * and will be of the form "add SP, -X => SP" where X is the current
 * stack frame size.
 *
 * @param pr Physical register id that should be spilled
 * @param prev_insn Reference to an instruction; the new instruction will be
 * inserted directly after this one
 * @param local_allocator Reference to the local frame allocator instruction
 * @returns BP-based offset where the register was spilled
 */
int insert_spill(int pr, ILOCInsn *prev_insn, ILOCInsn *local_allocator)
{
    /* adjust stack frame size to add new spill slot */

    int bp_offset = local_allocator->op[1].imm - WORD_SIZE;
    local_allocator->op[1].imm = bp_offset;

    /* create store instruction */
    ILOCInsn *new_insn = ILOCInsn_new_3op(STORE_AI,
                                          physical_register(pr), base_register(), int_const(bp_offset));

    /* insert into code */
    new_insn->next = prev_insn->next;
    prev_insn->next = new_insn;

    return bp_offset;
}

/**
 * @brief Insert a load instruction to load a spilled register
 *
 * @param bp_offset BP-based offset where the register value is spilled
 * @param pr Physical register where the value should be loaded
 * @param prev_insn Reference to an instruction; the new instruction will be
 * inserted directly after this one
 */
void insert_load(int bp_offset, int pr, ILOCInsn *prev_insn)
{
    /* create load instruction */
    ILOCInsn *new_insn = ILOCInsn_new_3op(LOAD_AI,
                                          base_register(), int_const(bp_offset), physical_register(pr));

    /* insert into code */
    new_insn->next = prev_insn->next;
    prev_insn->next = new_insn;
}

void spill(int pr, ILOCInsn *prev_insn, ILOCInsn *local_allocator, int *offset_arr, int *phys_reg_map)
{

    //if (offset_arr[phys_reg_map[pr]] != -1)
    int offset = insert_spill(pr, prev_insn, local_allocator);
    offset_arr[phys_reg_map[pr]] = offset;//-word_size?
    phys_reg_map[pr] = -1;
}

int dist(int vr, ILOCInsn *current_insn)
{
    ILOCInsn *search_insn = current_insn->next;
    int dist = 1;

    while (search_insn)
    {
        ILOCInsn *read_regs = ILOCInsn_get_read_registers(search_insn);
        for (int i = 0; i < 3; i++)
        {
            if (read_regs->op[i].type == VIRTUAL_REG && read_regs->op[i].id == vr)
            {
                ILOCInsn_free(read_regs);
                return dist;
            }
        }

        ILOCInsn_free(read_regs);
        search_insn = search_insn->next;
        dist++;
    }
    return MAX_VIRTUAL_REGS;
}

int allocate(int *phys_reg_map, int num_physical_registers, int vr, ILOCInsn *current_insn, ILOCInsn *local_allocator_insn, int *offset_arr, ILOCInsn *prev_insn)
{
    for (int i = 0; i < num_physical_registers; i++)
    {
        if (phys_reg_map[i] == -1)
        {
            phys_reg_map[i] = vr;
            return i;
        }
    }
    // spill case, for loops could be combined but makes code cleaner
    // find pr that maximizes dist(name[pr])
    int max_pr = -1;
    int max_pr_dist = -1;
    for (int i = 0; i < num_physical_registers; i++)
    {
        int vr = phys_reg_map[i];
        int current_dist = dist(vr, current_insn);
        if (current_dist > max_pr_dist)
        {
            max_pr = i;
            max_pr_dist = current_dist;
        }
    }
    spill(max_pr, prev_insn, local_allocator_insn, offset_arr, phys_reg_map);
    phys_reg_map[max_pr] = vr;
    return max_pr;
}

int ensure(int *phys_reg_map, int num_physical_registers, int vr, ILOCInsn *current_insn, ILOCInsn *local_allocator_insn, int *offset_arr, ILOCInsn *prev_insn)
{
    for (int i = 0; i < num_physical_registers; i++)
    {
        if (phys_reg_map[i] == vr)
        {
            return i;
        }
    }
    int pr = allocate(phys_reg_map, num_physical_registers, vr, current_insn, local_allocator_insn, offset_arr, prev_insn);
    
    if (offset_arr[vr] != -1)
    {
        insert_load(offset_arr[vr], pr, prev_insn);
    }
    return pr;
}

void allocate_registers(InsnList *list, int num_physical_registers)
{
    //BIggest issue is “I’m treating the wrong addI SP, imm => SP as the stack allocator inside fib.”
    if (num_physical_registers <= 0) {
        fprintf(stderr, "Error: no physical registers available for allocation\n");
    exit(1);
    }

    if (!list)
    {
        return;
    }

    // define and set physical registers to -1
    int phys_reg_map[num_physical_registers];
    for (int i = 0; i < num_physical_registers; i++)
    {
        phys_reg_map[i] = -1;
    }

    // offset array TODO determine if there is a better way to set size
    //Is this True?  No virtual registers are live across function boundaries. 
    //that would mean we have to reset offsets at each function and same for phys reg map
    int offset_arr[MAX_VIRTUAL_REGS];
    for (int i = 0; i < MAX_VIRTUAL_REGS; i++)
    {
        // set as invalid
        offset_arr[i] = -1;
    }

    // save previous instruction
    ILOCInsn *local_allocator_insn = NULL;
    ILOCInsn *last_processed_insn = NULL;

    FOR_EACH(ILOCInsn *, insn, list)
    {
        // save reference to stack allocator instruction if i is a call label
        //Save local allocator when the first instruction after a PUSH is an I2I and the next is an ADD_I
        if(insn->form == PUSH && insn->next != NULL && insn->next->form == I2I && insn->next->next != NULL && insn->next->next->form == ADD_I) {
            local_allocator_insn = insn->next->next;
        }
        ILOCInsn *read_regs = ILOCInsn_get_read_registers(insn);
        // for each read vr in insn:
        for (int i = 0; i < 3; i++)
        {
            if (read_regs->op[i].type == VIRTUAL_REG)
            {
                int vr = read_regs->op[i].id;

                // make sure vr is in a phys reg
                int pr = ensure(phys_reg_map, num_physical_registers, vr, insn, local_allocator_insn, offset_arr, last_processed_insn);
                replace_register(vr, pr, insn); // change register id

                // if dist(vr) == INFINITY:            // if no future use
                //         name[pr] = INVALID              // then free pr
                // Using max virtual regs for infinity
                if (dist(vr, insn) == MAX_VIRTUAL_REGS)
                {
                    phys_reg_map[pr] = -1;
                }
            }
        }
        ILOCInsn_free(read_regs);

        Operand write_reg = ILOCInsn_get_write_register(insn);
        if (write_reg.type == VIRTUAL_REG)
        {
            int vr = write_reg.id;

            // TODO: pr = allocate(vr)                   // make sure phys reg is available
            int pr = allocate(phys_reg_map, num_physical_registers, vr, insn, local_allocator_insn, offset_arr, last_processed_insn);
            replace_register(vr, pr, insn); // change register id and type
        }

        // spill any live registers before procedure calls
        if (insn->form == CALL)
        {
            if (last_processed_insn != NULL)
            {
                for (int i = 0; i < num_physical_registers; i++)
                {
                    if (phys_reg_map[i] != -1)
                    {
                        spill(i, last_processed_insn, local_allocator_insn, offset_arr, phys_reg_map);
                    }
                }
            }
        }

        last_processed_insn = insn;
    }
}
