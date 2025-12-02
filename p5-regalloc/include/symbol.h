/**
 * @file symbol.h
 * @brief Symbols, symbol tables, and static analysis errors
 *
 * This module provides declarations for the symbol table and static analysis
 * framework. The declarations in this file will be will be critical for
 * Project 3 (static analysis) and 4 (code generation).
 */
#ifndef __ANALYSIS_H
#define __ANALYSIS_H

#include "ast.h"
#include "visitor.h"

/**
 * @brief Simple "print" function that prints an attribute value as a Decaf type.
 */
void type_attr_print(void* value, FILE* output);

/**
 * @brief Function that prints a symbol table attribute using DOT formatting
 */
void symtable_attr_print(void* value, FILE* output);

/**
 * @brief Single Decaf symbol.
 * 
 * Might represent a scalar variable, an array, or a formal function parameter.
 */
typedef struct Symbol
{
    /**
     * @brief Kind of symbol (scalar, array, or function)
     */
    enum {
        SCALAR_SYMBOL,
        ARRAY_SYMBOL,
        FUNCTION_SYMBOL
    } symbol_type;

    /**
     * @brief Name of symbol in code
     */
    char name[MAX_ID_LEN];
    
    /**
     * @brief Variable or function return type
     */
    DecafType type;

    /**
     * @brief Length (for array symbols only)
     */
    long length;

    /**
     * @brief List of parameters (for function symbols only)
     */
    ParameterList* parameters;

    /**
     * @brief Memory access location (initialized during code generation)
     */
    enum {
        UNKNOWN_LOC,
        STATIC_VAR,
        STACK_PARAM,
        STACK_LOCAL
    } location;

    /**
     * @brief Memory offset (initialized during code generation)
     */
    long offset;

    /**
     * @brief Next symbol (if stored in a list)
     */
    struct Symbol* next;

} Symbol;

/**
 * @brief Create a new scalar symbol
 * 
 * @param name Name in source code
 * @param type Data type
 */
Symbol* Symbol_new (const char* name, DecafType type);

/**
 * @brief Create a new array symbol
 * 
 * @param name Name in source code
 * @param type Data type
 * @param length Array length
 */
Symbol* Symbol_new_array (const char* name, DecafType type, long length);

/**
 * @brief Create a new function symbol
 * 
 * @param name Name in source code
 * @param return_type Function return type
 * @param parameters List of formal parameter names and data types
 */
Symbol* Symbol_new_function (const char* name, DecafType return_type, ParameterList* parameters);

/**
 * @brief Print simplified string representation
 */
void Symbol_print (Symbol* symbol, FILE* output);

/**
 * @brief Deallocate a symbol
 */
void Symbol_free (Symbol* symbol);

DECL_LIST_TYPE(Symbol, struct Symbol*)

/**
 * @brief Stores symbol info for a single lexical scope.
 * 
 * Symbol tables are generated using a simple AST traversal algorithm, and are
 * used during code generation to look up type and location information for
 * individual symbols.
 */
typedef struct SymbolTable
{
    /**
     * @brief List of symbols defined in the scope associated with this table
     */
    SymbolList* local_symbols;

    /**
     * @brief Link to parent table
     */
    struct SymbolTable* parent;

} SymbolTable;

/**
 * @brief Create a new symbol table with no parent link
 */
SymbolTable* SymbolTable_new (void);

/**
 * @brief Create a new symbol table with a parent link
 * 
 * @param parent Pointer to parent table
 */
SymbolTable* SymbolTable_new_child (SymbolTable* parent);

/**
 * @brief Add a symbol to a table
 * 
 * @param table Symbol table to insert the symbol into
 * @param symbol Symbol to insert
 */
void SymbolTable_insert (SymbolTable* table, Symbol* symbol);

/**
 * @brief Retrieve a symbol from a table
 * 
 * Looks through parent tables if the symbol is not found in the local table.
 * 
 * @param table Symbol table to search
 * @param name Name of symbol to find
 * @returns The @ref Symbol if found, otherwise @c NULL
 */
Symbol* SymbolTable_lookup (SymbolTable* table, const char* name);

/**
 * @brief Deallocate a symbol table
 */
void SymbolTable_free (SymbolTable* table);

/**
 * @brief Look up a symbol in an AST
 *
 * The search has two phases: 1) searching AST nodes for a symbol table as a
 * "symbolTable" attribute and following "parent" attributes as necessary
 * (requires the links as set up by a SetParentVisitor), and 2) searching
 * symbol tables for the given symbol name and following parent pointers as
 * necessary.
 *
 * @param node AST node to begin the search at
 * @param name Name of symbol to find
 * @returns The @ref Symbol if found, otherwise @c NULL
 */
Symbol* lookup_symbol(ASTNode* node, const char* name);

/**
 * @brief Create a new visitor that builds symbol tables
 * 
 * @returns Pointer to visitor structure
 */
NodeVisitor* BuildSymbolTablesVisitor_new (void);

/**
 * @brief Create a new visitor that prints symbol tables
 * 
 * @param output File stream for the print output
 * @returns Pointer to visitor structure
 */
NodeVisitor* PrintSymbolsVisitor_new (FILE* output);

/**
 * @brief Static analysis error structure
 */
typedef struct AnalysisError
{
    char message[MAX_ERROR_LEN];    /**< @brief Error message */
    struct AnalysisError* next;     /**< @brief Next message (if stored in a list) */
} AnalysisError;

DECL_LIST_TYPE(Error, AnalysisError*)

/**
 * @brief Add an error message to a list of errors using @c printf syntax
 */
void ErrorList_printf (ErrorList* list, const char* format, ...);

#endif
