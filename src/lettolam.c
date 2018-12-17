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

#include <stdlib.h>
#include <stdio.h>

#include <heap.h>
#include <symrepr.h>
#include <lettolam.h>


uint32_t mklam(uint32_t binds, uint32_t body) {

  if (TYPE_OF(binds) == VAL_TYPE_SYMBOL &&
      DEC_SYM(binds) == symrepr_nil()) {
    return lettolam(body);
  }
  
  if (TYPE_OF(binds) == PTR_TYPE_CONS) {

    uint32_t key = car(car(binds));
    uint32_t val = lettolam(car(cdr(car(binds)))); 

    uint32_t args = cons(key, ENC_SYM(symrepr_nil()));
    
    return  cons(cons(ENC_SYM(symrepr_lambda()),
		      cons(args,
			   cons(mklam(cdr(binds),body),
				cons(val, ENC_SYM(symrepr_nil()))))),
		 cons (val, ENC_SYM(symrepr_nil()))) ;
      
	    
  }
  printf("mklam falling through\n");
  return ENC_SYM(symrepr_eerror()); 

}

uint32_t lettolamlis(uint32_t e){

  if (TYPE_OF(e) == VAL_TYPE_SYMBOL &&
      DEC_SYM(e) == symrepr_nil()) {
    return e; 
  }

  if (TYPE_OF(e) == PTR_TYPE_CONS) {
    return cons(lettolam(car(e)),
		lettolamlis(cdr(e)));
  }

  return ENC_SYM(symrepr_eerror());

}

uint32_t lettolam(uint32_t e){
  uint32_t head;

  switch (TYPE_OF(e)){
  case VAL_TYPE_SYMBOL:
  case VAL_TYPE_I28:
  case VAL_TYPE_U28:
  case VAL_TYPE_CHAR:
  case PTR_TYPE_I32:
  case PTR_TYPE_U32:
  case PTR_TYPE_F32:
  case PTR_TYPE_VEC_I32:
  case PTR_TYPE_VEC_U32:
  case PTR_TYPE_VEC_F32:
  case PTR_TYPE_STRING:
  case PTR_TYPE_REF_I32:
  case PTR_TYPE_REF_U32:
  case PTR_TYPE_REF_FLOAT:
    return e;
    break;
  case PTR_TYPE_CONS:
    head = car(e);

    if (TYPE_OF(head) == VAL_TYPE_SYMBOL) {

      // Special form: QUOTE
      // Dont know if I should convert or just return.
      if (DEC_SYM(head) == symrepr_quote()) {
	return cons(head, lettolam(cdr(e)));
      }

      // Special form: LAMBDA
      if (DEC_SYM(head) == symrepr_lambda()) {
	return cons(head,
		    cons(car(cdr(e)),
			 cons(lettolam (cdr(cdr(e))), ENC_SYM(symrepr_nil()))));
      }

      // Special form: IF
      if (DEC_SYM(head) == symrepr_if()) {
	return cons(head,
		    cons (lettolam (car(cdr(e))),
			  cons (lettolam (car(cdr(cdr(e)))),
				cons (lettolam (car(cdr(cdr(cdr(e))))),
				      ENC_SYM(symrepr_nil())))));
      }

      // Special form: DEFINE
      if (DEC_SYM(head) == symrepr_define()) {
	return cons(head,
		    cons (car(cdr(e)),
			  cons (lettolam (car(cdr(cdr(e)))),
				ENC_SYM(symrepr_nil()))));
      }

      // Special form: LET
      if (DEC_SYM(head) == symrepr_let()) {
	uint32_t binds = car(cdr(e));
	uint32_t body  = car(cdr(cdr(e)));
	return mklam(binds, body); 
      }
        
    }
    return lettolamlis(e); 
    
    break;
  }

  // Falling through is an error
  
  return ENC_SYM(symrepr_eerror());
}
