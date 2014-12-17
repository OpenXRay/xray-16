/*
** Hints for the JIT compiler backend.
** Copyright (C) 2005-2012 Mike Pall. See Copyright Notice in luajit.h
*/

#ifdef STRING_HINTS
#define HH(x, name)		#name
#define HH_START(x)		static const char *const hints_##x [] = {
#define HH_END(x)		NULL }
#else
#define HH(x, name)		JIT_##x##_##name
#define HH_START(x)		enum { JIT_##x##_NONE,
#define HH_END(x)		JIT_##x##_MAX }

/* Macros to access hints. */
#define JIT_H2NUM(x)		((x) << 16)

#define fhint_get(J, hh) \
  (luaH_getnum(J->comstate, JIT_H2NUM(JIT_FH_##hh)))
#define fhint_isset(J, hh)	(!ttisnil(fhint_get(J, hh)))

#define hint_getpc(J, hh, pc) \
  (luaH_getnum(J->comstate, (pc)+JIT_H2NUM(JIT_H_##hh)))
#define hint_get(J, hh)		hint_getpc(J, hh, J->nextpc-1)
#define hint_issetpc(J, hh, pc)	(!ttisnil(hint_getpc(J, hh, pc)))
#define hint_isset(J, hh)	hint_issetpc(J, hh, J->nextpc-1)

#endif

/* Hints for functions. */
HH_START(FH)
  HH(FH, NOCLOSE),		/* No luaF_close() needed. */
HH_END(FH);

/* Hints for individual bytecode instruction. */
HH_START(H)
  HH(H, COMBINE),	/* Combine/dead instruction: true/false. */
  HH(H, FOR_STEP_K),	/* FORPREP/FORLOOP step is const: step. */
  HH(H, TYPE),		/* Type hint: typed object. */
  HH(H, TYPEKEY),	/* Type hint for keys: typed object. */
  HH(H, INLINE),	/* Inline function call: internal index. */
HH_END(H);

#undef HH
#undef HH_START
#undef HH_END


/* Avoid multiple inclusion for the following. */
#ifndef ljit_hints_h
#define ljit_hints_h

/* Index numbers for inlining C functions. */
/* CHECK: the index numbers must match with jit.opt_lib. */

#define JIT_IH_LIB(x)		((x) >> 16)
#define JIT_IH_IDX(x)		((x) & 0xffff)
#define JIT_IH_MKIDX(l, f)	(((l) << 16) | (f))

/* Library index numbers. */
enum {
  JIT_IHLIB_INTERNAL,
  JIT_IHLIB_BASE,
  JIT_IHLIB_COROUTINE,
  JIT_IHLIB_STRING,
  JIT_IHLIB_TABLE,
  JIT_IHLIB_MATH,
  JIT_IHLIB__LAST
};

/* Internal functions. */
enum {
  JIT_IH_INTERNAL_RECURSIVE,  /* Recursive call. */
  JIT_IH_INTERNAL__LAST
};

/* Base library functions. */
enum {
  JIT_IH_BASE_PAIRS,
  JIT_IH_BASE_IPAIRS,
  JIT_IH_BASE__LAST
};

/* Coroutine library functions. */
enum {
  JIT_IH_COROUTINE_YIELD,
  JIT_IH_COROUTINE_RESUME,
  JIT_IH_COROUTINE__LAST
};

/* String library functions. */
enum {
  JIT_IH_STRING_LEN,
  JIT_IH_STRING_SUB,
  JIT_IH_STRING_CHAR,
  JIT_IH_STRING__LAST
};

/* Table library functions. */
enum {
  JIT_IH_TABLE_INSERT,
  JIT_IH_TABLE_REMOVE,
  JIT_IH_TABLE_GETN,
  JIT_IH_TABLE__LAST
};

/* Math library functions. */
/* CHECK: order must match with function table for jit_inline_math(). */
enum {
  /* 1 arg, 1 result. */
  /* Partially inlined. Call C function from libm: */
  JIT_IH_MATH_LOG,
  JIT_IH_MATH_LOG10,
  JIT_IH_MATH_EXP,
  JIT_IH_MATH_SINH,
  JIT_IH_MATH_COSH,
  JIT_IH_MATH_TANH,
  JIT_IH_MATH_ASIN,
  JIT_IH_MATH_ACOS,
  JIT_IH_MATH_ATAN,
  /* Fully inlined: */
  JIT_IH_MATH_SIN,
  JIT_IH_MATH_COS,
  JIT_IH_MATH_TAN,
  JIT_IH_MATH_CEIL,
  JIT_IH_MATH_FLOOR,
  JIT_IH_MATH_ABS,
  JIT_IH_MATH_SQRT,
  /* 2 args, 1 result. */
  JIT_IH_MATH_FMOD,
  JIT_IH_MATH_ATAN2,
  JIT_IH_MATH__LAST
};

#define JIT_IH_MATH__21	JIT_IH_MATH_FMOD

#endif
