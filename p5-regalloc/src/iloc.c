#include "iloc.h"

/*
 * ILOC operands
 */

Operand empty_operand (void)
{
    Operand op = { .type = EMPTY, .imm = 0 };
    return op;
}

Operand stack_register (void)
{
    Operand op = { .type = STACK_REG, .imm = 0 };
    return op;
}

Operand base_register(void)
{
    Operand op = { .type = BASE_REG, .imm = 0 };
    return op;
}

Operand return_register (void)
{
    Operand op = { .type = RETURN_REG, .imm = 0 };
    return op;
}

Operand virtual_register (void)
{
    static int next_id = 0;
    Operand op = { .type = VIRTUAL_REG, .id = next_id++ };
    return op;
}

Operand physical_register (int id)
{
    Operand op = { .type = PHYSICAL_REG, .id = id };
    return op;
}

Operand anonymous_label (void)
{
    static int next_id = 0;
    Operand op = { .type = JUMP_LABEL, .id = next_id++ };
    return op;
}

Operand call_label (const char* label)
{
    Operand op;
    op.type = CALL_LABEL;
    snprintf(op.str, MAX_LINE_LEN, "%s", label);
    return op;
}

Operand int_const (long integer)
{
    Operand op = { .type = INT_CONST, .imm = integer };
    return op;
}

Operand str_const (const char* string)
{
    Operand op;
    op.type = STR_CONST;
    snprintf(op.str, MAX_LINE_LEN, "%s", string);
    return op;
}

void Operand_print (Operand op, FILE* output)
{
    switch (op.type) {
        case EMPTY:       fprintf(output, "EMPTY");       break;
        case STACK_REG:   fprintf(output, "SP");          break;
        case BASE_REG:    fprintf(output, "BP");          break;
        case RETURN_REG:  fprintf(output, "RET");         break;
        case VIRTUAL_REG: fprintf(output, "r%d", op.id);  break;
        case PHYSICAL_REG:fprintf(output, "R%d", op.id);  break;
        case JUMP_LABEL:  fprintf(output, "l%d", op.id);  break;
        case CALL_LABEL:  fprintf(output, "%s", op.str);  break;
        case INT_CONST:   fprintf(output, "%ld", op.imm); break;
        case STR_CONST:
            fprintf(output, "\\\"");
            print_doubly_escaped_string(op.str, output);
            fprintf(output, "\\\"");
            break;
    }
}


/*
 * ILOC instructions (and lists of instructions)
 */

ILOCInsn* ILOCInsn_new_3op (InsnForm form, Operand op1, Operand op2, Operand op3)
{
    ILOCInsn* insn = (ILOCInsn*)calloc(1, sizeof(ILOCInsn));
    CHECK_MALLOC_PTR(insn)
    insn->form = form;
    insn->op[0] = op1;
    insn->op[1] = op2;
    insn->op[2] = op3;
    insn->next = NULL;          /* not strictly necessary b/c of the calloc */
    insn->comment[0] = '\0';    /* not strictly necessary b/c of the calloc */
    return insn;
}

ILOCInsn* ILOCInsn_new_2op (InsnForm form, Operand op1, Operand op2)
{
    return ILOCInsn_new_3op(form, op1, op2, empty_operand());
}

ILOCInsn* ILOCInsn_new_1op (InsnForm form, Operand op1)
{
    return ILOCInsn_new_3op(form, op1, empty_operand(), empty_operand());
}

ILOCInsn* ILOCInsn_new_0op (InsnForm form)
{
    return ILOCInsn_new_3op(form, empty_operand(), empty_operand(), empty_operand());
}

void ILOCInsn_set_comment (ILOCInsn* insn, const char* comment)
{
    snprintf(insn->comment, MAX_LINE_LEN, "%s", comment);
}

ILOCInsn* ILOCInsn_copy (ILOCInsn* insn)
{
    ILOCInsn* new_insn = (ILOCInsn*)calloc(1, sizeof(ILOCInsn));
    CHECK_MALLOC_PTR(new_insn)
    new_insn->form = insn->form;
    new_insn->op[0] = insn->op[0];
    new_insn->op[1] = insn->op[1];
    new_insn->op[2] = insn->op[2];
    new_insn->next = NULL;
    return new_insn;
}

#define PRINT(S) fprintf(output, S)
#define PRINTOP(I) Operand_print(insn->op[I], output)
#define PRINTPLUS(I) fprintf(output, "%s", (insn->op[I].imm >= 0 ? "+" : ""))

void ILOCInsn_print (ILOCInsn* insn, FILE* output)
{
    switch (insn->form) {

        /* arithmetic */
        case ADD:       PRINT("add ");   PRINTOP(0); PRINT(", ");   PRINTOP(1); PRINT(" => "); PRINTOP(2); break;
        case SUB:       PRINT("sub ");   PRINTOP(0); PRINT(", ");   PRINTOP(1); PRINT(" => "); PRINTOP(2); break;
        case MULT:      PRINT("mult ");  PRINTOP(0); PRINT(", ");   PRINTOP(1); PRINT(" => "); PRINTOP(2); break;
        case DIV:       PRINT("div ");   PRINTOP(0); PRINT(", ");   PRINTOP(1); PRINT(" => "); PRINTOP(2); break;
        case ADD_I:     PRINT("addI ");  PRINTOP(0); PRINT(", ");   PRINTOP(1); PRINT(" => "); PRINTOP(2); break;
        case MULT_I:    PRINT("multI "); PRINTOP(0); PRINT(", ");   PRINTOP(1); PRINT(" => "); PRINTOP(2); break;
        case AND:       PRINT("and ");   PRINTOP(0); PRINT(", ");   PRINTOP(1); PRINT(" => "); PRINTOP(2); break;
        case OR:        PRINT("or ");    PRINTOP(0); PRINT(", ");   PRINTOP(1); PRINT(" => "); PRINTOP(2); break;
        case NOT:       PRINT("not ");   PRINTOP(0); PRINT(" => "); PRINTOP(1);                            break;
        case NEG:       PRINT("neg ");   PRINTOP(0); PRINT(" => "); PRINTOP(1);                            break;

        /* data movement */
        case LOAD_I:    PRINT("loadI ");   PRINTOP(0); PRINT(" => ");  PRINTOP(1);                                         break;
        case LOAD:      PRINT("load [");   PRINTOP(0); PRINT("] => "); PRINTOP(1);                                         break;
        case LOAD_AI:   PRINT("loadAI ["); PRINTOP(0); PRINTPLUS(1);   PRINTOP(1); PRINT("] => "); PRINTOP(2);             break;
        case LOAD_AO:   PRINT("loadAO ["); PRINTOP(0); PRINTPLUS(1);   PRINTOP(1); PRINT("] => "); PRINTOP(2);             break;
        case STORE:     PRINT("store ");   PRINTOP(0); PRINT(" => ["); PRINTOP(1); PRINT("]");                             break;
        case STORE_AI:  PRINT("storeAI "); PRINTOP(0); PRINT(" => ["); PRINTOP(1); PRINTPLUS(2);   PRINTOP(2); PRINT("]"); break;
        case STORE_AO:  PRINT("storeAO "); PRINTOP(0); PRINT(" => ["); PRINTOP(1); PRINTPLUS(2);   PRINTOP(2); PRINT("]"); break;
        case I2I:       PRINT("i2i ");     PRINTOP(0); PRINT(" => ");  PRINTOP(1);                                         break;
        case PUSH:      PRINT("push ");    PRINTOP(0);                                                                     break;
        case POP:       PRINT("pop ");     PRINTOP(0);                                                                     break;

        /* control flow */
        case LABEL:     PRINTOP(0);      PRINT(":");                                                       break;
        case JUMP:      PRINT("jump ");  PRINTOP(0);                                                       break;
        case CALL:      PRINT("call ");  PRINTOP(0);                                                       break;
        case RETURN:    PRINT("return");                                                                   break;
        case CBR:       PRINT("cbr ");   PRINTOP(0); PRINT(" => "); PRINTOP(1); PRINT(", ");   PRINTOP(2); break;
        case PHI:       PRINT("phi ");   PRINTOP(0); PRINT(", ");   PRINTOP(1); PRINT(" => "); PRINTOP(2); break;

        /* comparison */
        case CMP_LT:    PRINT("cmp_LT "); PRINTOP(0); PRINT(", "); PRINTOP(1); PRINT(" => "); PRINTOP(2); break;
        case CMP_LE:    PRINT("cmp_LE "); PRINTOP(0); PRINT(", "); PRINTOP(1); PRINT(" => "); PRINTOP(2); break;
        case CMP_EQ:    PRINT("cmp_EQ "); PRINTOP(0); PRINT(", "); PRINTOP(1); PRINT(" => "); PRINTOP(2); break;
        case CMP_GE:    PRINT("cmp_GE "); PRINTOP(0); PRINT(", "); PRINTOP(1); PRINT(" => "); PRINTOP(2); break;
        case CMP_GT:    PRINT("cmp_GT "); PRINTOP(0); PRINT(", "); PRINTOP(1); PRINT(" => "); PRINTOP(2); break;
        case CMP_NE:    PRINT("cmp_NE "); PRINTOP(0); PRINT(", "); PRINTOP(1); PRINT(" => "); PRINTOP(2); break;

        /* misc */
        case NOP:       PRINT("nop"); break;
        case PRINT:     PRINT("print "); PRINTOP(0); break;

        /* unknown */
        default:        PRINT("??? ");    PRINTOP(0); PRINT(", "); PRINTOP(1); PRINT(", ");   PRINTOP(2); break;
    }
}

int ILOCInsn_get_operand_count (ILOCInsn* insn)
{
    int count = 0;
    for (int i = 0; i < 3; i++) {
        if (insn->op[i].type != EMPTY) {
            count++;
        }
    }
    return count;
}

ILOCInsn* ILOCInsn_get_read_registers (ILOCInsn* insn)
{
    ILOCInsn* ret = ILOCInsn_new_0op(NOP);
    switch (insn->form)
    {
        case STORE_AO:
            ret->op[0] = insn->op[0];
            ret->op[1] = insn->op[1];
            ret->op[2] = insn->op[2];
            break;

        case ADD: case SUB: case MULT: case DIV: case AND: case OR:
        case CMP_LT: case CMP_LE: case CMP_EQ: case CMP_NE: case CMP_GE: case CMP_GT:
        case LOAD_AO: case STORE: case STORE_AI:
        case PHI:
            ret->op[0] = insn->op[0];
            ret->op[1] = insn->op[1];
            break;

        case ADD_I: case MULT_I:
        case LOAD: case LOAD_AI: case I2I:
        case CBR: case NOT: case NEG: case PUSH:
        case PRINT:
            if (insn->op[0].type == VIRTUAL_REG ||
                insn->op[0].type == PHYSICAL_REG ||
                insn->op[0].type == STACK_REG ||
                insn->op[0].type == BASE_REG ||
                insn->op[0].type == RETURN_REG)
            {
                ret->op[0] = insn->op[0];
            }

        default:
            break;
    }

    return ret;
}

Operand ILOCInsn_get_write_register (ILOCInsn* insn)
{
    switch (insn->form)
    {
        case ADD: case SUB: case MULT: case DIV: case AND: case OR:
        case CMP_LT: case CMP_LE: case CMP_EQ: case CMP_NE: case CMP_GE: case CMP_GT:
        case ADD_I: case MULT_I:
        case LOAD_AI: case LOAD_AO:
        case PHI:
            return insn->op[2];

        case LOAD: case LOAD_I:
        case NOT: case NEG:
        case I2I:
            return insn->op[1];

        case POP:
            return insn->op[0];

        default:
            return empty_operand();
    }
}

void ILOCInsn_free (ILOCInsn* insn)
{
    free(insn);
}

DEF_LIST_IMPL(Insn, ILOCInsn*, ILOCInsn_free)

void InsnList_print (InsnList* list, FILE* output)
{
    FOR_EACH(ILOCInsn*, i, list) {
        if (i->form != LABEL) {
            printf("  ");
        }
        ILOCInsn_print(i, output);
        if (i->comment[0] != '\0') {
            fprintf(output, "  ; %s", i->comment);
        }
        fprintf(output, "\n");
    }
}


/*
 * AST VISITOR: Symbol storage/memory allocation
 */

/**
 * @brief State/data for symbol allocation visitor
 */
typedef struct AllocateSymbolsData
{
    /**
     * @brief Total size of static/global variables
     */
    long static_size;

    /**
     * @brief Size of current function's local variables (for building a stack frame)
     */
    long local_size;

    /**
     * @brief Currently inside a function? (used to distinguish between global and local variables)
     */
    bool in_function;

} AllocateSymbolsData;

AllocateSymbolsData* AllocateSymbolsData_new (void)
{
    AllocateSymbolsData* data = (AllocateSymbolsData*)calloc(1, sizeof(AllocateSymbolsData));
    CHECK_MALLOC_PTR(data);
    data->static_size = 0;
    data->local_size = 0;
    data->in_function = false;
    return data;
}

#define DATA ((AllocateSymbolsData*)visitor->data)

void AllocateSymbolsVisitor_previsit_funcdecl (NodeVisitor* visitor, ASTNode* node)
{
    DATA->in_function = true;
    int param_offset = 0;
    FOR_EACH (Parameter*, p, node->funcdecl.parameters) {
        Symbol* sym = lookup_symbol(node, p->name);
        sym->location = STACK_PARAM;
        sym->offset = PARAM_BP_OFFSET + param_offset;
        param_offset += WORD_SIZE;
    }
}

void AllocateSymbolsVisitor_postvisit_vardecl (NodeVisitor* visitor, ASTNode* node)
{
    Symbol* sym = lookup_symbol(node, node->vardecl.name);
    if (DATA->in_function) {
        /* local/stack variable */
        sym->location = STACK_LOCAL;
        sym->offset = LOCAL_BP_OFFSET - DATA->local_size;
        DATA->local_size += WORD_SIZE;
    } else {
        /* global/static variable */
        long total_size = sym->length * WORD_SIZE;
        sym->location = STATIC_VAR;
        sym->offset = STATIC_VAR_OFFSET + DATA->static_size;
        DATA->static_size += total_size;
    }
}

void AllocateSymbolsVisitor_postvisit_funcdecl (NodeVisitor* visitor, ASTNode* node)
{
    ASTNode_set_printable_attribute(node, "localSize", (void*)(long)DATA->local_size,
            int_attr_print, dummy_free);
    DATA->in_function = false;
    DATA->local_size = 0;
}

void AllocateSymbolsVisitor_postvisit_program (NodeVisitor* visitor, ASTNode* node)
{
    ASTNode_set_printable_attribute(node, "staticSize", (void*)(long)DATA->static_size,
            int_attr_print, dummy_free);
}

NodeVisitor* AllocateSymbolsVisitor_new (void)
{
    NodeVisitor* v = NodeVisitor_new();
    AllocateSymbolsData* data = (AllocateSymbolsData*)calloc(1, sizeof(AllocateSymbolsData));
    CHECK_MALLOC_PTR(data);
    data->static_size = 0;
    data->local_size = 0;
    data->in_function = false;
    v->data = data;
    v->dtor = free;
    v->previsit_funcdecl  = AllocateSymbolsVisitor_previsit_funcdecl;
    v->postvisit_vardecl  = AllocateSymbolsVisitor_postvisit_vardecl;
    v->postvisit_funcdecl = AllocateSymbolsVisitor_postvisit_funcdecl;
    v->postvisit_program  = AllocateSymbolsVisitor_postvisit_program;
    return v;
}


/*
 * ASTNode extensions for code generation
 */

void reg_attr_print (void* reg, FILE* output)
{
    /* used for "reg" attributes */
    fprintf(output, "r%d", (int)(long)reg);
}

void insnlist_attr_print (InsnList* list, FILE* output)
{
    /* used for "code" attributes */
    FOR_EACH(ILOCInsn*, i, list) {
        fprintf(output, "\\n");
        ILOCInsn_print(i, output);
    }
}

void ASTNode_copy_code (ASTNode* dest, ASTNode* src)
{
    /* ensure there's a code attribute in the destination (create if absent) */
    if (!ASTNode_has_attribute(dest, "code")) {
        ASTNode_set_printable_attribute(dest, "code", InsnList_new(),
                (AttributeValueDOTPrinter)insnlist_attr_print, (Destructor)InsnList_free);
    }

    /* make sure there's actually something to copy */
    if (!ASTNode_has_attribute(src, "code")) {
        return;
    }

    /* copy each instruction */
    InsnList* src_list  = ASTNode_get_attribute(src,  "code");
    InsnList* dest_list = ASTNode_get_attribute(dest, "code");
    FOR_EACH(ILOCInsn*, i, src_list) {
        InsnList_add(dest_list, ILOCInsn_copy(i));
    }
}

void ASTNode_emit_insn (ASTNode* dest, ILOCInsn* insn)
{
    if (!ASTNode_has_attribute(dest, "code")) {
        ASTNode_set_printable_attribute(dest, "code", (void*)NodeList_new(),
                (AttributeValueDOTPrinter)insnlist_attr_print, (Destructor)InsnList_free);
    }
    InsnList* list = ASTNode_get_attribute(dest, "code");
    InsnList_add(list, insn);
}

void ASTNode_add_comment (ASTNode* dest, const char* comment)
{
    if (!ASTNode_has_attribute(dest, "code")) {
        return;
    }
    InsnList* list = ASTNode_get_attribute(dest, "code");
    if (InsnList_is_empty(list)) {
        return;
    }
    ILOCInsn* last_insn = list->tail;
    ILOCInsn_set_comment(last_insn, comment);
}

void ASTNode_set_temp_reg (ASTNode* node, Operand reg)
{
    ASTNode_set_printable_attribute(node, "reg", (void*)(long)reg.id, reg_attr_print, dummy_free);
}

Operand ASTNode_get_temp_reg (ASTNode* node)
{
    Operand op = { .type = VIRTUAL_REG, .id = -1 };
    if (!ASTNode_has_attribute(node, "reg")) {
        printf("ERROR: Node is missing a temporary register");
        return op;
    }
    op.id = (int)(long)ASTNode_get_attribute(node, "reg");
    return op;
}


/*
 * ILOC machine simulator
 */

#if WORD_SIZE == 4
    typedef int32_t word_t;
    #define PRIW "%" PRId32
#else
    typedef int64_t word_t;
    #define PRIW "%" PRId64
#endif
typedef uint8_t byte_t;

#define UNINIT_REG       (-9999999)

/**
 * @brief Information about call targets (i.e., functions)
 * 
 * This information is prefetched with a single pass over the instruction
 * before it is simulated.
 */
typedef struct CallTarget
{
    /**
     * @brief Function name
     */
    char name[MAX_TOKEN_LEN];

    /**
     * @brief Pointer to corresponding label "instruction"
     */
    ILOCInsn* insn;
    
    /**
     * @brief Next call target (if stored in a list)
     */
    struct CallTarget* next;

} CallTarget;

DECL_LIST_TYPE(CallTarget, CallTarget*)
DEF_LIST_IMPL(CallTarget, CallTarget*, free)

void CallTargetList_add_new (CallTargetList* list, const char* name, ILOCInsn* target)
{
    CallTarget* new_target = (CallTarget*)calloc(1, sizeof(CallTarget));
    CHECK_MALLOC_PTR(new_target);
    snprintf(new_target->name, MAX_TOKEN_LEN, "%s", name);
    new_target->insn = target;
    CallTargetList_add(list, new_target);
}

ILOCInsn* CallTargetList_find (CallTargetList* list, const char* name)
{
    FOR_EACH (CallTarget*, target, list) {
        if (token_str_eq(target->name, name)) {
            return target->insn;
        }
    }
    printf("ERROR: No call target found for '%s'\n", name);
    exit(EXIT_FAILURE);
}

/**
 * @brief ILOC machine state structure
 */
typedef struct ILOCMachine
{
    /**
     * @brief Virtual register values
     */
    word_t reg[MAX_VIRTUAL_REGS];

    /**
     * @brief Physical register values
     */
    word_t pr[MAX_PHYSICAL_REGS];

    /**
     * @brief Program counter (pointer to next instruction to execute)
     */
    ILOCInsn* pc;

    /**
     * @brief Stack pointer value
     */
    word_t sp;
    
    /**
     * @brief Base pointer value
     */
    word_t bp;

    /**
     * @brief Function return value
     */
    word_t ret;

    /**
     * @brief Program address space (memory w/ global variables and stack)
     */
    byte_t mem[MEM_SIZE];

    /**
     * @brief List of program instructions (i.e., code)
     * 
     * Note that instructions are NOT stored in the program's "address space."
     */
    ILOCInsn* instructions[MAX_INSTRUCTIONS];

    /**
     * @brief Jump targets (instrution pointers indexed by jump label IDs)
     */
    ILOCInsn* jump_targets[MAX_INSTRUCTIONS];

    /**
     * @brief Call targets (list of string label and instruction pointer pairs)
     */
    CallTargetList* call_targets;

} ILOCMachine;

ILOCMachine* ILOCMachine_new(void)
{
    ILOCMachine* machine = (ILOCMachine*)calloc(1, sizeof(ILOCMachine));
    CHECK_MALLOC_PTR(machine);

    /* set all registers to special "uninitialized" value (helps find code gen bugs) */
    for (int i = 0; i < MAX_VIRTUAL_REGS; i++) {
        machine->reg[i] = UNINIT_REG;
    }
    for (int i = 0; i < MAX_PHYSICAL_REGS; i++) {
        machine->pr[i] = UNINIT_REG;
    }
    machine->sp = machine->bp = machine->ret = UNINIT_REG;

    /* initialize call target list */
    machine->call_targets = CallTargetList_new();

    /* everything else can stay zero/NULL from the calloc */
    return machine;
}

void ILOCMachine_set_reg(ILOCMachine* machine, Operand op, word_t value)
{
    switch (op.type) {
        case STACK_REG:  machine->sp  = value; break;
        case BASE_REG:   machine->bp  = value; break;
        case RETURN_REG: machine->ret = value; break;
        case VIRTUAL_REG:
            if (op.id < 0 || op.id > MAX_VIRTUAL_REGS) {
                printf("ERROR: Register r%d does not exist\n", op.id);
                exit(EXIT_FAILURE);
            }
            machine->reg[op.id] = value;
            break;
        case PHYSICAL_REG:
            if (op.id < 0 || op.id > MAX_PHYSICAL_REGS) {
                printf("ERROR: Register R%d does not exist\n", op.id);
                exit(EXIT_FAILURE);
            }
            machine->pr[op.id] = value;
            break;
        default:
            printf("ERROR: Cannot write register using a non-register operand: ");
            Operand_print(op, stdout);
            printf("\n");
            break;
    }
}

word_t ILOCMachine_get_reg(ILOCMachine* machine, Operand op)
{
    switch (op.type) {
        case STACK_REG:  return machine->sp;
        case BASE_REG:   return machine->bp;
        case RETURN_REG: return machine->ret;
        case VIRTUAL_REG:
            if (op.id < 0 || op.id > MAX_VIRTUAL_REGS) {
                printf("ERROR: Register r%d does not exist\n", op.id);
                exit(EXIT_FAILURE);
            } else if (machine->reg[op.id] == UNINIT_REG) {
                printf("WARNING: Potential uninitialized read from register r%d\n", op.id);
            }
            return machine->reg[op.id];
        case PHYSICAL_REG:
            if (op.id < 0 || op.id > MAX_PHYSICAL_REGS) {
                printf("ERROR: Register R%d does not exist\n", op.id);
                exit(EXIT_FAILURE);
            } else if (machine->pr[op.id] == UNINIT_REG) {
                printf("WARNING: Potential uninitialized read from register R%d\n", op.id);
            }
            return machine->pr[op.id];
        default:
            printf("ERROR: Cannot read register using a non-register operand: ");
            Operand_print(op, stdout);
            printf("\n");
            exit(EXIT_FAILURE);
    }
}

void ILOCMachine_set_mem(ILOCMachine* machine, long address, word_t value)
{
    if (address < 0 || address > MEM_SIZE - WORD_SIZE) {
        printf("ERROR: Address %ld is invalid (out of range)\n", address);
        exit(EXIT_FAILURE);
    }
    /* actual memory write */
    *(word_t*)(machine->mem + address) = value;
}

word_t ILOCMachine_get_mem(ILOCMachine* machine, long address)
{
    if (address < 0 || address > MEM_SIZE - WORD_SIZE) {
        printf("ERROR: Address %ld is invalid (out of range)\n", address);
        exit(EXIT_FAILURE);
    }
    /* actual memory read */
    return *(word_t*)(machine->mem + address);
}

void ILOCMachine_print(ILOCMachine* machine, FILE* output)
{
    fprintf(output, "==========================\n");

    /* registers (special and virtual) */
    fprintf(output, "sp=" PRIW " bp=" PRIW " ret=" PRIW "\n", machine->sp, machine->bp, machine->ret);
    fprintf(output, "registers: ");
    for (int i = 0; i < MAX_VIRTUAL_REGS; i++) {
        if (machine->reg[i] != UNINIT_REG) {
            fprintf(output, " r%d=" PRIW, i, machine->reg[i]);
        }
    }
    for (int i = 0; i < MAX_PHYSICAL_REGS; i++) {
        if (machine->pr[i] != UNINIT_REG) {
            fprintf(output, " R%d=" PRIW, i, machine->pr[i]);
        }
    }
    fprintf(output, "\n");
    
    /* stack (memory from MEM_SIZE down to stack pointer) */
    fprintf(output, "stack:");
    for (int addr = MEM_SIZE - WORD_SIZE; addr >= machine->sp; addr -= WORD_SIZE) {
        fprintf(output, "  %d: " PRIW, addr, ILOCMachine_get_mem(machine, addr));
    }
    fprintf(output, "\n");

    /* other memory (any WORD_SIZE-aligned value that is non-zero) */
    fprintf(output, "other memory:");
    for (int addr = STATIC_VAR_OFFSET; addr < machine->sp; addr += WORD_SIZE) {
        word_t value = ILOCMachine_get_mem(machine, addr);
        if (value != 0) {
            fprintf(output, "  %d: " PRIW, addr, value);
        }
    }
    fprintf(output, "\n");

    fprintf(output, "==========================\n");
}

void ILOCMachine_free(ILOCMachine* machine)
{
    CallTargetList_free(machine->call_targets);
    free(machine);
}

void assert_operand_count (ILOCInsn* insn, int count)
{
    int actual_count = ILOCInsn_get_operand_count(insn);
    if (actual_count != count) {
        printf("ERROR: Invalid instruction (expected %d operands but found %d): ",
                count, actual_count);
        ILOCInsn_print(insn, stdout);
        printf("\n");
        exit(EXIT_FAILURE);
    }
}

void assert_operand_is_register (ILOCInsn* insn, Operand op)
{
    if (op.type != STACK_REG  && op.type != BASE_REG &&
        op.type != RETURN_REG && op.type != VIRTUAL_REG && op.type != PHYSICAL_REG)
    {
        printf("ERROR: Invalid operand '");
        Operand_print(op, stdout);
        printf("' (expected register): ");
        ILOCInsn_print(insn, stdout);
        printf("\n");
        exit(EXIT_FAILURE);
    }
}

void assert_all_register_operands (ILOCInsn* insn, int count)
{
    assert_operand_count(insn, count);
    for (int i = 0; i < count; i++) {
        assert_operand_is_register(insn, insn->op[i]);
    }
}

void assert_operand_type (ILOCInsn* insn, Operand op, OperandType type)
{
    if (op.type != type) {
        printf("ERROR: Invalid operand '");
        Operand_print(op, stdout);
        printf("': ");
        ILOCInsn_print(insn, stdout);
        printf("\n");
        exit(EXIT_FAILURE);
    }
}

void assert_valid_insn (ILOCInsn* insn)
{
    switch (insn->form)
    {
        /* no operands */
        case RETURN:
        case NOP:
            assert_operand_count(insn, 0);
            break;

        /* reg */
        case PUSH:
        case POP:
            assert_operand_count(insn, 1);
            assert_operand_is_register(insn, insn->op[0]);
            break;

        /* reg, reg */
        case I2I:
        case NOT:
        case NEG:
        case LOAD:
        case STORE:
            assert_all_register_operands(insn, 2);
            break;

        /* reg, reg, reg */
        case ADD:
        case SUB:
        case MULT:
        case DIV:
        case AND:
        case OR:
        case CMP_LT:
        case CMP_LE:
        case CMP_EQ:
        case CMP_GE:
        case CMP_GT:
        case CMP_NE:
        case LOAD_AO:
        case STORE_AO:
        case PHI:
            assert_all_register_operands(insn, 3);
            break;

        /* int, reg */
        case LOAD_I:
            assert_operand_count(insn, 2);
            assert_operand_type(insn, insn->op[0], INT_CONST);
            assert_operand_is_register(insn, insn->op[1]);
            break;

        /* reg, int, reg */
        case ADD_I:
        case MULT_I:
        case LOAD_AI:
            assert_operand_count(insn, 3);
            assert_operand_is_register(insn, insn->op[0]);
            assert_operand_type(insn, insn->op[1], INT_CONST);
            assert_operand_is_register(insn, insn->op[2]);
            break;

        /* reg, reg, int */
        case STORE_AI:
            assert_operand_count(insn, 3);
            assert_operand_is_register(insn, insn->op[0]);
            assert_operand_is_register(insn, insn->op[1]);
            assert_operand_type(insn, insn->op[2], INT_CONST);
            break;

        /* lbl */
        case CALL:
            assert_operand_count(insn, 1);
            assert_operand_type(insn, insn->op[0], CALL_LABEL);
            break;
        case JUMP:
            assert_operand_count(insn, 1);
            assert_operand_type(insn, insn->op[0], JUMP_LABEL);
            break;

        /* reg, lbl, lbl */
        case CBR:
            assert_operand_count(insn, 3);
            assert_operand_is_register(insn, insn->op[0]);
            assert_operand_type(insn, insn->op[1], JUMP_LABEL);
            assert_operand_type(insn, insn->op[2], JUMP_LABEL);
            break;

        /* lbl */
        case LABEL:
            assert_operand_count(insn, 1);
            if (insn->op[0].type != CALL_LABEL &&
                insn->op[0].type != JUMP_LABEL)
            {
                printf("Invalid label '");
                Operand_print(insn->op[0], stdout);
                printf("': ");
                ILOCInsn_print(insn, stdout);
                printf("\n");
                exit(EXIT_FAILURE);
            }
            break;

        /* int/str/reg */
        case PRINT:
            assert_operand_count(insn, 1);
            if (insn->op[0].type != STACK_REG &&
                insn->op[0].type != BASE_REG &&
                insn->op[0].type != RETURN_REG &&
                insn->op[0].type != VIRTUAL_REG &&
                insn->op[0].type != PHYSICAL_REG &&
                insn->op[0].type != INT_CONST &&
                insn->op[0].type != STR_CONST)
            {
                printf("Invalid parameter '");
                Operand_print(insn->op[0], stdout);
                printf("': ");
                ILOCInsn_print(insn, stdout);
                printf("\n");
                exit(EXIT_FAILURE);
            }
            break;

        default:
            printf("Unrecognized instruction: ");
            ILOCInsn_print(insn, stdout);
            printf("\n");
            exit(EXIT_FAILURE);
    }
}

/*
 * shortcut macros to make the simulator code cleaner
 */

#define OP0    (machine->pc->op[0])
#define OP1    (machine->pc->op[1])
#define OP2    (machine->pc->op[2])
#define IMMOP0 (machine->pc->op[0].imm)
#define IMMOP1 (machine->pc->op[1].imm)
#define IMMOP2 (machine->pc->op[2].imm)
#define STROP0 (machine->pc->op[0].str)

#define SET_REG(OP,VAL)   ILOCMachine_set_reg(machine, (OP), (VAL))
#define GET_REG(OP)       ILOCMachine_get_reg(machine, (OP))
#define SET_MEM(ADDR,VAL) ILOCMachine_set_mem(machine, (ADDR), (VAL))
#define GET_MEM(ADDR)     ILOCMachine_get_mem(machine, (ADDR))

#define PUSH(VAL)   machine->sp -= WORD_SIZE; \
                    if (machine->sp <= STATIC_VAR_OFFSET) { \
                        printf("ERROR: Stack overflow\n"); \
                        exit(EXIT_FAILURE); \
                    } \
                    ILOCMachine_set_mem(machine, machine->sp, (VAL));

#define POP(LOC)    if (machine->sp > MEM_SIZE - WORD_SIZE) { \
                        printf("ERROR: Cannot pop from empty stack\n"); \
                        exit(EXIT_FAILURE); \
                    } \
                    *(LOC) = ILOCMachine_get_mem(machine, machine->sp); \
                    machine->sp += WORD_SIZE;

#define TIMEOUT_NUM_INSTRUCTIONS 100000000

long run_simulator (InsnList* program, bool print_trace)
{
    /* initialize machine */
    ILOCMachine* machine = ILOCMachine_new();
    machine->sp = MEM_SIZE;

    /* build jump and call target indices */
    int i = 0;
    FOR_EACH (ILOCInsn*, insn, program) {
        machine->instructions[i++] = insn;
        if (insn->form == LABEL) {
            if (insn->op[0].type == JUMP_LABEL) {
                machine->jump_targets[insn->op[0].id] = insn;
            } else {
                CallTargetList_add_new(machine->call_targets, insn->op[0].str, insn);
            }
        }
        if (i == MAX_INSTRUCTIONS) {
            printf("Exceeds maximum instruction count (%d) of the simulator\n", MAX_INSTRUCTIONS);
            exit(EXIT_FAILURE);
        }
    }

    /* search for main and begin there */
    machine->pc = CallTargetList_find(machine->call_targets, "main")->next;

    /* main program loop */
    int num_instructions_executed = 0;
    while (machine->pc != NULL) {

        /* assumes no jumps; may be overwritten later */
        ILOCInsn* next_insn = machine->pc->next;

        /* print trace debug info if desired */
        if (print_trace) {
            printf("\n");
            ILOCMachine_print(machine, stdout);
            printf("\nExecuting: ");
            ILOCInsn_print(machine->pc, stdout);
            printf("\n");
        }

        /* verify that current instruction is valid */
        assert_valid_insn(machine->pc);

        /* handle current instruction */
        switch (machine->pc->form)
        {
            case LOAD_I:   SET_REG(OP1, IMMOP0);                               break;
            case LOAD:     SET_REG(OP1, GET_MEM(GET_REG(OP0)));                break;
            case LOAD_AI:  SET_REG(OP2, GET_MEM(GET_REG(OP0) + IMMOP1));       break;
            case LOAD_AO:  SET_REG(OP2, GET_MEM(GET_REG(OP0) + GET_REG(OP1))); break;
            case STORE:    SET_MEM(GET_REG(OP1),                GET_REG(OP0)); break;
            case STORE_AI: SET_MEM(GET_REG(OP1) + IMMOP2,       GET_REG(OP0)); break;
            case STORE_AO: SET_MEM(GET_REG(OP1) + GET_REG(OP2), GET_REG(OP0)); break;

            case ADD:    SET_REG(OP2, GET_REG(OP0) +  GET_REG(OP1)); break;
            case SUB:    SET_REG(OP2, GET_REG(OP0) -  GET_REG(OP1)); break;
            case MULT:   SET_REG(OP2, GET_REG(OP0) *  GET_REG(OP1)); break;
            case DIV:    SET_REG(OP2, GET_REG(OP0) /  GET_REG(OP1)); break;
            case AND:    SET_REG(OP2, GET_REG(OP0) &  GET_REG(OP1)); break;
            case OR:     SET_REG(OP2, GET_REG(OP0) |  GET_REG(OP1)); break;
            case CMP_LT: SET_REG(OP2, GET_REG(OP0) <  GET_REG(OP1)); break;
            case CMP_LE: SET_REG(OP2, GET_REG(OP0) <= GET_REG(OP1)); break;
            case CMP_EQ: SET_REG(OP2, GET_REG(OP0) == GET_REG(OP1)); break;
            case CMP_NE: SET_REG(OP2, GET_REG(OP0) != GET_REG(OP1)); break;
            case CMP_GE: SET_REG(OP2, GET_REG(OP0) >= GET_REG(OP1)); break;
            case CMP_GT: SET_REG(OP2, GET_REG(OP0) >  GET_REG(OP1)); break;

            case ADD_I:  SET_REG(OP2, GET_REG(OP0) + IMMOP1); break;
            case MULT_I: SET_REG(OP2, GET_REG(OP0) * IMMOP1); break;

            case I2I:    SET_REG(OP1,    GET_REG(OP0) );    break;
            case NOT:    SET_REG(OP1, ((~GET_REG(OP0))&1)); break;
            case NEG:    SET_REG(OP1,  -(GET_REG(OP0)));    break;

            case PUSH:
                PUSH(GET_REG(OP0));
                break;

            case POP:
            {
                word_t tmp;
                POP(&tmp);
                SET_REG(OP0, tmp);
                break;
            }

            case JUMP:
                next_insn = machine->jump_targets[OP0.id]->next;
                break;

            case CBR:
                if ((bool)GET_REG(OP0)) {
                    next_insn = machine->jump_targets[OP1.id]->next;
                } else {
                    next_insn = machine->jump_targets[OP2.id]->next;
                }
                break;

            case CALL:
            {
                /* calculate index of next instruction */
                int idx = 0;
                for (next_insn = program->head;
                     next_insn != NULL && next_insn != machine->pc->next;
                     next_insn = next_insn->next) {
                    idx++;
                }
                PUSH((word_t)idx);
                next_insn = CallTargetList_find(machine->call_targets, STROP0)->next;
                break;
            }

            case RETURN:
            {
                if (machine->sp == MEM_SIZE) {
                    /* stack is empty, so this must be the return from main() */
                    next_insn = NULL;
                    break;
                }
                word_t tmp;
                POP(&tmp);
                next_insn = machine->instructions[tmp];
                break;
            }

            case PRINT:
                if (OP0.type == STR_CONST) {
                    printf("%s", STROP0);
                } else {  /* virtual register */
                    printf(PRIW, GET_REG(OP0));
                }
                break;

            case LABEL:
            case NOP:
            case PHI:
                /* nothing to do */
                break;
        }

        /* update pc */
        machine->pc = next_insn;

        /* check timeout */
        num_instructions_executed++;
        if (num_instructions_executed > TIMEOUT_NUM_INSTRUCTIONS) {
            fprintf(stderr, "TIMEOUT: Program executed too many instructions (probably an infinite loop)");
            exit(EXIT_FAILURE);
        }
    }

    /* clean up */
    word_t return_value = machine->ret;
    CallTargetList_free(machine->call_targets);
    free(machine);

    return (long)return_value;
}
