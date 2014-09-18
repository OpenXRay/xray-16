/*
** Interface to DynASM engine.
** Copyright (C) 2005-2008 Mike Pall. See Copyright Notice in luajit.h
*/

#ifndef ljit_dasm_h
#define ljit_dasm_h

#include "ljit.h"

/* DynASM glue definitions. */
#define Dst		J
#define Dst_DECL	jit_State *J
#define Dst_REF		(J->D)
#define DASM_FDEF	LUAI_FUNC

#include "dasm_proto.h"

#endif
