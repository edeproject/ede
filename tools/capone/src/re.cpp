#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern "C" {
#define USE_INTERFACE 1

#include "scheme-private.h"
#include "scheme.h"
}

#include "pcre/pcre.h"
#include "re.h"

#define RE_VEC_COUNT (16 * 3) /* max sub expressions in PCRE */

#define car(p)           ((p)->_object._cons._car)
#define cdr(p)           ((p)->_object._cons._cdr)

/* TODO: this is used in sys.cpp too. Move it somewhere else */
pointer reverse_in_place(scheme *sc, pointer term, pointer list) {
	pointer p = list, result = term, q;

	while (p != sc->NIL) {
		q = cdr(p);
		cdr(p) = result;
		result = p;
		p = q;
	}

	return result;
}

static pointer build_string(scheme* sc, const char* p, int len) {
	char* str = (char*)malloc(len + 1);
	if(!str)
		return sc->NIL;

	char* ptr = str;

	for(int i = 0; i < len; i++)
		*ptr++ = *p++;
	*ptr = '\0';

	pointer ret = mk_string(sc, str);
	free(str);

	return ret;
}

/* 
 * (re-split <pattern> <string>) => ("list" "of" "splitted" "items") 
 * or return empty list if matched nothing or failed 
 */
static pointer re_split(scheme* sc, pointer args) {
	if(args == sc->NIL)
		return args;

	pointer a;
	const char* reg, *str;

	a = sc->vptr->pair_car(args);
	if(a == sc->NIL || !sc->vptr->is_string(a))
		return sc->NIL;
	reg = sc->vptr->string_value(a);

	args = sc->vptr->pair_cdr(args);

	a = sc->vptr->pair_car(args);
	if(a == sc->NIL || !sc->vptr->is_string(a))
		return sc->NIL;
	str = sc->vptr->string_value(a);

	const char* errstr;
	int erroffset;

	pcre* p = pcre_compile(reg, 0, &errstr, &erroffset, NULL);
	if(!p) {
		printf("Failed to compile '%s' pattern\n", reg);
		return sc->NIL; /* sc->NIL means empty list */
	}

	int ovector[RE_VEC_COUNT];
	int str_len = strlen(str);
	int slen = str_len;
	const char* sp = str;

	int ret = pcre_exec(p, NULL, sp, slen, 0, 0, ovector, RE_VEC_COUNT);

	/* check to see if we have anything before loop */
	if(ret == -1) {
		free(p);
		printf("No match\n");
		return sc->NIL;
	}

	if(ret < 0) {
		free(p);
		printf("pcre_exec() failed\n");
		return sc->NIL;
	}

	pointer lst_ret = sc->NIL;

	while(1) {
		/* 
		 * ovector[0] is num bytes before match
		 * ovector[1] is num bytes of match + ovector[0]
		 * so ovector[1] - ovector[0] is match length
		 */
		if((ovector[1] - ovector[0]) == 0) {
			/* use last token after last match */
			lst_ret = cons(sc, build_string(sc, sp, slen), lst_ret);
			break;
		}

		lst_ret = cons(sc, build_string(sc, sp, ovector[0]), lst_ret);

		sp += ovector[1];
		slen -= ovector[1]; 
		ret = pcre_exec(p, NULL, sp, slen, 0, 0, ovector, RE_VEC_COUNT);
	}

	free(p);

	/* reverse list due cons() property */
	return reverse_in_place(sc, sc->NIL, lst_ret);
}

/*
 * (re-match <pattern> <string> <optional-start-pos>) => (<match-start> <match-len>)
 * or return empty list if matched nothing or failed 
 */
static pointer re_match(scheme* sc, pointer args) {
	if(args == sc->NIL)
		return args;

	pointer a;
	const char* reg, *str;

	a = sc->vptr->pair_car(args);
	if(a == sc->NIL || !sc->vptr->is_string(a))
		return sc->NIL;
	reg = sc->vptr->string_value(a);

	args = sc->vptr->pair_cdr(args);

	a = sc->vptr->pair_car(args);
	if(a == sc->NIL || !sc->vptr->is_string(a))
		return sc->NIL;
	str = sc->vptr->string_value(a);

	int start = 0, len = strlen(str);

	args = sc->vptr->pair_cdr(args);
	a = sc->vptr->pair_car(args);
	if(a != sc->NIL && sc->vptr->is_number(a)) {
		start = sc->vptr->ivalue(a);
		/*
		 * TODO: this will always return empty list
		 * Should this be 'start = 0' ?
		 */
		if(start > len)
			start = len;

		if(start < 0)
			start = 0;
	}

	const char* errstr;
	int erroffset;

	pcre* p = pcre_compile(reg, 0, &errstr, &erroffset, NULL);
	if(!p) {
		printf("Failed to compile '%s' pattern\n", reg);
		return sc->NIL; /* sc->NIL means empty list */
	}

	int ovector[RE_VEC_COUNT];
	int ret = pcre_exec(p, NULL, str, len, start, 0, ovector, RE_VEC_COUNT);
	free(p);

	if(ret >= 0) {
		return cons(sc, mk_integer(sc, ovector[0]),
				cons(sc, mk_integer(sc, ovector[1]), sc->NIL));
	}

	return sc->NIL;
}

/*
 * (re-replace <pattern> <target> <replacement> <optional-false>) => <replaced-target>
 * if given <optional-false>, only first pattern will be replaced
 * or <target> if nothing found
 * TODO: it will return #f if parameters are wrong. Good?
 */
pointer re_replace(scheme* sc, pointer args) {
	if(args == sc->NIL)
		return sc->F;

	pointer a;
	const char* reg, *target, *rep;
	bool replace_all = true;

	a = sc->vptr->pair_car(args);
	if(a == sc->NIL || !sc->vptr->is_string(a))
		return sc->F;
	reg = sc->vptr->string_value(a);

	args = sc->vptr->pair_cdr(args);

	a = sc->vptr->pair_car(args);
	if(a == sc->NIL || !sc->vptr->is_string(a))
		return sc->F;
	target = sc->vptr->string_value(a);

	args = sc->vptr->pair_cdr(args);

	a = sc->vptr->pair_car(args);
	if(a == sc->NIL || !sc->vptr->is_string(a))
		return sc->F;
	rep = sc->vptr->string_value(a);

	args = sc->vptr->pair_cdr(args);

	/* TODO: here can be any integer that will be casted to bool */
	a = sc->vptr->pair_car(args);
	if(a != sc->NIL && sc->vptr->is_number(a))
		replace_all = sc->vptr->ivalue(a);

	const char* errstr;
	int erroffset;

	pcre* p = pcre_compile(reg, 0, &errstr, &erroffset, NULL);
	if(!p) {
		printf("Failed to compile '%s' pattern\n", reg);
		return sc->F;
	}

	int len = strlen(target);
	int ovector[RE_VEC_COUNT];
	int ret = pcre_exec(p, NULL, target, len, 0, 0, ovector, RE_VEC_COUNT);

	if(ret >= 0) {
		int rep_len = strlen(rep);
		/* ovector[1] - ovector[0] is len of matched pattern */
		int nlen = len - (ovector[1] - ovector[0]) + rep_len;
		char* newstr = (char*)malloc(nlen + 1);

		int i, j;
		for(i = 0; i < ovector[0]; i++)
			newstr[i] = target[i];
		for(j = 0; j < rep_len; i++, j++)
			newstr[i] = rep[j];
		/* now copy the rest */
		for(j = ovector[1]; j < len; j++, i++)
			newstr[i] = target[j];
		newstr[i] = '\0';

		pointer s = mk_string(sc, newstr);
		free(newstr);
		free(p);
		return s;
	}

	free(p);
	return mk_string(sc, target);
}

void register_re_functions(scheme* sc) {
	sc->vptr->scheme_define(
		sc,
		sc->global_env,
		sc->vptr->mk_symbol(sc, "re-split"),
		sc->vptr->mk_foreign_func(sc, re_split));

	sc->vptr->scheme_define(
		sc,
		sc->global_env,
		sc->vptr->mk_symbol(sc, "re-match"),
		sc->vptr->mk_foreign_func(sc, re_match));

	sc->vptr->scheme_define(
		sc,
		sc->global_env,
		sc->vptr->mk_symbol(sc, "re-replace"),
		sc->vptr->mk_foreign_func(sc, re_replace));
}
