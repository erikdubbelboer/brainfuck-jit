/******************************** -*- C -*- ****************************
 *
 *	Test ternary->binary op conversion 
 *
 ***********************************************************************/


/***********************************************************************
 *
 * Copyright 2008 Free Software Foundation, Inc.
 * Written by Paolo Bonzini.
 *
 * This file is part of GNU lightning.
 *
 * GNU lightning is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU lightning is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with GNU lightning; see the file COPYING.LESSER; if not, write to the
 * Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 ***********************************************************************/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include "lightning.h"

#ifdef JIT_FPR
static jit_insn codeBuffer[1024];

double
test_double (int a, int b, int c)
{
  double x;
  int ofs;

  jit_set_ip (codeBuffer);
  jit_leaf (2);
  ofs = jit_arg_d ();
  jit_getarg_d (b, ofs);
  ofs = jit_arg_d ();
  jit_getarg_d (c, ofs);
  jit_subr_d (a,b,c);
  jit_movr_d (JIT_FPRET, a);
  jit_ret ();

  jit_flush_code ((char *) codeBuffer, jit_get_ip ().ptr);

#ifdef LIGHTNING_DISASSEMBLE
  disassemble (stderr, (char *) codeBuffer, jit_get_ip ().ptr);
#endif

#ifndef LIGHTNING_CROSS
  x = ((double (*) (double, double)) codeBuffer) (3.0, 2.0);
  printf ("%g %g\n", ((b == c) ? 0.0 : 1.0), x);
#endif

  return x;
}
 
double
test_int (int a, int b, int c)
{
  int x;
  int ofs;

  jit_set_ip (codeBuffer);
  jit_leaf (2);
  ofs = jit_arg_i ();
  jit_getarg_i (b, ofs);
  ofs = jit_arg_i ();
  jit_getarg_i (c, ofs);
  jit_subr_i (a,b,c);
  jit_movr_i (JIT_RET, a);
  jit_ret ();

  jit_flush_code ((char *) codeBuffer, jit_get_ip ().ptr);

#ifdef LIGHTNING_DISASSEMBLE
  disassemble (stderr, (char *) codeBuffer, jit_get_ip ().ptr);
#endif

#ifndef LIGHTNING_CROSS
  x = ((int (*) (int, int)) codeBuffer) (3, 2);
  printf ("%d %d\n", ((b == c) ? 0 : 1), x);
#endif

  return x;
}
 
int
main ()
{
  test_double (JIT_FPR0, JIT_FPR0, JIT_FPR0);
  test_double (JIT_FPR0, JIT_FPR0, JIT_FPR1);
  test_double (JIT_FPR0, JIT_FPR1, JIT_FPR0);
  test_double (JIT_FPR0, JIT_FPR1, JIT_FPR2);

  test_double (JIT_FPR3, JIT_FPR3, JIT_FPR3);
  test_double (JIT_FPR3, JIT_FPR3, JIT_FPR1);
  test_double (JIT_FPR3, JIT_FPR1, JIT_FPR3);
  test_double (JIT_FPR3, JIT_FPR1, JIT_FPR2);

  test_double (JIT_FPR3, JIT_FPR0, JIT_FPR0);
  test_double (JIT_FPR3, JIT_FPR0, JIT_FPR3);
  test_double (JIT_FPR3, JIT_FPR3, JIT_FPR0);

  test_int (JIT_R0, JIT_R0, JIT_R0);
  test_int (JIT_R0, JIT_R0, JIT_R1);
  test_int (JIT_R0, JIT_R1, JIT_R0);
  test_int (JIT_R0, JIT_R1, JIT_R2);

  test_int (JIT_V0, JIT_V0, JIT_V0);
  test_int (JIT_V0, JIT_V0, JIT_R1);
  test_int (JIT_V0, JIT_R1, JIT_V0);
  test_int (JIT_V0, JIT_R1, JIT_R2);

  test_int (JIT_V0, JIT_R0, JIT_R0);
  test_int (JIT_V0, JIT_R0, JIT_V0);
  test_int (JIT_V0, JIT_V0, JIT_R0);

  return 0;
}
#else
int
main()
{       
	  return (77);
} 
#endif
