/**
 * @file p4-codegen.h
 * @brief Interface for Project 4 (Code Generation)
 */

#ifndef __P4_CODEGEN_H
#define __P4_CODEGEN_H

#include "common.h"
#include "ast.h"
#include "visitor.h"
#include "symbol.h"
#include "iloc.h"

/**
 * @brief Convert an AST into linear ILOC code
 * 
 * @param tree Root of AST
 * @returns List of ILOC instructions
 */
InsnList* generate_code (ASTNode* tree);

#endif
