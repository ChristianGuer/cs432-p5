/**
 * @file y86.h
 * @brief Y86 emitter
 */
#ifndef __H_Y86
#define __H_Y86

#include "common.h"
#include "token.h"
#include "iloc.h"

/**
 * @brief Generate Y86 assembly from ILOC
 *
 * Some code courtesy of Kevin Kelly (honors option, Fall 2018)
 * 
 * @param iloc ILOC program as a list of instructions
 * @param output File stream for output
 */
void emit_y86 (InsnList* iloc, FILE* output);

#endif
