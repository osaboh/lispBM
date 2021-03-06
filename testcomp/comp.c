/*
    Copyright 2018 Joel Svensson	svenssonjoel@yahoo.se

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>

#include "heap.h"
#include "symrepr.h"
#include "builtin.h"
#include "tokpar.h"
#include "bytecode.h"
#include "print.h"
#include "stack.h"

int main(int argc, char **argv) {
  int res = 0;
  res = symrepr_init();
  if (res)
    printf("Symrepr initialized.\n");
  else {
    printf("Error initializing symrepr!\n");
    return 0;
  }

  int heap_size = 8 * 1024 * 1024;
  res = heap_init(heap_size);
  if (res)
    printf("Heap initialized. Heap size: %f MiB. Free cons cells: %d\n", heap_size_bytes() / 1024.0 / 1024.0, heap_num_free());
  else {
    printf("Error initializing heap!\n");
    return 0;
  }

  res = builtin_init();
  if (res)
    printf("Built in functions initialized.\n");
  else {
    printf("Error initializing built in functions.\n");
    return 0;
  }

  stack *s = stack_init(100,true);
  VALUE t;
  bytecode_t *bc;
  int err;
  VALUE result;

  //bytecode_create(&bc, 1000);

  printf("PARSING\n");
  t = tokpar_parse("(+ 1 2)");
  printf("COMPILING: "); simple_print(car(t)); printf("\n");
  bc = bytecode_compile(s, car(t), &err);

  if (!bc) {
    printf("Error: %d - %s\n", err, bytecode_get_error(err));
  } else {
    result = bytecode_eval(s, bc);

    for (unsigned int i = 0; i < bc->code_size +1; i ++) {
      printf("%u\n", bc->code[i]);
    }
    printf("Result: "); simple_print(result); printf("\n");
  }


  
  printf("PARSING\n");
  t = tokpar_parse("(+ (+ 1 2) (+ 1 2))");
  printf("COMPILING: "); simple_print(car(t)); printf("\n");
  bc = bytecode_compile(s, car(t), &err);

  if (!bc) {
    printf("Error: %d - %s\n", err, bytecode_get_error(err));
  } else {
    result = bytecode_eval(s, bc);

    for (unsigned int i = 0; i < bc->code_size +1; i ++) {
      printf("%u\n", bc->code[i]);
    }
    printf("Result: "); simple_print(result); printf("\n");
  }


  printf("PARSING\n");
  t = tokpar_parse("(closure (x) (+ x 1) ())");
  printf("COMPILING: "); simple_print(car(t)); printf("\n");
  bc = bytecode_compile(s, car(t), &err);
  if (!bc ) {
    printf("Error: %d - %s\n", err, bytecode_get_error(err));
    return 0;
  } else {
    result = bytecode_eval(s, bc);
  }
  /* for (unsigned int i = 0; i < bc.code_size +1; i ++) { */

  /*   printf("%u\n", bc.code[i]); */
  /* } */

  /* printf("Result: "); simple_print(result); printf("\n"); */


  symrepr_del();
  heap_del();

  return 1;
}
