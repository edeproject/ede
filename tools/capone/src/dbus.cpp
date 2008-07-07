#include <string.h>
#include <stdio.h>
#include <edelib/EdbusConnection.h>

extern "C" {
#define USE_INTERFACE 1

#include "scheme-private.h"
#include "scheme.h"
}

#include "dbus.h"

using namespace edelib;

/* next types in scheme_types in 'scheme.cpp' */
/* 
#define T_DBUS_CONNECTION 15
#define T_DBUS_MESSAGE    16
*/

/* (dbus-send <service> <interface> <path> <"system|session"> <message>*/
static pointer dbus_send(scheme* sc, pointer args) {
	if(args == sc->NIL)
		return sc->F;

	pointer a;
	const char* service, *interface, *path;
	bool is_system = false;

	a = sc->vptr->pair_car(args);
	if(a == sc->NIL || !sc->vptr->is_string(a))
		return sc->F;
	service = sc->vptr->string_value(a);

	args = sc->vptr->pair_cdr(args);

	a = sc->vptr->pair_car(args);
	if(a == sc->NIL || !sc->vptr->is_string(a))
		return sc->F;
	interface = sc->vptr->string_value(a);

	args = sc->vptr->pair_cdr(args);

	a = sc->vptr->pair_car(args);
	if(a == sc->NIL || !sc->vptr->is_string(a))
		return sc->F;
	path = sc->vptr->string_value(a);

	args = sc->vptr->pair_cdr(args);

	a = sc->vptr->pair_car(args);
	if(a == sc->NIL || !sc->vptr->is_string(a))
		return sc->F;

	const char* sv = sc->vptr->string_value(a);
	if(strcmp(sv, "system") == 0)
		is_system = true;

	EdbusConnection c;
	bool ret;
	if(is_system)
		ret = c.connect(EDBUS_SYSTEM);
	else
		ret = c.connect(EDBUS_SESSION);
	if(!ret)
		return sc->F;

	args = sc->vptr->pair_cdr(args);

	EdbusMessage msg;
	msg.create_signal(path, interface, service);

	if(sc->vptr->is_number(a))
		msg << EdbusData::from_int32(sc->vptr->ivalue(a));
	else if(sc->vptr->is_string(a))
		msg << EdbusData::from_string(sc->vptr->string_value(a));
	else
		return sc->F;

	return c.send(msg) ? sc->T : sc->F;
}

void register_dbus_functions(scheme* sc) {
	sc->vptr->scheme_define(
		sc,
		sc->global_env,
		sc->vptr->mk_symbol(sc, "dbus-send"),
		sc->vptr->mk_foreign_func(sc, dbus_send));
}
