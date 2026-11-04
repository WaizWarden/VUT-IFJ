/**
 * @file    codegen.h
 * @author  xoramst00
 * @brief   Generator definition of final code IFJcode26
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdbool.h>
#include <stdint.h>

/// @brief Increments global id counter
/// @return unique label id
uint32_t cg_next_label_id();

/// @brief print 64bit int to stdout
/// @param x intiger to print
void cg_print_int(int64_t val);

void cg_print_double(double val);

void cg_print_bool(bool val);

void cg_print_unit(void);

void cg_print_string(char *str);

void cg_print_var(const char *name, uint32_t uid);

#endif