
#include <stdio.h>
#include <stdlib.h> 
#include <stdint.h>
#include <memory.h>

#include "symbol0.h" 

/*
  This module is used for conversion of string names (symbols) to 
  unique numbers. The mapping, number to string is stored in a table. 

  
*/

static uint32_t next_symbol;
static symtab_t symtab; 

int symtab_init() {

  next_symbol = 0; 
  
  symtab.symbols = (char **)malloc(INITIAL_SYMBOL_TABLE_SIZE * sizeof(char *));
  if (!symtab.symbols) return 0;

  memset(symtab.symbols, 0, sizeof(char *) * INITIAL_SYMBOL_TABLE_SIZE);
  symtab.next = NULL; 
  return 1; 
} 

static void symtab_free(symtab_t *s) {

  if (s->next) {
    symtab_free(s->next);
    free(s->next); 
  }
  for (int i = 0; i < INITIAL_SYMBOL_TABLE_SIZE; i ++) {
    if(s->symbols[i]) free(s->symbols[i]);
  }
  free(s->symbols);
}

void symtab_del() {
  symtab_free(&symtab); 
}

static int new_symbol(char *sym, symtab_t *s, uint32_t index){

  if (index >= INITIAL_SYMBOL_TABLE_SIZE) {
    if (!s->next) {
      s->next = (symtab_t*)malloc(sizeof(symtab_t));
      s->next->symbols = (char**)malloc(INITIAL_SYMBOL_TABLE_SIZE * sizeof(char*));
      if (!s->next || !s->next->symbols) {
	return 0;
      }
      memset(s->next->symbols, 0, INITIAL_SYMBOL_TABLE_SIZE * sizeof(char*));
      s->next->next = NULL;
    }
    return (new_symbol(sym, s->next, index - INITIAL_SYMBOL_TABLE_SIZE));
  }

  s->symbols[index] = (char*)malloc(strlen(sym));
  if (!s->symbols[index]) {
    return 0;
  }
  strcpy(s->symbols[index], sym); 

  return 1; 
}

void symtab_print() {
  int i = 0;
  int index = 0; 
  
  int offset = 0;
  symtab_t *s = &symtab; 
  
  while (i < next_symbol) {
    if ( index == INITIAL_SYMBOL_TABLE_SIZE ) {
      index = 0;
      s = s->next;
    }
    printf("%d: %s\n", i, s->symbols[index]);
    
    index++;
    i++; 
  }
}



int symtab_addsym(char *sym, uint32_t *symid) {

  uint32_t sym_index = next_symbol;
  next_symbol++;
  *symid = sym_index;
		  
  if (!new_symbol(sym, &symtab, sym_index)) {
    return 0; 
  }
  return 1;
}


