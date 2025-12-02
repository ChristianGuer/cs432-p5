/**
 * @file main.c
 * @brief Compiler driver
 */

#include "p1-lexer.h"
#include "p2-parser.h"
#include "p3-analysis.h"
#include "p4-codegen.h"
#include "p5-regalloc.h"

#include "y86.h"

/**
 * @brief Error message buffer
 */
char decaf_error_msg[MAX_ERROR_LEN];

/**
 * @brief Data structure used by @c setjmp / @c longjmp for exception handling
 */
jmp_buf decaf_error;

/**
 * @brief Throw an exception with an error message using printf syntax
 *
 * This function is declared in common.h but must be defined here in main.c
 * because that's where the @c jmp_buf declaration is.
 */
void Error_throw_printf(const char *format, ...)
{
    /* delegate to vsnprintf for error message formatting */
    va_list args;
    va_start(args, format);
    vsnprintf(decaf_error_msg, MAX_ERROR_LEN, format, args);
    va_end(args);

    /* jump to location saved by setjmp */
    longjmp(decaf_error, 1);
}

/**
 * @brief Read all text data from a file
 *
 * @param filename Name of file to read
 * @param text String buffer destination (must be #MAX_FILE_SIZE characters long)
 * @returns True if and only if the file read was successful.
 */
bool read_file(const char *filename, char *text)
{
    FILE *input = fopen(filename, "r");
    if (input == NULL)
    {
        return false;
    }
    size_t nchars = 0;
    char *p = text;
    int c;
    while (nchars < MAX_FILE_SIZE && (c = fgetc(input)) != EOF)
    {
        *p++ = (char)c;
        nchars++;
    }
    *p = '\0';
    fclose(input);
    return true;
}

/**
 * @brief Compiler entry point
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @returns @c EXIT_SUCCESS if the compilation succeeds and @c EXIT_FAILURE
 * otherwise
 */
int main(int argc, char **argv)
{
    /* check for filename */
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <decaf-filename>\n", argv[0]);
        return EXIT_FAILURE;
    }
    char *filename = argv[argc - 1];

    /* read file */
    char text[MAX_FILE_SIZE];
    if (!read_file(filename, text))
    {
        fprintf(stderr, "Could not read file: %s", filename);
        exit(EXIT_FAILURE);
    }

    /* FRONT END */

    TokenQueue *tokens = NULL;
    ASTNode *tree = NULL;

    /* fatal errors are possible in the front end, so check for them */
    if (setjmp(decaf_error) == 0)
    {

        /* PROJECT 1: lexer */
        tokens = lex(text);

        /* PROJECT 2: parser */
        tree = parse(tokens);

        /* clean up tokens (no longer needed) */
        TokenQueue_free(tokens);
        tokens = NULL;
    }
    else
    {

        /* handle fatal error: print message and clean up */
        fprintf(stderr, "%s", decaf_error_msg);
        if (tokens != NULL)
            TokenQueue_free(tokens);
        if (tree != NULL)
            ASTNode_free(tree);
        exit(EXIT_FAILURE);
    }

    /* set up parent links and calculate node depths */
    NodeVisitor_traverse_and_free(SetParentVisitor_new(), tree);
    NodeVisitor_traverse_and_free(CalcDepthVisitor_new(), tree);

    /* MIDDLE END */

    /* build symbol tables */
    NodeVisitor_traverse_and_free(BuildSymbolTablesVisitor_new(), tree);

    /* PROJECT 3: analysis */
    ErrorList *errors = analyze(tree);

    /* print analysis errors */
    FOR_EACH(AnalysisError *, err, errors)
    {
        printf("%s\n", err->message);
    }

    /* abort if analysis has reported errors */
    if (!ErrorList_is_empty(errors))
    {
        ASTNode_free(tree);
        ErrorList_free(errors);
        exit(EXIT_FAILURE);
    }

    /* clean up error list */
    ErrorList_free(errors);
    errors = NULL;

    /* BACK END */

    /* run symbol allocation */
    NodeVisitor_traverse_and_free(AllocateSymbolsVisitor_new(), tree);

    /* PROJECT 4: code gen */
    InsnList *iloc = generate_code(tree);

    /* clean up syntax tree (no longer needed) */
    ASTNode_free(tree);
    tree = NULL;

    /* PROJECT 5: register allocation */
    allocate_registers(iloc, 4);

    /* print ILOC */
    InsnList_print(iloc, stdout);

    /* run program (change 'true' to 'false' to disable trace output) */
    int return_value = run_simulator(iloc, true);
    printf("RETURN VALUE = %d\n", return_value);

    /* enable this to generate Y86 (requires a functional P5 solution first) */
    /*
     *FILE* y86_file = fopen("program.ys", "w");
     *if (y86_file != NULL) {
     *    emit_y86(iloc, y86_file);
     *    fclose(y86_file);
     *}
     */

    /* clean up ILOC code (no longer needed) */
    InsnList_free(iloc);
    iloc = NULL;

    return EXIT_SUCCESS;
}
