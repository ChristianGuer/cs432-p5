/**
 * @file public.c
 * @brief Public test cases (and location for new tests)
 * 
 * This file provides a few basic sanity test cases and a location to add new tests.
 */

#include "testsuite.h"

#ifndef SKIP_IN_DOXYGEN

TEST_EXPRESSION(D_expr_add,    5, "2+3")
TEST_EXPRESSION(D_expr_mul,    6, "2*3")

TEST_EXPRESSION_WITH_REGS(D_expr_5regs,   5,  9, "2+3+4")
TEST_EXPRESSION_WITH_REGS(D_expr_13regs, 13, 35, "2+3+4+5+6+7+8")

TEST_EXPRESSION(C_expr_2regs, 10, "1+2+3+4")
TEST_EXPRESSION(C_expr_3regs, 10, "(1+2)+(3+4)")

TEST_PROGRAM(C_assign, 14, 
        "def int main() { "
        "  int a; a = 2 + 3 * 4; "
        "  return a; }")

TEST_PROGRAM(C_assign_multiple, 40, 
        "def int main() { "
        "  int a;"
        "  a = 2 + 3 * 4; " // 14
        "  a = a - 4 / 2; " // 12
        "  a = a * 3 + 4; " // 40
        "  return a; }")

TEST_PROGRAM(C_conditional, 3, 
        "def int main() { "
        "  if (true) { return 2+1; } "
        "  else { return 3+1; } }")

TEST_PROGRAM(C_while, 10, 
        "def int main() { "
        "  int a; a = 0; "
        "  while (a < 10) { a = a + 1; } "
        "  return a; }")

TEST_PROGRAM(B_func_call, 5, 
        "def int add(int a, int b) { return a + b; } "
        "def int main() { return add(2,3); }")

TEST_PROGRAM(B_spilled_regs, 72, 
        "def int main() { "
        "  return (((1+2)+(3+4))+((5+6)+(7+8)))+"
        "         (((1+2)+(3+4))+((5+6)+(7+8))); }")

#endif

/**
 * @brief Register all test cases
 * 
 * @param s Test suite to which the tests should be added
 */
void public_tests (Suite *s)
{
    TCase *tc = tcase_create ("Public");

    TEST(D_expr_add);
    TEST(D_expr_mul);
    TEST(D_expr_5regs);
    TEST(D_expr_13regs);

    TEST(C_expr_2regs);
    TEST(C_expr_3regs);
    TEST(C_assign);
    TEST(C_assign_multiple);
    TEST(C_conditional);
    TEST(C_while);

    TEST(B_func_call);
    TEST(B_spilled_regs);

    suite_add_tcase (s, tc);
}

