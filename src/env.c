
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

#include <stdio.h>

#include "symrepr.h"
#include "heap.h"
#include "print.h"
#include "typedefs.h"

// Copies just the skeleton structure of an environment
// The new "copy" will have pointers to the original key-val bindings.
VALUE env_copy_shallow(VALUE env) {

  VALUE res = enc_sym(symrepr_nil());
  VALUE curr = env;

  while (type_of(curr) == PTR_TYPE_CONS) {
    VALUE key = car(car(curr));
    if (dec_sym(key) != symrepr_nil()) {
      res = cons(car(curr), res);

      // Check for "out of memory"
      if (type_of(res) == VAL_TYPE_SYMBOL &&
	  dec_sym(res) == symrepr_merror()) {
	return res;
      }
    }
    curr = cdr(curr);
  }
  return  res;
}

VALUE env_lookup(VALUE sym, VALUE env) {
  VALUE curr = env;

  if(dec_sym(sym) == symrepr_nil()) {
    return sym;
  }

  while (type_of(curr) == PTR_TYPE_CONS) {
    if (car(car(curr)) == sym) {
      return cdr(car(curr));
    }
    curr = cdr(curr);
  }
  return enc_sym(symrepr_not_found());
}

VALUE env_set(VALUE env, VALUE key, VALUE val) {
  
  VALUE curr = env;
  VALUE new_env;
  VALUE keyval;
  
  while(type_of(curr) == PTR_TYPE_CONS) {
    if (car(car(curr)) == key) {
      set_cdr(car(curr),val);
      return env;
    }
    curr = cdr(curr);
  }

  keyval = cons(key,val);
  if (type_of(keyval) == VAL_TYPE_SYMBOL) {
    return keyval;
  }

  new_env = cons(keyval, env);
  if (type_of(new_env) == VAL_TYPE_SYMBOL) {
    return keyval;
  }

  return new_env;
}


VALUE env_modify_binding(VALUE env, VALUE key, VALUE val) {

  VALUE curr = env;

  while (type_of(curr) == PTR_TYPE_CONS) {
    if (car(car(curr)) == key) {
      set_cdr(car(curr), val);
      return env;
    }
    curr = cdr(curr);

  }
  return enc_sym(symrepr_not_found());
}


VALUE env_build_params_args(VALUE params,
			    VALUE args,
			    VALUE env0) {
  VALUE curr_param = params;
  VALUE curr_arg = args;

  // TODO: This should be checked outside of this function.
  //
  if (length(params) != length(args)) { // programmer error
    return enc_sym(symrepr_fatal_error());
  }

  VALUE env = env0;
  while (type_of(curr_param) == PTR_TYPE_CONS) {

    VALUE entry = cons(car(curr_param), car(curr_arg));
    if (type_of(entry) == VAL_TYPE_SYMBOL &&
	dec_sym(entry) == symrepr_merror())
      return enc_sym(symrepr_merror());

    env = cons(entry,env);

    if (type_of(env) == VAL_TYPE_SYMBOL &&
	dec_sym(env) == symrepr_merror())
      return enc_sym(symrepr_merror());

    curr_param = cdr(curr_param);
    curr_arg   = cdr(curr_arg);
  }
  return env;
}
