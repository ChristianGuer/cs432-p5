/**
 * @file iloc.h
 * @brief Declarations of ILOC structures
 *
 * This module provides declarations of all structures and framework functions
 * for ILOC programs. It is necessary for Projects 4 (code generation) and 5
 * (register allocation).
 */
#ifndef __ILOC_H
#define __ILOC_H

#include "common.h"
#include "token.h"
#include "ast.h"
#include "visitor.h"
#include "symbol.h"

/**
 * @brief Machine word size (64 bits)
 */
#define WORD_SIZE 8

/**
 * @brief Machine memory size (64K)
 */
#define MEM_SIZE  65536

/**
 * @brief Maximum number of virtual registers
 */
#define MAX_VIRTUAL_REGS 2048

/**
 * @brief Maximum number of physical registers
 */
#define MAX_PHYSICAL_REGS 32

/**
 * @brief Maximum number of instructions
 */
#define MAX_INSTRUCTIONS 4096

/**
 * @brief Base pointer offset for parameters
 * 
 * Skips base pointer and return address
 */
#define PARAM_BP_OFFSET   (2 * WORD_SIZE)

/**
 * @brief Base pointer offset for local variables
 */
#define LOCAL_BP_OFFSET   (-WORD_SIZE)

/**
 * @brief Base offset/address in memory for static/global variables
 */
#define STATIC_VAR_OFFSET 0x100

/**
 * @brief Operand type
 */
typedef enum OperandType
{
    EMPTY,          /**< @brief No operand */
    STACK_REG,      /**< @brief Stack pointer register (SP) */
    BASE_REG,       /**< @brief Base pointer register (BP) */
    RETURN_REG,     /**< @brief Return value register (RET) */
    VIRTUAL_REG,    /**< @brief Numbered virtual register */
    PHYSICAL_REG,   /**< @brief Numbered physical register */
    JUMP_LABEL,     /**< @brief Numbered jump label */
    CALL_LABEL,     /**< @brief Textual call label */
    INT_CONST,      /**< @brief Integer constant/literal */
    STR_CONST       /**< @brief String constant/literal */
} OperandType;

/**
 * @brief ILOC operand structure
 */
typedef struct Operand
{
    /**
     * @brief Operand type (discriminator/tag for anonymous union)
     */
    OperandType type;

    union {
        int id;                     /**< @brief Virtual/physical register or jump label ID */
        long imm;                   /**< @brief Integer constant/literal */
        char str[MAX_LINE_LEN];     /**< @brief String constant/literal */
    };

} Operand;

/**
 * @brief Create a empty operand
 */
Operand empty_operand (void);

/**
 * @brief Create a SP operand
 */
Operand stack_register (void);

/**
 * @brief Create a BP operand
 */
Operand base_register (void);

/**
 * @brief Create a RET operand
 */
Operand return_register (void);

/**
 * @brief Create a new virtual register operand (uses next available ID)
 */
Operand virtual_register (void);

/**
 * @brief Create a physical register operand with a specific register ID
 */
Operand physical_register (int id);

/**
 * @brief Create a new jump label operand (uses next available ID)
 */
Operand anonymous_label (void);

/**
 * @brief Create a call label with the a specific name
 */
Operand call_label (const char* label);

/**
 * @brief Create an integer constant operand with a specific value
 */
Operand int_const (long integer);

/**
 * @brief Create a string constant operand with a specific value
 */
Operand str_const (const char* string);

/**
 * @brief Print an operand
 * 
 * @param op Operand to print
 * @param output File stream to print to
 */
void Operand_print (Operand op, FILE* output);

/** 
 * @brief ILOC instruction form
 * 
 * Every ILOC instruction has a base type or "form" (it is also sometimes called
 * a "class" but we avoid this term because it implies objects). The form is
 * usually encoded as a number (e.g., opcode or icode) but because ILOC is
 * intended to be a higher-level intermediate assembly language we do not
 * specify the ABI down to that level.
 */
typedef enum InsnForm
{
    /* Forms documented in EAC2 Appendix A */

    ADD,        /**< @brief Integer addition (r1 + r2 => r3) */
    SUB,        /**< @brief Integer subtraction (r1 - r2 => r3) */
    MULT,       /**< @brief Integer multiplication (r1 * r2 => r3) */
    DIV,        /**< @brief Integer division (r1 / r2 => r3) w/ truncation of answer if fractional */
    AND,        /**< @brief Boolean AND (r1 && r2 => r3) */
    OR,         /**< @brief Boolean OR (r1 || r2 => r3) */
    LOAD_I,     /**< @brief Load/set a register to an immediate value */
    LOAD,       /**< @brief Load from memory using address (reg) addressing */
    LOAD_AI,    /**< @brief Load from memory using address + immediate (reg/imm) addressing */
    LOAD_AO,    /**< @brief Load from memory using address + offset (reg/reg) addressing */
    STORE,      /**< @brief Store to memory using address (reg) addressing */
    STORE_AI,   /**< @brief Store to memory using address + immediate (reg/imm) addressing */
    STORE_AO,   /**< @brief Store to memory using address + offset (reg/reg) addressing */
    NOP,        /**< @brief Do nothing (sometimes useful for testing) */
    I2I,        /**< @brief Copy one integer register to another */
    JUMP,       /**< @brief Unconditional control branch to a target label */
    CBR,        /**< @brief Conditional control branch to one of two target labels */
    CMP_LT,     /**< @brief Compare integers and save the boolean result (r1 <  r2 => r3) */
    CMP_LE,     /**< @brief Compare integers and save the boolean result (r1 <= r2 => r3) */
    CMP_EQ,     /**< @brief Compare integers and save the boolean result (r1 == r2 => r3) */
    CMP_GE,     /**< @brief Compare integers and save the boolean result (r1 >= r2 => r3) */
    CMP_GT,     /**< @brief Compare integers and save the boolean result (r1 >  r2 => r3) */
    CMP_NE,     /**< @brief Compare integers and save the boolean result (r1 != r2 => r3) */

    /* New or modified forms */

    ADD_I,  /**< @brief Add an immediate/constant value to a register value (r1 + c1 => r2) */
    MULT_I, /**< @brief Multiply an immediate/constant value with a register value (r1 * c1 => r2) */
    NOT,    /**< @brief Boolean NOT (!r1 => r2) */
    NEG,    /**< @brief Integer negation (-r1 => r2) */
    PUSH,   /**< @brief Push register value onto system stack (decrement SP and store from r1) */
    POP,    /**< @brief Pop register value from system stack (load into r2 and increment SP) */
    LABEL,  /**< @brief Jump label w/ integer ID (no effect) */
    CALL,   /**< @brief Call a function (push return address and jump) */
    RETURN, /**< @brief Return from a function (pop return address and jump) */
    PRINT,  /**< @brief Print a constant or register value */
    PHI     /**< @brief Combine two registers in SSA form */

} InsnForm;

/**
 * @brief ILOC instruction
 * 
 * There are four normal constructors, based on the number of operands needed:
 *   * @ref ILOCInsn_new_3op
 *   * @ref ILOCInsn_new_2op
 *   * @ref ILOCInsn_new_1op
 *   * @ref ILOCInsn_new_0op
 * 
 * There is also a copy constructor (@ref ILOCInsn_copy). Instructions should
 * be deallocated using @ref ILOCInsn_free.
 * 
 * Members:
 *   * @ref ILOCInsn_set_comment
 *   * @ref ILOCInsn_copy
 *   * @ref ILOCInsn_print
 *   * @ref ILOCInsn_get_operand_count
 *   * @ref ILOCInsn_get_read_registers
 *   * @ref ILOCInsn_get_write_register

 */
typedef struct ILOCInsn
{
    /**
     * @brief Type ("form") of instruction
     */
    InsnForm form;

    /**
     * @brief Operands
     * 
     * Fixed-size array of three elements; if the instruction requires fewer
     * operands, any unused operands should be @c EMPTY.
     */
    Operand op[3];

    /**
     * @brief Comment associated with this instruction
     * 
     * An empty string (only a null terminator) indicates there is no comment.
     */
    char comment[MAX_LINE_LEN];

    /**
     * @brief Next instruction (if stored in a list)
     */
    struct ILOCInsn* next;

} ILOCInsn;

/**
 * @brief Create a new 3-operand instruction
 * 
 * @param form Instruction type/form
 * @param op1 First operand
 * @param op2 Second operand
 * @param op3 Third operand
 * @returns Pointer to new instruction
 */
ILOCInsn* ILOCInsn_new_3op (InsnForm form, Operand op1, Operand op2, Operand op3);

/**
 * @brief Create a new 2-operand instruction
 * 
 * @param form Instruction type/form
 * @param op1 First operand
 * @param op2 Second operand
 * @returns Pointer to new instruction
 */
ILOCInsn* ILOCInsn_new_2op (InsnForm form, Operand op1, Operand op2);

/**
 * @brief Create a new 1-operand instruction
 * 
 * @param form Instruction type/form
 * @param op1 First operand
 * @returns Pointer to new instruction
 */
ILOCInsn* ILOCInsn_new_1op (InsnForm form, Operand op1);

/**
 * @brief Create a new 0-operand instruction
 * 
 * @param form Instruction type/form
 * @returns Pointer to new instruction
 */
ILOCInsn* ILOCInsn_new_0op (InsnForm form);

/**
 * @brief Add a comment to an instruction
 * 
 * @param insn Instruction to add a comment to
 * @param comment Comment to add
 */
void ILOCInsn_set_comment (ILOCInsn* insn, const char* comment);

/**
 * @brief Create a new instruction that is a copy of an existing instruction
 * 
 * @param insn Instruction to copy
 * @returns Pointer to a new instruction with all the data copied from the given instruction
 */
ILOCInsn* ILOCInsn_copy (ILOCInsn* insn);

/**
 * @brief Print an instruction
 * 
 * @param insn Instruction to print
 * @param output File stream to print to
 */
void ILOCInsn_print (ILOCInsn* insn, FILE* output);

/**
 * @brief Count the number of operands in an instruction
 * 
 * @param insn Instruction to count operands of
 * @returns Number of non-empty operands in the instruction
 */
int ILOCInsn_get_operand_count (ILOCInsn* insn);

/**
 * @brief Get a list of registers that are read from by this instruction
 * 
 * This function returns the registers inside of a new "fake" instruction
 * because C doesn't allow us to return an array of operands -- don't
 * forget to deallocate that instruction when you're done with it.
 * 
 * @param insn Instruction to examine
 * @returns Fake @c NOP instruction with the relevant registers as operands
 */
ILOCInsn* ILOCInsn_get_read_registers (ILOCInsn* insn);

/**
 * @brief Get the register (if any) that is written to by this instruction
 * 
 * @param insn Instruction to examine
 * @returns Operand that is written (or an @c EMPTY operand if there is none)
 */
Operand ILOCInsn_get_write_register (ILOCInsn* insn);

/**
 * @brief Deallocate an instruction structure
 * 
 * @param insn Instruction to deallocate
 */
void ILOCInsn_free (ILOCInsn* insn);

DECL_LIST_TYPE (Insn, ILOCInsn*)

/**
 * @brief Print an instruction list with proper indentation and comments
 * 
 * @param list List of instructions to print
 * @param output File stream to print to
 */
void InsnList_print (InsnList* list, FILE* output);

/**
 * @brief Create a new AST visitor that allocates addresses for all variable symbols
 *
 * @returns Pointer to visitor structure
 */
NodeVisitor* AllocateSymbolsVisitor_new(void);

/**
 * @brief Print a register attribute
 * 
 * @param reg Pointer to register attribute to print
 * @param output File stream to print to
 */
void reg_attr_print (void* reg, FILE* output);

/**
 * @brief Print an instruction list attribute
 * 
 * @param list Pointer to instruction list to print
 * @param output File stream to print to
 */
void insnlist_attr_print (InsnList* list, FILE* output);

/**
 * @brief Copy code attribute from one AST node to another
 * 
 * @param dest Pointer to destination AST node
 * @param src Pointer to source AST node
 */
void ASTNode_copy_code (ASTNode* dest, ASTNode* src);

/**
 * @brief Add/append an instruction to the code attribute (instruction list) for an AST node
 * 
 * @param dest Pointer to destination AST node
 * @param insn Instruction to add
 */
void ASTNode_emit_insn (ASTNode* dest, ILOCInsn* insn);

/**
 * @brief Add a comment to the most recently-emitted instruction for an AST node
 * 
 * If there is no code attribute or if it's an empty list, nothing is done.
 * 
 * @param dest Pointer to destination AST node
 * @param comment Comment to add
 */
void ASTNode_add_comment (ASTNode* dest, const char* comment);

/**
 * @brief Set a temporary result register for an AST node
 * 
 * @param node AST node
 * @param reg Register as an operand
 */
void ASTNode_set_temp_reg (ASTNode* node, Operand reg);

/**
 * @brief Retrieve a temporary result register from an AST node
 * 
 * Aborts with an error message if the given node has no "reg" attribute.
 * 
 * @param node AST node
 * @returns Register operand
 */
Operand ASTNode_get_temp_reg (ASTNode* node);

/**
 * @brief Run ILOC simulator on an ILOC program
 * 
 * If tracing is enabled, the simulator will print the machine state before
 * executing each instruction.
 * 
 * @param program List of ILOC instructions
 * @param print_trace Enable/disable debug tracing
 */
long run_simulator (InsnList* program, bool print_trace);

#endif
