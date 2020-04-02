/*
    Copyright 2018, 2020 Joel Svensson	svenssonjoel@yahoo.se

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

#include "symrepr.h"
#include "heap.h"
#include "env.h"
#include "bytecode.h"
#include "eval_cps.h"
#include "stack.h"
#include "fundamental.h"
#include "extensions.h"
#include "typedefs.h"
#ifdef VISUALIZE_HEAP
#include "heap_vis.h"
#endif

#define DONE              1
#define SET_GLOBAL_ENV    2
#define BIND_TO_KEY_REST  3
#define IF                4
#define PROGN_REST        5
#define APPLICATION       6
#define APPLICATION_ARGS  7
#define AND               8
#define OR                9

#define FATAL_ON_FAIL(done, x)  if (!(x)) { (done)=true; ctx->r = enc_sym(symrepr_fatal_error()); return ; }
#define FATAL_ON_FAIL_R(done, x)  if (!(x)) { (done)=true; ctx->r = enc_sym(symrepr_fatal_error()); return ctx->r; }
#define FOF(x)  if  (!(x)) { ctx_running->done = true; ctx_running->r = enc_sym(symrepr_fatal_error()); ctx_running = NULL; break;}
#define ERROR printf("Line: %d\n", __LINE__);
#define DEFAULT_SLEEP_US  1000

static VALUE eval_cps_global_env;
static VALUE NIL;
static VALUE NONSENSE;

static bool     eval_running = false;
static uint32_t next_ctx_id = 1;

/* Callbacks and task queue */
static eval_context_t *ctx_queue = NULL;
static eval_context_t *ctx_queue_last = NULL;
static eval_context_t *ctx_done = NULL;
static eval_context_t *ctx_running = NULL;

static void (*usleep_callback)(uint32_t) = NULL;
static uint32_t (*timestamp_us_callback)(void) = NULL;
static void (*ctx_done_callback)(eval_context_t *) = NULL;

void eval_cps_set_usleep_callback(void (*fptr)(uint32_t)) {
  usleep_callback = fptr;
}

void eval_cps_set_timestamp_us_callback(uint32_t (*fptr)(void)) {
  timestamp_us_callback = fptr;
}

void eval_cps_set_ctx_done_callback(void (*fptr)(eval_context_t *)) {
  ctx_done_callback = fptr;
}

void enqueue_ctx(eval_context_t *ctx) {

  if (ctx_queue_last == NULL) {
    ctx->prev = NULL;
    ctx->next = NULL;
    ctx_queue = ctx;
    ctx_queue_last = ctx;
  } else {
    ctx->prev = ctx_queue_last;
    ctx->next = NULL;
    ctx_queue_last->next = ctx;
    ctx_queue_last = ctx;
  }
}

void finish_ctx(eval_context_t *ctx) {
  if (ctx_done == NULL) {
    ctx->prev = NULL;
    ctx->next = NULL;
    ctx_done = ctx;
  } else {
    ctx->prev = NULL;
    ctx->next = ctx_done;
    ctx_done = ctx;
  }

  if (ctx_done_callback) {
    ctx_done_callback(ctx);
  }
  ctx_running = NULL;
}

void error_ctx(eval_context_t *ctx, VALUE err_val) {
  ctx->r = err_val;
  finish_ctx(ctx);
}


eval_context_t *dequeue_ctx(uint32_t *us) {
  uint32_t min_us = DEFAULT_SLEEP_US;

  uint32_t t_now;
  if (timestamp_us_callback) {
    t_now = timestamp_us_callback();
  } else {
    t_now = 0;
  }

  eval_context_t *curr = ctx_queue;

  while (curr != NULL) {
    uint32_t t_diff;
    if ( curr->timestamp > t_now) {
      /* There was an overflow on the counter */
      t_diff = (0xFFFFFFFF - curr->timestamp) + t_now;
    } else {
      t_diff = t_now - curr->timestamp;
    }

    if (t_diff >= curr->sleep_us) {
      eval_context_t *result = curr;
      if (curr == ctx_queue_last) {
	if (curr->prev) {
	  ctx_queue_last = curr->prev;
	  ctx_queue_last->next = NULL;
	} else {
	  ctx_queue = NULL;
	  ctx_queue_last = NULL;
	}
      } else if (curr->prev == NULL) {
	ctx_queue = curr->next;
	if (ctx_queue) {
	  ctx_queue->prev = NULL;
	}
      } else {
	curr->prev->next = curr->next;
	if (curr->next) {
	  curr->next->prev = curr->prev;
	}
      }
      return result;
    }
    if (min_us > t_diff) min_us = t_diff;
    curr = curr->next;
  }

  *us = min_us;
  return NULL;
}

void yield_ctx(uint32_t sleep_us) {
  if (timestamp_us_callback) {
    ctx_running->timestamp = timestamp_us_callback();
    ctx_running->sleep_us = sleep_us;
  } else {
    ctx_running->timestamp = 0;
    ctx_running->sleep_us = 0;
  }
  //printf("yield: %u,  %u\n", ctx_running->timestamp, ctx_running->sleep_us);
  ctx_running->r = enc_sym(symrepr_true());
  ctx_running->app_cont = true;
  enqueue_ctx(ctx_running);
  ctx_running = NULL;
}

CID create_ctx(VALUE program, uint32_t stack_size, bool grow_stack) {

  if (next_ctx_id == 0) return 0; // overflow of CIDs

  if (type_of(program) != PTR_TYPE_CONS) return 0;

  eval_context_t *ctx = NULL;
  ctx = malloc(sizeof(eval_context_t));
  ctx->program = cdr(program);
  ctx->curr_exp = car(program);
  ctx->curr_env = NIL;
  ctx->done = false;
  ctx->app_cont = false;
  ctx->timestamp = 0;
  ctx->sleep_us = 0;
  ctx->id = next_ctx_id++;
  if (!stack_allocate(&ctx->K, stack_size, grow_stack)) {
    free(ctx);
    return 0;
  }
  if (!push_u32(&ctx->K, enc_u(DONE))) {
    free(ctx);
    stack_free(&ctx->K);
    return 0;
  }

  enqueue_ctx(ctx);

  return ctx->id;
}

void advance_ctx() {

  if (type_of(ctx_running->program) == PTR_TYPE_CONS) {
    push_u32(&ctx_running->K, enc_u(DONE));
    ctx_running->curr_exp = car(ctx_running->program);
    ctx_running->program = cdr(ctx_running->program);
    ctx_running->r = NIL;
    ctx_running->app_cont = false;

  } else {
    ctx_running->done = true;
    finish_ctx(ctx_running);
  }
}

VALUE eval_cps_get_env(void) {
  return eval_cps_global_env;
}

// ////////////////////////////////////////////////////////
// Continuation points and apply cont
// ////////////////////////////////////////////////////////

void cont_set_global_env(eval_context_t *ctx, bool *perform_gc){

  VALUE key;
  VALUE val = ctx->r;

  pop_u32(&ctx->K, &key);
  VALUE new_env = env_set(eval_cps_global_env,key,val);

  if (type_of(new_env) == VAL_TYPE_SYMBOL) {
    if (dec_sym(new_env) == symrepr_merror()) {
      FATAL_ON_FAIL(ctx->done, push_u32_2(&ctx->K, key, enc_u(SET_GLOBAL_ENV)));
      *perform_gc = true;
      return;
    }
    if (dec_sym(new_env) == symrepr_fatal_error()) {
      ERROR
      error_ctx(ctx, new_env);
      return;
    }
  }
  eval_cps_global_env = new_env;
  ctx->r = enc_sym(symrepr_true());
  return;
}

void apply_continuation(eval_context_t *ctx, bool *perform_gc){

  VALUE k;
  pop_u32(&ctx->K, &k);

  VALUE arg = ctx->r;

  ctx->app_cont = false;

  switch(dec_u(k)) {
  case DONE:
    advance_ctx(ctx);
    return;
  case SET_GLOBAL_ENV:
    cont_set_global_env(ctx, perform_gc);
    if (!ctx->done)
      ctx->app_cont = true;
    return;
  case PROGN_REST: {
    VALUE rest;
    VALUE env;
    pop_u32_2(&ctx->K, &rest, &env);
    if (type_of(rest) == VAL_TYPE_SYMBOL && rest == NIL) {
      ctx->app_cont = true;
      return;
    }

    if (symrepr_is_error(rest)) {
      ERROR
      error_ctx(ctx, rest);
      return;
    }
    // allow for tail recursion
    if (type_of(cdr(rest)) == VAL_TYPE_SYMBOL &&
	cdr(rest) == NIL) {
      ctx->curr_exp = car(rest);
      return;
    }
    // Else create a continuation
    FATAL_ON_FAIL(ctx->done, push_u32_3(&ctx->K, env, cdr(rest), enc_u(PROGN_REST)));
    ctx->curr_exp = car(rest);
    ctx->curr_env = env;
    return;
  }
  case APPLICATION: {
    VALUE count;
    pop_u32(&ctx->K, &count);

    UINT *fun_args = stack_ptr(&ctx->K, dec_u(count)+1);

    VALUE fun = fun_args[0];

    if (type_of(fun) == PTR_TYPE_CONS) { // a closure (it better be)
      VALUE args = NIL;
      for (UINT i = dec_u(count); i > 0; i --) {
	args = cons(fun_args[i], args);
	if (type_of(args) == VAL_TYPE_SYMBOL) {
	  FATAL_ON_FAIL(ctx->done, push_u32_2(&ctx->K, count, enc_u(APPLICATION)));
	  *perform_gc = true;
	  ctx->app_cont = true;
	  ctx->r = fun;
	  return;
	}
      }
      VALUE params  = car(cdr(fun));
      VALUE exp     = car(cdr(cdr(fun)));
      VALUE clo_env = car(cdr(cdr(cdr(fun))));

      if (length(params) != length(args)) { // programmer error
	ERROR
	error_ctx(ctx,enc_sym(symrepr_eerror()));
	return;
      }

      VALUE local_env = env_build_params_args(params, args, clo_env);
      if (type_of(local_env) == VAL_TYPE_SYMBOL) {
	if (dec_sym(local_env) == symrepr_merror() ) {
	  FATAL_ON_FAIL(ctx->done, push_u32_2(&ctx->K, count, enc_u(APPLICATION)));
	  *perform_gc = true;
	  ctx->app_cont = true;
	  ctx->r = fun;
	  return;
	}

	if (dec_sym(local_env) == symrepr_fatal_error()) {
	  ctx->r = local_env;
	  return;
	}
      }

      /* ************************************************************
	 Odd area!  It feels like the callers environment should be
	 explicitly restored after an application of a closure.
	 However, if the callers environment is pushed onto the stack
	 here, it will make the stack grow proportional to the call
	 depth.

	 I am very unsure about the correctness here.
         ************************************************************ */

      stack_drop(&ctx->K, dec_u(count)+1);
      ctx->curr_exp = exp;
      ctx->curr_env = local_env;
      return;
    } else if (type_of(fun) == VAL_TYPE_SYMBOL) {


      /* TODO: check there are enough arguments */
      if (dec_sym(fun) == symrepr_yield()) {
	if (type_of(fun_args[1]) == VAL_TYPE_I) {
	  INT ts = dec_i(fun_args[1]);
	  stack_drop(&ctx->K, dec_u(count)+1);
	  yield_ctx(ts);
	} else {
	  ERROR
	  error_ctx(ctx, enc_sym(symrepr_eerror()));
	}
	return;
      }

      if (dec_sym(fun) == symrepr_eval()) {
	ctx->curr_exp = fun_args[1];
	stack_drop(&ctx->K, dec_u(count)+1);
	return;
      }

      VALUE res;

      if (is_fundamental(fun)) {
	res = fundamental_exec(&fun_args[1], dec_u(count), fun);
	if (type_of(res) == VAL_TYPE_SYMBOL &&
	    dec_sym(res) == symrepr_eerror()) {
	  ERROR
	  error_ctx(ctx, res);
	  return;
	} else if (type_of(res) == VAL_TYPE_SYMBOL &&
		   dec_sym(res) == symrepr_merror()) {
	  FATAL_ON_FAIL(ctx->done, push_u32_2(&ctx->K, count, enc_u(APPLICATION)));
	  *perform_gc = true;
	  ctx->app_cont = true;
	  ctx->r = fun;
	  return;
	}
  	stack_drop(&ctx->K, dec_u(count)+1);
	ctx->app_cont = true;
	ctx->r = res;
	return;
      }
    }

    // It may be an extension

    extension_fptr f = extensions_lookup(dec_sym(fun));
    if (f == NULL) {
      ERROR
      error_ctx(ctx, enc_sym(symrepr_eerror()));
      return;
    }

    VALUE ext_res = f(&fun_args[1] , dec_u(count));

    if (type_of(ext_res) == VAL_TYPE_SYMBOL &&
	(dec_sym(ext_res) == symrepr_merror())) {
      FATAL_ON_FAIL(ctx->done, push_u32_2(&ctx->K, count, enc_u(APPLICATION)));
      *perform_gc = true;
      ctx->app_cont = true;
      ctx->r = fun;
      return;
    }

    stack_drop(&ctx->K, dec_u(count) + 1);

    ctx->app_cont = true;
    ctx->r = ext_res;
    return;
  }
  case AND: {
    VALUE env;
    VALUE rest;
    pop_u32_2(&ctx->K, &rest, &env);
    if (type_of(arg) == VAL_TYPE_SYMBOL &&
	dec_sym(arg) == symrepr_nil()) {
      ctx->app_cont = true;
      ctx->r = enc_sym(symrepr_nil());
      return;
    }
    if (type_of(rest) == VAL_TYPE_SYMBOL &&
	rest == NIL) {
      ctx->app_cont = true;
      return;
    } else {
      FATAL_ON_FAIL(ctx->done, push_u32_3(&ctx->K, env, cdr(rest), enc_u(AND)));
      ctx->curr_exp = car(rest);
      ctx->curr_env = env;
      return;
    }
  }
  case OR: {
    VALUE env;
    VALUE rest;
    pop_u32_2(&ctx->K, &rest, &env);
    if (type_of(arg) != VAL_TYPE_SYMBOL ||
	dec_sym(arg) != symrepr_nil()) {
      ctx->app_cont = true;
      return;
    }
    if (type_of(rest) == VAL_TYPE_SYMBOL &&
	rest == NIL) {
      ctx->app_cont = true;
      ctx->r = enc_sym(symrepr_nil());
      return;
    } else {
      FATAL_ON_FAIL(ctx->done, push_u32_3(&ctx->K, env, cdr(rest), enc_u(OR)));
      ctx->curr_exp = car(rest);
      ctx->curr_env = env;
      return;
    }
  }
  case APPLICATION_ARGS: {
    VALUE count;
    VALUE env;
    VALUE rest;

    pop_u32_3(&ctx->K, &rest, &count, &env);

    /* Deal with short-circuiting operators */
    if (type_of(arg) == VAL_TYPE_SYMBOL &&
	dec_sym(arg) == symrepr_and()) {
      if (type_of(rest) == VAL_TYPE_SYMBOL &&
	  rest == NIL) {
	ctx->app_cont = true;
	ctx->r = enc_sym(symrepr_true());
	return;
      } else {
	FATAL_ON_FAIL(ctx->done, push_u32_3(&ctx->K, env, cdr(rest), enc_u(AND)));
	ctx->curr_exp = car(rest);
	ctx->curr_env = env;
	return;
      }
    }

    if (type_of(arg) == VAL_TYPE_SYMBOL &&
	dec_sym(arg) == symrepr_or()) {
      if (type_of(rest) == VAL_TYPE_SYMBOL &&
	  rest == NIL) {
	ctx->app_cont = true;
	ctx->r = enc_sym(symrepr_nil());
	return;
      } else {
	FATAL_ON_FAIL(ctx->done, push_u32_3(&ctx->K, env, cdr(rest), enc_u(OR)));
	ctx->curr_exp = car(rest);
	ctx->curr_env = env;
	return;
      }
    }

    FATAL_ON_FAIL(ctx->done, push_u32(&ctx->K, arg));
    /* Deal with general fundamentals */
    if (type_of(rest) == VAL_TYPE_SYMBOL &&
	rest == NIL) {
      // no arguments
      FATAL_ON_FAIL(ctx->done, push_u32_2(&ctx->K, count, enc_u(APPLICATION)));
      ctx->app_cont = true;
      return;
    }
    FATAL_ON_FAIL(ctx->done, push_u32_4(&ctx->K, env, enc_u(dec_u(count) + 1), cdr(rest), enc_u(APPLICATION_ARGS)));
    ctx->curr_exp = car(rest);
    ctx->curr_env = env;
    return;
  }
  case BIND_TO_KEY_REST:{
    VALUE key;
    VALUE env;
    VALUE rest;

    pop_u32_3(&ctx->K, &key, &env, &rest);

    env_modify_binding(env, key, arg);

    if ( type_of(rest) == PTR_TYPE_CONS ){
      VALUE keyn = car(car(rest));
      VALUE valn_exp = car(cdr(car(rest)));

      FATAL_ON_FAIL(ctx->done, push_u32_4(&ctx->K, cdr(rest), env, keyn, enc_u(BIND_TO_KEY_REST)));

      ctx->curr_exp = valn_exp;
      ctx->curr_env = env;
      return;
    }

    // Otherwise evaluate the expression in the populated env
    VALUE exp;
    pop_u32(&ctx->K, &exp);
    ctx->curr_exp = exp;
    ctx->curr_env = env;
    return;
  }
  case IF: {
    VALUE then_branch;
    VALUE else_branch;

    pop_u32_2(&ctx->K, &then_branch, &else_branch);

    if (type_of(arg) == VAL_TYPE_SYMBOL && dec_sym(arg) == symrepr_true()) {
      ctx->curr_exp = then_branch;
    } else {
      ctx->curr_exp = else_branch;
    }
    return;
  }
  } // end switch
  ERROR
  error_ctx(ctx,enc_sym(symrepr_eerror()));
  return;
}

void eval_cps_run_eval(void){

  bool perform_gc = false;
  uint32_t non_gc = 0;

  while (eval_running) {

    if (!ctx_running) {
      uint32_t us;
      ctx_running = dequeue_ctx(&us);
      if (!ctx_running) {
	if (usleep_callback) {
	  usleep_callback(us);
	}
	continue;
      }
    }

    eval_context_t *ctx = ctx_running;

#ifdef VISUALIZE_HEAP
    heap_vis_gen_image();
#endif

    if (perform_gc) {
      if (non_gc == 0) {
	ERROR
	error_ctx(ctx,enc_sym(symrepr_merror()));
	continue;
      }
      non_gc = 0;
      heap_perform_gc_aux(eval_cps_global_env,
			  ctx->curr_env,
			  ctx->curr_exp,
			  ctx->program,
			  ctx->r,
			  ctx->K.data,
			  ctx->K.sp);
      perform_gc = false;
    } else {
      non_gc ++;
    }

    if (ctx->app_cont) {
      apply_continuation(ctx, &perform_gc);
      continue;
    }

    VALUE head;
    VALUE value = enc_sym(symrepr_eerror());

    switch (type_of(ctx->curr_exp)) {

    case VAL_TYPE_SYMBOL:

      value = env_lookup(ctx->curr_exp, ctx->curr_env);
      if (type_of(value) == VAL_TYPE_SYMBOL &&
	  dec_sym(value) == symrepr_not_found()) {

	value = env_lookup(ctx->curr_exp, eval_cps_global_env);

	if (type_of(value) == VAL_TYPE_SYMBOL &&
	    dec_sym(value) == symrepr_not_found()) {

	  if (is_fundamental(ctx->curr_exp)) {
	    value = ctx->curr_exp;
	  } else if (extensions_lookup(dec_sym(ctx->curr_exp)) == NULL) {
	    ERROR
	    error_ctx(ctx,enc_sym(symrepr_eerror()));
	    continue;
	  } else {
	    value = ctx->curr_exp; // symbol representing extension
	                           // evaluates to itself at this stage.
	  }
	}
      }
      ctx->app_cont = true;
      ctx->r = value;
      break;
    case PTR_TYPE_BOXED_F:
    case PTR_TYPE_BOXED_U:
    case PTR_TYPE_BOXED_I:
    case VAL_TYPE_I:
    case VAL_TYPE_U:
    case VAL_TYPE_CHAR:
    case PTR_TYPE_ARRAY:
      ctx->app_cont = true;
      ctx->r = ctx->curr_exp;
      break;
    case PTR_TYPE_REF:
    case PTR_TYPE_STREAM:
      ERROR
      error_ctx(ctx, enc_sym(symrepr_eerror()));
      break;
    case PTR_TYPE_CONS:
      head = car(ctx->curr_exp);

      if (type_of(head) == VAL_TYPE_SYMBOL) {

	// Special form: QUOTE
	if (dec_sym(head) == symrepr_quote()) {
	  ctx->r = car(cdr(ctx->curr_exp));
	  ctx->app_cont = true;
	  continue;
	}

	// Special form: DEFINE
	if (dec_sym(head) == symrepr_define()) {
	  VALUE key = car(cdr(ctx->curr_exp));
	  VALUE val_exp = car(cdr(cdr(ctx->curr_exp)));

	  if (type_of(key) != VAL_TYPE_SYMBOL ||
	      key == NIL) {
	    ERROR
	    error_ctx(ctx, enc_sym(symrepr_eerror()));
	    continue;
	  }

	  FOF(push_u32_2(&ctx->K, key, enc_u(SET_GLOBAL_ENV)));
	  ctx->curr_exp = val_exp;
	  continue;
	}

	// Special form: PROGN
	if (dec_sym(head) == symrepr_progn()) {
	  VALUE exps = cdr(ctx->curr_exp);
	  VALUE env  = ctx->curr_env;

	  if (type_of(exps) == VAL_TYPE_SYMBOL && exps == NIL) {
	    ctx->r = NIL;
	    ctx->app_cont = true;
	    continue;
	  }

	  if (symrepr_is_error(exps)) {
	    ERROR
	    error_ctx(ctx, exps);
	    continue;
	  }
	  FOF(push_u32_3(&ctx->K, env, cdr(exps), enc_u(PROGN_REST)));
	  ctx->curr_exp = car(exps);
	  ctx->curr_env = env;
	  continue;
	}

	// Special form: LAMBDA
	if (dec_sym(head) == symrepr_lambda()) {

	  VALUE env_cpy = env_copy_shallow(ctx->curr_env);

	  if (type_of(env_cpy) == VAL_TYPE_SYMBOL &&
	      dec_sym(env_cpy) == symrepr_merror()) {
	    perform_gc = true;
	    ctx->app_cont = false;
	    continue; // perform gc and resume evaluation at same expression
	  }

	  VALUE env_end;
	  VALUE body;
	  VALUE params;
	  VALUE closure;
	  env_end = cons(env_cpy,NIL);
	  body    = cons(car(cdr(cdr(ctx->curr_exp))), env_end);
	  params  = cons(car(cdr(ctx->curr_exp)), body);
	  closure = cons(enc_sym(symrepr_closure()), params);

	  if (type_of(env_end) == VAL_TYPE_SYMBOL ||
	      type_of(body)    == VAL_TYPE_SYMBOL ||
	      type_of(params)  == VAL_TYPE_SYMBOL ||
	      type_of(closure) == VAL_TYPE_SYMBOL) {
	    perform_gc = true;
	    ctx->app_cont = false;
	    continue; // perform gc and resume evaluation at same expression
	  }

	  ctx->app_cont = true;
	  ctx->r = closure;
	  continue;
	}

	// Special form: IF
	if (dec_sym(head) == symrepr_if()) {

	  FOF(push_u32_3(&ctx->K,
			 car(cdr(cdr(cdr(ctx->curr_exp)))), // Else branch
			 car(cdr(cdr(ctx->curr_exp))),      // Then branch
			 enc_u(IF)));
	  ctx->curr_exp = car(cdr(ctx->curr_exp));
	  continue;
	}
	// Special form: LET
	if (dec_sym(head) == symrepr_let()) {
	  VALUE orig_env = ctx->curr_env;
	  VALUE binds    = car(cdr(ctx->curr_exp)); // key value pairs.
	  VALUE exp      = car(cdr(cdr(ctx->curr_exp))); // exp to evaluate in the new env.

	  VALUE curr = binds;
	  VALUE new_env = orig_env;

	  if (type_of(binds) != PTR_TYPE_CONS) {
	    // binds better be nil or there is a programmer error.
	    ctx->curr_exp = exp;
	    continue;
	  }

	  // Implements letrec by "preallocating" the key parts
	  while (type_of(curr) == PTR_TYPE_CONS) {
	    VALUE key = car(car(curr));
	    VALUE val = NIL;
	    VALUE binding;
	    binding = cons(key, val);
	    new_env = cons(binding, new_env);

	    if (type_of(binding) == VAL_TYPE_SYMBOL ||
		type_of(new_env) == VAL_TYPE_SYMBOL) {
	      perform_gc = true;
	      ctx->app_cont = false;
	      continue;
	    }
	    curr = cdr(curr);
	  }

	  VALUE key0 = car(car(binds));
	  VALUE val0_exp = car(cdr(car(binds)));

	  FOF(push_u32_5(&ctx->K, exp, cdr(binds), new_env,
			 key0, enc_u(BIND_TO_KEY_REST)));
	  ctx->curr_exp = val0_exp;
	  ctx->curr_env = new_env;
	  continue;
	}
      } // If head is symbol
      FOF(push_u32_4(&ctx->K,
		     ctx->curr_env,
		     enc_u(0),
		     cdr(ctx->curr_exp),
		     enc_u(APPLICATION_ARGS)));

      ctx->curr_exp = head; // evaluate the function
      continue;
    default:
      // BUG No applicable case!
      ERROR
      error_ctx(ctx, enc_sym(symrepr_eerror()));
      break;
    }
  } // while (true)
}

CID eval_cps_program(VALUE lisp) {
  return create_ctx(lisp, 256, false);
}

int eval_cps_init() {
  int res = 1;
  NIL = enc_sym(symrepr_nil());
  NONSENSE = enc_sym(symrepr_nonsense());

  eval_cps_global_env = NIL;

  VALUE nil_entry = cons(NIL, NIL);
  eval_cps_global_env = cons(nil_entry, eval_cps_global_env);

  if (type_of(nil_entry) == VAL_TYPE_SYMBOL ||
      type_of(eval_cps_global_env) == VAL_TYPE_SYMBOL) res = 0;

  eval_running = true;

  return res;
}

void eval_cps_del(void) {

}
