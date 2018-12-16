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

#include <heap.h>
#include <symrepr.h>
#include <lettolam.h>


uint32_t lettolam(uint32_t e){
  
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
    /* TODO: Work happens here */ 
    break;

 
  }

  // Falling through is an error 
  return ENC_SYM(symrepr_eerror());   
}
