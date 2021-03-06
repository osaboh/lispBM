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

#ifndef SYMTAB_H_
#define SYMTAB_H_

#include <stdint.h>
#include <stdbool.h>

#include "typedefs.h"

// Default and fixed symbol ids
#define DEF_REPR_NIL           0x0FFFF
#define DEF_REPR_QUOTE         0x1FFFF
#define DEF_REPR_TRUE          0x2FFFF
#define DEF_REPR_IF            0x3FFFF
#define DEF_REPR_LAMBDA        0x4FFFF
#define DEF_REPR_CLOSURE       0x5FFFF
#define DEF_REPR_LET           0x6FFFF
#define DEF_REPR_RERROR        0x7FFFF   /* READ ERROR */
#define DEF_REPR_TERROR        0x8FFFF   /* TYPE ERROR */
#define DEF_REPR_EERROR        0x9FFFF   /* EVAL ERROR */
#define DEF_REPR_MERROR        0xAFFFF
#define DEF_REPR_DIVZERO       0xBFFFF
#define DEF_REPR_FATAL_ERROR   0xCFFFF   /* Runtime system is corrupt */
#define DEF_REPR_DEFINE        0xDFFFF
#define DEF_REPR_PROGN         0xEFFFF
//#define DEF_REPR_BACKQUOTE     0xFFFFF
#define DEF_REPR_COMMA         0x10FFFF
#define DEF_REPR_COMMAAT       0x11FFFF

// Special symbol ids
#define DEF_REPR_ARRAY_TYPE     0x20FFFF
#define DEF_REPR_BOXED_I_TYPE   0x21FFFF
#define DEF_REPR_BOXED_U_TYPE   0x22FFFF
#define DEF_REPR_BOXED_F_TYPE   0x23FFFF
#define DEF_REPR_REF_TYPE       0x24FFFF
#define DEF_REPR_RECOVERED      0x25FFFF
#define DEF_REPR_BYTECODE_TYPE  0x26FFFF
#define DEF_REPR_NONSENSE       0x27FFFF
#define DEF_REPR_NOT_FOUND      0x28FFFF

// Type identifying symbols
#define DEF_REPR_TYPE_LIST      0x29FFFF
#define DEF_REPR_TYPE_I28       0x2AFFFF
#define DEF_REPR_TYPE_U28       0x2BFFFF
#define DEF_REPR_TYPE_FLOAT     0x2CFFFF
#define DEF_REPR_TYPE_I32       0x2DFFFF
#define DEF_REPR_TYPE_U32       0x2EFFFF
#define DEF_REPR_TYPE_ARRAY     0x2FFFFF
#define DEF_REPR_TYPE_SYMBOL    0x30FFFF
#define DEF_REPR_TYPE_CHAR      0x31FFFF

// Fundamental Operations
#define SYM_ADD                 0x100FFFF
#define SYM_SUB                 0x101FFFF
#define SYM_MUL                 0x102FFFF
#define SYM_DIV                 0x103FFFF
#define SYM_MOD                 0x104FFFF
#define SYM_EQ                  0x105FFFF
#define SYM_NUMEQ               0x106FFFF
#define SYM_LT                  0x107FFFF
#define SYM_GT                  0x108FFFF
#define SYM_EVAL                0x109FFFF

#define SYM_AND                 0x110FFFF
#define SYM_OR                  0x111FFFF
#define SYM_NOT                 0x112FFFF

#define SYM_CONS                0x120FFFF
#define SYM_CAR                 0x121FFFF
#define SYM_CDR                 0x122FFFF
#define SYM_LIST                0x123FFFF
#define SYM_APPEND              0x124FFFF

#define SYM_ARRAY_READ          0x130FFFF
#define SYM_ARRAY_WRITE         0x131FFFF
#define SYM_ARRAY_CREATE        0x132FFFF
#define SYM_TYPE_OF             0x200FFFF

#define SYMBOL_MAX              0xFFFFFFF

extern int symrepr_addsym(char *, UINT*);
extern bool symrepr_init(void);
extern int symrepr_lookup(char *, UINT*);
extern char* symrepr_lookup_name(UINT);
extern void symrepr_del(void);

static inline UINT symrepr_nil(void)         { return DEF_REPR_NIL; }
static inline UINT symrepr_quote(void)       { return DEF_REPR_QUOTE; }
static inline UINT symrepr_true(void)        { return DEF_REPR_TRUE; }
static inline UINT symrepr_if(void)          { return DEF_REPR_IF; }
static inline UINT symrepr_lambda(void)      { return DEF_REPR_LAMBDA; }
static inline UINT symrepr_closure(void)     { return DEF_REPR_CLOSURE; }
static inline UINT symrepr_let(void)         { return DEF_REPR_LET; }
static inline UINT symrepr_define(void)      { return DEF_REPR_DEFINE; }
static inline UINT symrepr_progn(void)       { return DEF_REPR_PROGN; }
//static inline UINT symrepr_backquote(void)   { return DEF_REPR_BACKQUOTE; }
static inline UINT symrepr_comma(void)       { return DEF_REPR_COMMA; }
static inline UINT symrepr_commaat(void)     { return DEF_REPR_COMMAAT; }

static inline UINT symrepr_cons(void)        { return SYM_CONS; }
static inline UINT symrepr_list(void)        { return SYM_LIST; }
static inline UINT symrepr_append(void)      { return SYM_APPEND; }
static inline UINT symrepr_and(void)         { return SYM_AND; }
static inline UINT symrepr_or(void)          { return SYM_OR; }

static inline UINT symrepr_rerror(void)      { return DEF_REPR_RERROR; }
static inline UINT symrepr_terror(void)      { return DEF_REPR_TERROR; }
static inline UINT symrepr_eerror(void)      { return DEF_REPR_EERROR; }
static inline UINT symrepr_merror(void)      { return DEF_REPR_MERROR; }
static inline UINT symrepr_divzero(void)     { return DEF_REPR_DIVZERO; }
static inline UINT symrepr_fatal_error(void) { return DEF_REPR_FATAL_ERROR; }

static inline UINT symrepr_nonsense(void)    { return DEF_REPR_NONSENSE; }
static inline UINT symrepr_not_found(void)   { return DEF_REPR_NOT_FOUND; }

static inline UINT symrepr_type_list(void)   {return DEF_REPR_TYPE_LIST; }
static inline UINT symrepr_type_i28(void)    {return DEF_REPR_TYPE_I28; }       
static inline UINT symrepr_type_u28(void)    {return DEF_REPR_TYPE_U28; }       
static inline UINT symrepr_type_float(void)  {return DEF_REPR_TYPE_FLOAT; }     
static inline UINT symrepr_type_i32(void)    {return DEF_REPR_TYPE_I32; }       
static inline UINT symrepr_type_u32(void)    {return DEF_REPR_TYPE_U32; }       
static inline UINT symrepr_type_array(void)  {return DEF_REPR_TYPE_ARRAY; }     
static inline UINT symrepr_type_symbol(void) {return DEF_REPR_TYPE_SYMBOL; }
static inline UINT symrepr_type_char(void)   {return DEF_REPR_TYPE_CHAR; }


static inline bool symrepr_is_error(UINT symrep){
  return (symrep == DEF_REPR_RERROR ||
	  symrep == DEF_REPR_TERROR ||
	  symrep == DEF_REPR_RERROR ||
	  symrep == DEF_REPR_MERROR ||
	  symrep == DEF_REPR_FATAL_ERROR);
}

#endif
