#include "symbol.h"


void type_attr_print(void* value, FILE* output)
{
    fprintf(output, "%s", DecafType_to_string((DecafType)(long)value));
}

void symtable_attr_print(void* value, FILE* output)
{
    if (((SymbolTable*)value)->local_symbols->size == 0) {
        fprintf(output, "(empty)");
    } else {
        FOR_EACH(Symbol*, sym, ((SymbolTable*)value)->local_symbols) {
            fprintf(output, "\\n");
            fprintf(output, "  ");
            Symbol_print(sym, output);
        }
    }
}

/*
 * Symbol and SymbolTable definitions
 */

Symbol* Symbol_new (const char* name, DecafType type)
{
    Symbol* symbol = (Symbol*)calloc(1, sizeof(Symbol));
    CHECK_MALLOC_PTR(symbol)
    symbol->symbol_type = SCALAR_SYMBOL;
    snprintf(symbol->name, MAX_ID_LEN, "%s", name);
    symbol->type = type;
    symbol->length = 1;
    symbol->parameters = ParameterList_new();
    symbol->location = UNKNOWN_LOC;
    symbol->offset = 0;
    symbol->next = NULL;
    return symbol;
}

Symbol* Symbol_new_array (const char* name, DecafType type, long length)
{
    Symbol* symbol = (Symbol*)calloc(1, sizeof(Symbol));
    CHECK_MALLOC_PTR(symbol)
    symbol->symbol_type = ARRAY_SYMBOL;
    snprintf(symbol->name, MAX_ID_LEN, "%s", name);
    symbol->type = type;
    symbol->length = length;
    symbol->parameters = ParameterList_new();
    symbol->location = UNKNOWN_LOC;
    symbol->offset = 0;
    symbol->next = NULL;
    return symbol;
}

Symbol* Symbol_new_function (const char* name, DecafType return_type, ParameterList* parameters)
{
    Symbol* symbol = (Symbol*)calloc(1, sizeof(Symbol));
    CHECK_MALLOC_PTR(symbol)
    symbol->symbol_type = FUNCTION_SYMBOL;
    snprintf(symbol->name, MAX_ID_LEN, "%s", name);
    symbol->type = return_type;
    symbol->length = 1;
    symbol->parameters = ParameterList_new();
    FOR_EACH(Parameter*, p, parameters) {
        ParameterList_add_new(symbol->parameters, p->name, p->type);
    }
    symbol->location = UNKNOWN_LOC;
    symbol->offset = 0;
    symbol->next = NULL;
    return symbol;
}

void Symbol_print (Symbol* symbol, FILE* output)
{
    switch (symbol->symbol_type)
    {
        case SCALAR_SYMBOL:
            fprintf(output, "%s : %s", symbol->name, DecafType_to_string(symbol->type));
            break;

        case ARRAY_SYMBOL:
            fprintf(output, "%s : %s [%ld]", symbol->name, DecafType_to_string(symbol->type),
                                   symbol->length);
            break;

        case FUNCTION_SYMBOL:
            fprintf(output, "%s : (", symbol->name);
            bool first = true;
            FOR_EACH (Parameter*, p, symbol->parameters) {
                if (first) { first = false; } else { fprintf(output, ", "); }
                fprintf(output, "%s", DecafType_to_string(p->type));
            }
            fprintf(output, ") -> %s", DecafType_to_string(symbol->type));
            break;
    }
    /* if symbol->location has been set, allocation has happened, so print that info too */
    switch (symbol->location)
    {
        case STATIC_VAR:    fprintf(output, " {static offset=%ld}", symbol->offset);    break;
        case STACK_PARAM:
        case STACK_LOCAL:   fprintf(output, " {stack offset=%ld}", symbol->offset);     break;
        default: break;
    }
}

void Symbol_free (Symbol* symbol)
{
    ParameterList_free(symbol->parameters);
    free(symbol);
}

DEF_LIST_IMPL(Symbol, Symbol*, Symbol_free)

SymbolTable* SymbolTable_new (void)
{
    SymbolTable* table = (SymbolTable*)calloc(1, sizeof(SymbolTable));
    CHECK_MALLOC_PTR(table)
    table->local_symbols = SymbolList_new();
    table->parent = NULL;
    return table;
}

SymbolTable* SymbolTable_new_child (SymbolTable* parent)
{
    SymbolTable* table = SymbolTable_new();
    table->parent = parent;
    return table;
}

void SymbolTable_insert (SymbolTable* table, Symbol* symbol)
{
    SymbolList_add(table->local_symbols, symbol);
}

Symbol* SymbolTable_lookup (SymbolTable* table, const char* name)
{
    FOR_EACH(Symbol*, sym, table->local_symbols) {
        if (strncmp(name, sym->name, MAX_ID_LEN) == 0) {
            return sym;
        }
    }
    if (table->parent != NULL) {
        return SymbolTable_lookup(table->parent, name);
    }
    return NULL;
}

void SymbolTable_free (SymbolTable* table)
{
    SymbolList_free(table->local_symbols);
    free(table);
}

Symbol* lookup_symbol(ASTNode* node, const char* name)
{
    /* phase 1: traverse up the tree until we find a symbol table or reach the root */
    while (node != NULL && !ASTNode_has_attribute(node, "symbolTable")) {
        node = (ASTNode*)ASTNode_get_attribute(node, "parent");
    }
    /* phase 2: if we found a symbol table, look up the symbol in a recursive
     * search managed by @ref SymbolTable_lookup */
    Symbol* symbol = NULL;
    if (node != NULL) {
        symbol = SymbolTable_lookup((SymbolTable*)ASTNode_get_attribute(node, "symbolTable"), name);
    }
    return symbol;
}

/*
 * SymbolTable construction (AST visitor)
 */

Symbol* create_print_symbol(const char* name, DecafType type)
{
    ParameterList* params = ParameterList_new();
    ParameterList_add_new(params, "value", type);
    Symbol* symbol = Symbol_new_function(name, VOID, params);
    ParameterList_free(params);
    return symbol;
}

void BuildSymbolTablesVisitor_previsit_program (NodeVisitor* visitor, ASTNode* node)
{
    /* new root table */
    SymbolTable* table = SymbolTable_new();

    /* add to AST as an attribute */
    ASTNode_set_printable_attribute(node, "symbolTable", table, symtable_attr_print, (Destructor)SymbolTable_free);

    /* initialize stack */
    visitor->data = table;

    /* add symbols for built-in functions */
    SymbolTable_insert(table, create_print_symbol("print_int",  INT));
    SymbolTable_insert(table, create_print_symbol("print_bool", BOOL));
    SymbolTable_insert(table, create_print_symbol("print_str",  STR));

    /* add symbols for user-defined functions (global variables will be handled in vardecl visitor) */
    FOR_EACH (ASTNode*, func, node->program.functions) {
        Symbol* new_symbol = Symbol_new_function(func->funcdecl.name, func->funcdecl.return_type,
                                                 func->funcdecl.parameters);
        SymbolTable_insert(table, new_symbol);
    }
}

void BuildSymbolTablesVisitor_previsit_funcdecl (NodeVisitor* visitor, ASTNode* node)
{
    /* new child table w/ a parent pointer to the table on top of the stack */
    SymbolTable* table = SymbolTable_new_child((SymbolTable*)visitor->data);
    ASTNode_set_printable_attribute(node, "symbolTable", table, symtable_attr_print, (Destructor)SymbolTable_free);
    visitor->data = table;  /* push onto stack (parent pointer acts as 'next') */

    /* add symbols for parameters (local variables will be handled in vardecl visitor) */
    FOR_EACH (Parameter*, p, node->funcdecl.parameters) {
        SymbolTable_insert(table, Symbol_new(p->name, p->type));
    }
}

void BuildSymbolTablesVisitor_previsit_block (NodeVisitor* visitor, ASTNode* node)
{
    /* new child table w/ a parent pointer to the table on top of the stack */
    SymbolTable* table = SymbolTable_new_child((SymbolTable*)visitor->data);

    /* add to AST as an attribute */
    ASTNode_set_printable_attribute(node, "symbolTable", table, symtable_attr_print, (Destructor)SymbolTable_free);

    /* push onto stack (parent pointer acts as 'next') */
    visitor->data = table;
}

void BuildSymbolTablesVisitor_visit_vardecl (NodeVisitor* visitor, ASTNode* node)
{
    /* create and add new symbol to the current/top symbol table */
    SymbolTable* current_table = (SymbolTable*) visitor->data;
    Symbol* new_symbol = NULL;
    if (node->vardecl.is_array) {
        new_symbol = Symbol_new_array(node->vardecl.name, node->vardecl.type,
                                      node->vardecl.array_length);
    } else {
        new_symbol = Symbol_new(node->vardecl.name, node->vardecl.type);
    }
    SymbolTable_insert(current_table, new_symbol);
}

void BuildSymbolTablesVisitor_postvisit (NodeVisitor* visitor, ASTNode* node)
{
    visitor->data = ((SymbolTable*)visitor->data)->parent;  /* pop stack */
}

NodeVisitor* BuildSymbolTablesVisitor_new (void)
{
    NodeVisitor* v = NodeVisitor_new();
    /* no code needed here, but we'll use the "data" pointer as a pointer to
     * the top of a SymbolTable stack; it will allow us to set up the "sheaf"
     * of symbol tables using parent pointers and also we'll always have a
     * readily available reference to the "current" symbol table for adding
     * new symbols when we get to variable declarations */
    v->previsit_program   = BuildSymbolTablesVisitor_previsit_program;
    v->postvisit_program  = BuildSymbolTablesVisitor_postvisit;
    v->previsit_funcdecl  = BuildSymbolTablesVisitor_previsit_funcdecl;
    v->postvisit_funcdecl = BuildSymbolTablesVisitor_postvisit;
    v->previsit_block     = BuildSymbolTablesVisitor_previsit_block;
    v->postvisit_block    = BuildSymbolTablesVisitor_postvisit;
    v->previsit_vardecl   = BuildSymbolTablesVisitor_visit_vardecl;
    return v;
}

/*
 * SymbolTable debug output (AST visitor)
 */

#define OUTFILE ((FILE*)visitor->data)
#define PRINT_INDENT    long depth = (long)ASTNode_get_attribute(node, "depth"); \
                        for (long i = 0; i < depth; i++) { \
                            fprintf(OUTFILE, "  "); \
                        }

void PrintVisitor_visit_program (NodeVisitor* visitor, ASTNode* node);
void PrintVisitor_visit_funcdecl (NodeVisitor* visitor, ASTNode* node);
void PrintVisitor_visit_block (NodeVisitor* visitor, ASTNode* node);

void print_symbol_table (NodeVisitor* visitor, ASTNode* node)
{
    /* print symbol table if present */
    if (ASTNode_has_attribute(node, "symbolTable")) {
        PRINT_INDENT
        fprintf(OUTFILE, "SYM TABLE:\n");
        SymbolTable* table = (SymbolTable*)ASTNode_get_attribute(node, "symbolTable");
        FOR_EACH(Symbol*, sym, table->local_symbols) {
            PRINT_INDENT
            fprintf(OUTFILE, " ");
            Symbol_print(sym, OUTFILE);
            fprintf(OUTFILE, "\n");
        }
    }
    fprintf(OUTFILE, "\n");
}

void PrintSymbolsVisitor_visit_program (NodeVisitor* visitor, ASTNode* node)
{
    /* print regular output */
    PrintVisitor_visit_program(visitor, node);

    print_symbol_table(visitor, node);
}

void PrintSymbolsVisitor_visit_funcdecl (NodeVisitor* visitor, ASTNode* node)
{
    /* print regular output */
    PrintVisitor_visit_funcdecl(visitor, node);

    print_symbol_table(visitor, node);
}

void PrintSymbolsVisitor_visit_block (NodeVisitor* visitor, ASTNode* node)
{
    /* print regular output */
    PrintVisitor_visit_block(visitor, node);

    print_symbol_table(visitor, node);
}

NodeVisitor* PrintSymbolsVisitor_new (FILE* output)
{
    NodeVisitor* v = NodeVisitor_new();
    /* use "data" field to store FILE* output pointer */
    v->data = (void*)output;
    v->previsit_program  = PrintSymbolsVisitor_visit_program;
    v->previsit_funcdecl = PrintSymbolsVisitor_visit_funcdecl;
    v->previsit_block    = PrintSymbolsVisitor_visit_block;
    return v;
}

/*
 * static analysis definitions
 */

DEF_LIST_IMPL(Error, AnalysisError*, free)

void ErrorList_printf (ErrorList* list, const char* format, ...)
{
    AnalysisError* err = (AnalysisError*)calloc(1, sizeof(AnalysisError));
    CHECK_MALLOC_PTR(err)
    va_list args;
    va_start(args, format);
    vsnprintf(err->message, MAX_ERROR_LEN, format, args);
    va_end(args);
    ErrorList_add(list, err);
}
