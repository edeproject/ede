#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <edelib/Missing.h>

extern "C" {
#define USE_INTERFACE 1

#include "scheme-private.h"
#include "scheme.h"
}

#include "sys.h"

extern char** environ;
extern pointer reverse_in_place(scheme *sc, pointer term, pointer list);

/*
 * (getenv <what>) => <string>
 * returns environment value for <what>; if <what> is not
 * given, returns a list of all environment key/value pairs
 */
static pointer s_getenv(scheme* sc, pointer arg) {
	if(arg == sc->NIL) {
		char** env = environ;
		pointer lst = sc->NIL;

		while(*env) {
			lst = cons(sc, mk_string(sc, *env), lst);
			env++;
		}

		return reverse_in_place(sc, sc->NIL, lst);
	}
	
	pointer a = sc->vptr->pair_car(arg);
	if(a != sc->NIL && sc->vptr->is_string(a)) {
		const char* val;
		if((val = getenv(sc->vptr->string_value(a))) != NULL)
			return mk_string(sc, val);
	}

	return sc->F;
}

static pointer s_setenv(scheme* sc, pointer args) {
	if(args == sc->NIL)
		return sc->F;

	const char* key, *val;
	pointer a = sc->vptr->pair_car(args);
	if(a == sc->NIL || !sc->vptr->is_string(a))
		return sc->F;
	key = sc->vptr->string_value(a);

	args = sc->vptr->pair_cdr(args);
	a = sc->vptr->pair_car(args);
	if(a == sc->NIL || !sc->vptr->is_string(a))
		return sc->F;
	val = sc->vptr->string_value(a);

	if(edelib_setenv(key, val, 1) == 0)
		return sc->T;
	return sc->F;
}

void register_sys_functions(scheme* sc) {
	sc->vptr->scheme_define(
		sc,
		sc->global_env,
		sc->vptr->mk_symbol(sc, "getenv"),
		sc->vptr->mk_foreign_func(sc, s_getenv));

	sc->vptr->scheme_define(
		sc,
		sc->global_env,
		sc->vptr->mk_symbol(sc, "setenv"),
		sc->vptr->mk_foreign_func(sc, s_setenv));
}
