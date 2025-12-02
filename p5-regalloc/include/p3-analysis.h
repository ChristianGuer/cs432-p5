/**
 * @file p3-analysis.h
 * @brief Interface for Project 3 (Analysis)
 */

#ifndef __P3_ANALYSIS_H
#define __P3_ANALYSIS_H

#include "common.h"
#include "token.h"
#include "ast.h"
#include "visitor.h"
#include "symbol.h"

/**
 * @brief Perform static analysis on an AST and return a list of errors
 * 
 * @param tree Root of AST
 * @returns List of static analysis errors found (the list should be empty if
 * no errors were found)
 */
ErrorList* analyze (ASTNode* tree);

#endif
