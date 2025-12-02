#include "y86.h"

static FILE* out = NULL;

#define ONE "%rbx"
#define RSP "%rsp"

#define IOSRC "%rsi"
#define IODST "%rdi"

#define TMP1 "%r8"
#define TMP2 "%r9"
#define TMP3 "%r12"
#define TMP4 "%r13"
#define TMP5 "%r14"

/*
 * For reference:
 *
 * rax - return register (RET)
 * rcx - r0
 * rdx - r1
 * rbx - literal 1
 * rsp - stack pointer (SP)
 * rbp - base pointer (BP)
 * rsi - I/O source
 * rdi - I/O destination
 * r8  - reserved
 * r9  - reserved
 * r10 - r2
 * r11 - r3
 * r12 - reserved
 * r13 - reserved
 * r14 - reserved (currently unused)
 */
const char* reg_name(Operand op)
{
    const char* reg = "INVALID";
    switch (op.type) {
        case BASE_REG:   reg = "%rbp"; break;   // BP
        case STACK_REG:  reg = "%rsp"; break;   // SP
        case RETURN_REG: reg = "%rax"; break;   // RET
        case PHYSICAL_REG:
            if (op.id >= 4) {
                fprintf(stderr, "Invalid register: ");
                Operand_print(op, stderr);
                fprintf(stderr, " (must be R0-R3 for translation to physical register)\n");
                exit(EXIT_FAILURE);
            }
            switch (op.id) {
                case 0: reg = "%rcx"; break;    // R0
                case 1: reg = "%rdx"; break;    // R1
                case 2: reg = "%r10"; break;    // R2
                case 3: reg = "%r11"; break;    // R3
            }
            break;
        default:
            break;
    }
    return reg;
}

void emit_call_label (const char* text)
{
    fprintf(out, "%s:\n", text);
}

void emit_jump_label (int id)
{
    fprintf(out, "l%d:\n", id);
}

void emit (const char* text)
{
    fprintf(out, "    %s\n", text);
}

void emit_w_comment (const char* text, const char* comment)
{
    fprintf(out, "    %-40s# %s\n", text, comment);
}

void emitf (const char* format, ...)
{
    char buffer[MAX_LINE_LEN];

    /* delegate to vsnprintf */
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, MAX_LINE_LEN, format, args);
    va_end(args);

    emit(buffer);
}

void emit_bin_op (const char* opcode, Operand op0, Operand op1, Operand op2)
{
    if (op0.id == op2.id) {
        /* first operand is also the output; overwrite it */
        emitf("%s %s, %s", opcode, reg_name(op1), reg_name(op0));
    } else if (op1.id == op2.id) {
        /* second operand is also the output; overwrite it */
        emitf("%s %s, %s", opcode, reg_name(op0), reg_name(op1));
    } else {
        /* no operands duplicated; use an extra move instruction */
        emitf("rrmovq %s, %s", reg_name(op0), reg_name(op2));
        emitf("%s %s, %s", opcode, reg_name(op1), reg_name(op2));
    }
}

void emit_cmp (const char* opcode, Operand op0, Operand op1, Operand op2)
{
    emitf("xorq %s, %s", TMP1, TMP1);
    emitf("rrmovq %s, %s", reg_name(op0), TMP2);
    emitf("subq %s, %s", reg_name(op1), TMP2);
    emitf("cmov%s %s, %s", opcode, ONE, TMP1);
    emitf("rrmovq %s, %s", TMP1, reg_name(op2));
}

#define OP0 (i->op[0])
#define OP1 (i->op[1])
#define OP2 (i->op[2])
#define REG0 reg_name(OP0)
#define REG1 reg_name(OP1)
#define REG2 reg_name(OP2)

#define MAX_STRINGS 256

void emit_y86 (InsnList* iloc, FILE* output)
{
    const char* strings[MAX_STRINGS];
    int num_strings = 0;
    bool need_mult = false;
    bool need_div = false;

    out = output;

    /* address zero boilerplate (for compatibility with CS:APP simulator) */
    emit(".pos 0 code");
    emit("jmp _start");
    emit("");

    /* static data (not really necessary b/c Decaf has no initialized data region) */
    emit(".pos 0x100 data");
    /* TODO: detect static size and emit ".quad" directives? */
    emit("");

    /* entry point boilerplate (for compatibility with CS 261 projects) */
    emit(".pos 0x400 code");
    emit_call_label("_start");
    emitf("irmovq $1, %s", ONE);
    emit("irmovq _stack, %rsp");
    emit("call main");
    emit("halt");
    emit("");

    FOR_EACH (ILOCInsn*, i, iloc)
    {
        switch (i->form)
        {
            /* data movement (relatively straightforward conversions) */

            case I2I:       emitf("rrmovq %s, %s", REG0, REG1);                 break;
            case PUSH:      emitf("pushq %s", REG0);                            break;
            case POP:       emitf("popq %s", REG0);                             break;
            case LOAD_I:    emitf("irmovq $%d, %s", OP0.imm, REG1);             break;
            case LOAD:      emitf("mrmovq (%s), %s", REG0, REG1);               break;
            case LOAD_AI:   emitf("mrmovq $%d(%s), %s", OP1.imm, REG0, REG2);   break;
            case LOAD_AO:   emitf("rrmovq %s, %s", REG0, TMP1);
                            emitf("addq %s, %s", REG1, TMP1);
                            emitf("mrmovq (%s), %s", TMP1, REG2);               break;
            case STORE:     emitf("rmmovq %s, (%s)", REG0, REG1);               break;
            case STORE_AI:  emitf("rmmovq %s, $%d(%s)", REG0, OP2.imm, REG1);   break;
            case STORE_AO:  emitf("rrmovq %s, %s", REG1, TMP1);
                            emitf("addq %s, %s", REG2, TMP1);
                            emitf("rmmovq %s, (%s)", REG0, TMP1);               break;

            /* unary and binary operations (complicated by Y86's minimal OPq instruction) */

            case ADD:       emit_bin_op("addq", OP0, OP1, OP2); break;
            case SUB:       emit_bin_op("subq", OP0, OP1, OP2); break;
            case AND:       emit_bin_op("andq", OP0, OP1, OP2); break;

            case ADD_I:     emitf("irmovq $%d, %s", OP1.imm, TMP1);
                            if (OP0.id != OP2.id) {
                                emitf("rrmovq %s, %s", REG0, REG2);
                            }
                            emitf("addq %s, %s", TMP1, REG2);
                            break;

            case MULT:      emitf("rrmovq %s, %s", REG0, TMP1);
                            emitf("rrmovq %s, %s", REG1, TMP2);
                            emitf("call _builtin_mult");
                            emitf("rrmovq %s, %s", TMP3, REG2);
                            need_mult = true;
                            break;

            case MULT_I:    emitf("rrmovq %s, %s", REG0, TMP1);
                            emitf("irmovq $%d, %s", OP1.imm, TMP2);
                            emitf("call _builtin_mult");
                            emitf("rrmovq %s, %s", TMP3, REG2);
                            need_mult = true;
                            break;

            case DIV:       emitf("rrmovq %s, %s", REG0, TMP1);
                            emitf("rrmovq %s, %s", REG1, TMP2);
                            emitf("call _builtin_div");
                            emitf("rrmovq %s, %s", TMP3, REG2);
                            need_div = true;
                            break;

            /* OR(x,y) == XOR(x,y) + AND(x,y) */
            case OR:        emitf("rrmovq %s, %s", REG0, TMP1);
                            emitf("xorq %s, %s", REG1, TMP1);
                            emitf("rrmovq %s, %s", REG0, TMP2);
                            emitf("andq %s, %s", REG1, TMP2);
                            emitf("addq %s, %s", TMP2, TMP1);
                            emitf("rrmovq %s, %s", TMP1, REG2);
                            break;

            /* -x = 0 - x */
            case NEG:       emitf("xorq %s, %s", TMP1, TMP1);
                            emitf("subq %s, %s", REG0, TMP1);
                            emitf("rrmovq %s, %s", TMP1, REG1);
                            break;

            /* !x = XOR(1, x) */
            case NOT:       if (OP0.id != OP1.id) {
                                emitf("rrmovq %s, %s", REG0, REG1);
                            }
                            emitf("xorq %s, %s", ONE, REG1);
                            break;


            /* comparisons -- delegate to helper method that uses cmov */

            case CMP_GT: emit_cmp("g",  OP0, OP1, OP2); break;
            case CMP_GE: emit_cmp("ge", OP0, OP1, OP2); break;
            case CMP_LT: emit_cmp("l",  OP0, OP1, OP2); break;
            case CMP_LE: emit_cmp("le", OP0, OP1, OP2); break;
            case CMP_EQ: emit_cmp("e",  OP0, OP1, OP2); break;
            case CMP_NE: emit_cmp("ne", OP0, OP1, OP2); break;

            /* control flow handlers (relatively straightforward conversions) */

            case LABEL:
                if (OP0.type == CALL_LABEL) {
                    emit_call_label(OP0.str);
                } else {
                    emit_jump_label(OP0.id);
                }
                break;

            case JUMP:
                emitf("jmp l%d", OP0.id);
                break;

            case CBR:
                emitf("andq %s, %s", REG0, REG0);
                emitf("jne l%d", OP1.id); /* true */
                emitf("jmp l%d", OP2.id); /* false */
                break;

            case CALL:
                emitf("call %s", OP0.str);
                break;

            case RETURN:
                emit("ret");
                break;

            /* misc instructions */

            case PRINT:
            {
                switch(OP0.type)
                {
                    case PHYSICAL_REG:
                        emitf("pushq %s", REG0);
                        emitf("rrmovq %s, %s", RSP, IOSRC);
                        emit("iotrap 2");   /* DECOUT */
                        emit("iotrap 5");   /* FLUSH */
                        emitf("popq %s", REG0);
                        break;

                    case STR_CONST:
                    {
                        int sidx = num_strings;
                        for (int s = 0; s < num_strings; s++) {
                            if (token_str_eq(strings[s], OP0.str)) {
                                sidx = s;
                            }
                        }
                        if (sidx == num_strings) {
                            strings[num_strings] = (const char*)&(OP0.str);
                            num_strings++;
                        }
                        emitf("irmovq _str%d, %s", sidx, IOSRC);
                        emit("iotrap 4");   /* STROUT */
                        emit("iotrap 5");   /* FLUSH */
                        break;
                    }

                    default:
                        printf("Unsupported instruction: ");
                        ILOCInsn_print(i, output);
                        printf("\n");
                        break;
                }
                break;
            }

            case NOP:
                emit("nop");    /* not really necessary; included for completeness */
                break;

            case PHI:
                /* nothing to do */
                break;

            default:
                printf("Unsupported instruction: ");
                ILOCInsn_print(i, output);
                printf("\n");
                break;
        }
    }

    if (need_mult) {
        /* TODO: handle negative numbers */
        emit("");
        emit_call_label("_builtin_mult");   /* x in TMP1, y in TMP2 */
        emitf("xorq %s, %s", TMP3, TMP3);   /* prod = 0 */
        emit_call_label("_mul_loop");
        emitf("andq %s, %s", TMP1, TMP1);
        emitf("je _mul_done");              /* while (x != 0): */
        emitf("addq %s, %s", TMP2, TMP3);   /*   prod += y */
        emitf("subq %s, %s", ONE, TMP1);    /*   x = x - 1 */
        emitf("jmp _mul_loop");
        emit_call_label("_mul_done");
        emit("ret");                        /* return prod in TMP3 */
    }

    if (need_div) {
        /* TODO: handle negative numbers */
        emit("");
        emit_call_label("_builtin_div");    /* x in TMP1, y in TMP2 */
        emitf("xorq %s, %s", TMP3, TMP3);   /* div = 0 */
        emit_call_label("_div_loop");
        emitf("xorq %s, %s", TMP4, TMP4);
        emitf("subq %s, %s", TMP1, TMP4);
        emitf("jge _div_done");             /* while (0 < x): */
        emitf("subq %s, %s", TMP2, TMP1);   /*   x -= y */
        emitf("addq %s, %s", ONE, TMP3);    /*   div = div + 1 */
        emitf("jmp _div_loop");
        emit_call_label("_div_done");
        emit("ret");                        /* return div in TMP3 */
    }

    /* emit string table if needed */
    if (num_strings > 0) {
        emit("");
        emit(".pos 0xa00 rodata");
        for (int s = 0; s < num_strings; s++) {
            fprintf(out, "_str%d:\n", s);
            fprintf(out, "    .string \"");
            print_escaped_string(strings[s], out);
            fprintf(out, "\"\n");
        }
    }

    /* emit stack location marker */
    emit("");
    emit(".pos 0xf00 stack");
    emit_call_label("_stack");
    emit("");
}
