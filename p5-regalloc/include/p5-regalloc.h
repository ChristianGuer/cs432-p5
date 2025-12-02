/**
 * @file p5-regalloc.h
 * @brief Interface for Project 5 (Register Allocation)
 */

#ifndef __OTHER_H
#define __OTHER_H

#include "common.h"
#include "iloc.h"

/**
 * @brief Allocate registers for an ILOC program
 * 
 * @param list ILOC program as a list of instructions (the list is modified in place)
 * @param num_physical_registers Maximum number of physical registers to be used
 */
void allocate_registers (InsnList* list, int num_physical_registers);

#endif
