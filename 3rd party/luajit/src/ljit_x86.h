/*
** This file has been pre-processed with DynASM.
** http://luajit.org/dynasm.html
** DynASM version 1.1.4, DynASM x86 version 1.1.4
** DO NOT EDIT! The original file is in "ljit_x86.dasc".
*/

#if DASM_VERSION != 10104
#error "Version mismatch between DynASM and included encoding engine"
#endif

/*
** Bytecode to machine code translation for x86 CPUs.
** Copyright (C) 2005-2008 Mike Pall. See Copyright Notice in luajit.h
*/

#define DASM_SECTION_CODE	0
#define DASM_SECTION_DEOPT	1
#define DASM_SECTION_TAIL	2
#define DASM_SECTION_MFMAP	3
#define DASM_MAXSECTION		4
#define Dt1(_V) (int)&(((lua_State *)0)_V)
#define Dt2(_V) (int)&(((TValue *)0)_V)
#define Dt3(_V) (int)&(((TValue *)0)_V)
#define Dt4(_V) (int)&(((CallInfo *)0)_V)
#define Dt5(_V) (int)&(((LClosure *)0)_V)
#define Dt6(_V) (int)&(((global_State *)0)_V)
#define Dt7(_V) (int)&(((TValue *)0)_V)
#define Dt8(_V) (int)&(((Value *)0)_V)
#define Dt9(_V) (int)&(((CallInfo *)0)_V)
#define DtA(_V) (int)&(((GCObject *)0)_V)
#define DtB(_V) (int)&(((TString *)0)_V)
#define DtC(_V) (int)&(((Table *)0)_V)
#define DtD(_V) (int)&(((CClosure *)0)_V)
#define DtE(_V) (int)&(((Proto *)0)_V)
#define DtF(_V) (int)&(((UpVal *)0)_V)
#define Dt10(_V) (int)&(((Node *)0)_V)
static const unsigned char jit_actionlist[5059] = {
  156,90,137,209,129,252,242,0,0,32,0,82,157,156,90,49,192,57,209,15,132,245,
  247,64,83,15,162,91,137,208,249,1,195,255,254,0,251,15,249,10,141,68,36,4,
  195,251,15,249,11,85,137,229,131,252,236,8,137,93,252,252,139,93,12,137,117,
  12,139,117,8,137,125,252,248,139,190,235,139,131,235,139,142,235,102,252,
  255,134,235,252,255,144,235,139,142,235,137,190,235,139,145,235,139,69,16,
  137,150,235,139,145,235,133,192,137,150,235,15,136,245,248,193,224,4,1,195,
  49,201,137,158,235,249,1,137,143,235,129,199,241,57,223,15,130,245,1,249,
  2,255,102,252,255,142,235,184,239,139,125,252,248,139,93,252,252,139,117,
  12,137,252,236,93,195,251,15,249,12,139,144,235,129,186,235,241,15,133,245,
  247,139,146,235,137,144,235,252,255,226,249,1,131,252,236,12,139,129,235,
  137,142,235,137,190,235,199,68,36,8,252,255,252,255,252,255,252,255,137,134,
  235,137,92,36,4,43,158,235,137,52,36,232,244,133,192,15,133,245,248,137,52,
  36,199,68,36,4,1,0,0,0,232,244,249,2,131,196,12,3,158,235,255,139,190,235,
  195,251,15,249,13,141,135,235,131,252,236,12,59,134,235,15,131,245,14,59,
  142,235,141,137,235,15,132,245,15,137,142,235,137,153,235,137,129,235,139,
  147,235,129,195,241,137,190,235,137,158,235,137,153,235,249,16,137,52,36,
  252,255,146,235,249,2,131,196,12,139,142,235,255,193,224,4,139,185,235,15,
  132,245,250,139,158,235,137,218,41,195,249,3,139,3,131,195,4,137,7,131,199,
  4,57,211,15,130,245,3,249,4,139,153,235,129,233,241,137,142,235,195,144,144,
  144,144,144,144,251,15,249,17,252,246,134,235,237,15,133,245,253,249,6,137,
  52,36,252,255,146,235,252,246,134,235,237,15,132,245,2,255,137,195,137,52,
  36,199,68,36,4,239,199,68,36,8,252,255,252,255,252,255,252,255,232,244,137,
  216,233,245,2,249,7,137,211,137,52,36,199,68,36,4,239,199,68,36,8,252,255,
  252,255,252,255,252,255,232,244,137,218,233,245,6,251,15,249,14,41,252,248,
  193,252,248,4,137,190,235,43,158,235,137,76,36,8,137,52,36,137,68,36,4,232,
  244,139,76,36,8,3,158,235,139,190,235,139,131,235,131,196,12,252,255,160,
  235,251,15,249,15,137,190,235,137,52,36,232,244,141,136,235,255,139,131,235,
  137,142,235,131,196,12,252,255,160,235,255,249,18,90,233,245,19,249,20,137,
  190,235,249,19,137,150,235,137,52,36,232,244,139,158,235,139,190,235,252,
  255,224,251,15,255,137,190,235,255,232,245,21,255,251,15,249,21,252,246,134,
  235,237,15,132,245,248,252,255,142,235,15,132,245,247,252,246,134,235,237,
  15,132,245,248,249,1,139,4,36,131,252,236,12,137,52,36,137,68,36,4,232,244,
  131,196,12,139,158,235,139,190,235,249,2,195,255,250,255,233,246,255,250,
  243,255,254,1,233,245,19,254,0,250,254,2,250,251,1,252,255,252,255,254,3,
  242,0,0,0,0,0,0,0,0,0,254,0,141,249,9,186,239,254,0,249,9,186,239,233,245,
  20,254,0,139,142,235,139,129,235,191,247,253,59,129,235,15,131,245,22,249,
  7,255,251,15,249,22,137,52,36,232,244,139,158,235,252,255,231,255,131,187,
  235,5,15,133,245,9,49,192,137,131,235,137,131,235,254,3,238,238,254,0,131,
  190,235,0,15,132,245,9,199,134,235,239,129,195,241,255,141,187,235,255,137,
  158,235,137,190,235,137,52,36,232,244,139,158,235,139,190,235,255,199,135,
  235,0,0,0,0,255,139,139,235,252,243,15,126,131,235,137,139,235,102,15,214,
  131,235,255,139,139,235,139,147,235,139,131,235,137,139,235,137,147,235,137,
  131,235,255,57,223,15,130,245,9,255,131,187,235,8,15,133,245,9,139,131,235,
  131,184,235,0,15,132,245,9,199,134,235,239,137,190,235,137,52,36,137,92,36,
  4,199,68,36,8,239,232,244,139,158,235,255,137,199,255,131,187,235,4,15,133,
  245,9,139,139,235,219,129,235,199,131,235,3,0,0,0,221,155,235,255,141,187,
  235,232,245,23,137,131,235,199,131,235,4,0,0,0,255,141,187,235,232,245,24,
  137,131,235,199,131,235,4,0,0,0,255,131,187,235,3,15,133,245,9,141,134,235,
  221,131,235,219,24,129,56,252,255,0,0,0,15,135,245,9,137,52,36,137,68,36,
  4,199,68,36,8,1,0,0,0,232,244,137,131,235,199,131,235,4,0,0,0,255,251,15,
  249,23,139,135,235,193,224,4,11,135,235,193,224,4,11,135,235,45,51,4,0,0,
  15,133,245,18,221,135,235,221,135,235,219,92,36,8,219,92,36,4,139,143,235,
  139,185,235,139,84,36,8,57,215,15,130,245,250,249,1,11,68,36,4,15,142,245,
  252,249,2,41,194,15,140,245,253,141,140,253,1,235,66,249,3,137,116,36,4,137,
  76,36,8,137,84,36,12,139,190,235,139,135,235,255,59,135,235,15,131,245,254,
  233,244,249,4,15,140,245,251,141,84,58,1,233,245,1,249,5,137,252,250,233,
  245,1,249,6,15,132,245,251,1,252,248,64,15,143,245,2,249,5,184,1,0,0,0,233,
  245,2,249,7,49,210,233,245,3,255,251,15,249,24,139,135,235,193,224,4,11,135,
  235,131,232,67,15,133,245,18,221,135,235,219,92,36,4,139,143,235,139,185,
  235,137,252,250,233,245,1,249,8,131,252,236,12,137,52,36,232,244,131,196,
  12,139,158,235,233,244,255,131,187,235,5,15,133,245,9,255,141,131,235,137,
  52,36,137,68,36,4,232,244,255,141,131,235,141,139,235,137,52,36,137,68,36,
  4,137,76,36,8,232,244,255,139,131,235,137,4,36,232,244,137,4,36,219,4,36,
  221,155,235,199,131,235,3,0,0,0,255,131,187,235,3,15,133,245,9,221,131,235,
  255,139,131,235,193,224,4,11,131,235,131,252,248,51,15,133,245,9,255,217,
  252,254,255,217,252,255,255,217,252,242,221,216,255,217,60,36,217,45,239,
  217,252,252,217,44,36,255,217,225,255,217,252,250,255,221,131,235,221,131,
  235,249,1,217,252,248,223,224,158,15,138,245,1,221,217,255,221,131,235,221,
  131,235,217,252,243,255,221,28,36,232,244,255,131,187,235,6,15,133,245,9,
  129,187,235,239,15,133,245,9,255,141,131,235,57,199,15,133,245,9,255,141,
  187,235,137,190,235,255,131,196,12,129,252,235,241,129,174,235,241,195,255,
  141,187,235,137,52,36,137,124,36,4,232,244,133,192,15,133,246,255,139,131,
  235,64,139,147,235,137,131,235,137,20,36,137,68,36,4,232,244,139,136,235,
  133,201,15,132,245,255,219,131,235,199,131,235,3,0,0,0,221,155,235,139,144,
  235,139,128,235,137,139,235,137,147,235,137,131,235,233,246,249,9,255,141,
  135,235,131,252,236,12,59,134,235,15,131,245,14,59,142,235,141,137,235,15,
  132,245,15,49,192,137,153,235,129,195,241,137,142,235,255,141,147,235,57,
  215,255,137,223,255,15,71,252,250,255,15,134,245,247,137,215,249,1,255,141,
  147,235,137,129,235,137,145,235,137,150,235,137,158,235,137,153,235,255,15,
  130,245,251,249,4,254,2,249,5,137,135,235,129,199,241,57,215,15,130,245,5,
  233,245,4,254,0,137,190,235,137,185,235,137,129,235,255,139,139,235,252,243,
  15,126,131,235,137,143,235,102,15,214,135,235,255,139,139,235,139,147,235,
  137,143,235,139,139,235,137,151,235,137,143,235,255,137,252,251,141,147,235,
  141,187,235,137,145,235,137,150,235,255,137,135,235,255,249,2,137,135,235,
  137,135,235,129,199,241,57,215,15,130,245,2,255,137,52,36,232,244,255,252,
  246,134,235,237,15,132,245,255,232,245,25,249,9,255,251,15,249,25,139,142,
  235,139,185,235,139,135,235,139,184,235,139,135,235,131,192,4,137,134,235,
  131,252,236,12,137,52,36,199,68,36,4,239,199,68,36,8,252,255,252,255,252,
  255,252,255,232,244,131,196,12,139,135,235,137,134,235,139,158,235,195,255,
  137,52,36,137,92,36,4,232,244,255,129,174,235,241,137,223,129,252,235,241,
  131,196,12,255,139,142,235,139,153,235,129,233,241,137,142,235,141,187,235,
  131,196,12,255,252,246,134,235,237,15,132,245,253,232,245,26,249,7,255,139,
  68,36,12,137,134,235,255,251,15,249,26,139,4,36,137,134,235,131,252,236,12,
  137,52,36,199,68,36,4,239,199,68,36,8,252,255,252,255,252,255,252,255,232,
  244,131,196,12,139,158,235,139,190,235,195,255,139,145,235,57,252,251,15,
  131,245,248,249,1,139,3,131,195,4,137,2,131,194,4,57,252,251,15,130,245,1,
  249,2,131,196,12,139,153,235,129,233,241,137,215,137,142,235,195,255,129,
  174,235,241,129,252,235,241,255,131,196,12,141,187,235,195,255,139,142,235,
  139,185,235,129,233,241,137,142,235,255,139,139,235,139,147,235,139,131,235,
  137,143,235,137,151,235,137,135,235,255,131,196,12,137,252,251,255,129,199,
  241,255,139,142,235,131,187,235,6,255,139,131,235,186,239,137,145,235,255,
  15,133,245,20,255,15,133,245,19,255,15,132,245,247,232,245,27,249,1,255,251,
  15,249,27,131,252,236,12,137,150,235,137,190,235,137,52,36,137,92,36,4,232,
  244,131,196,12,137,195,139,190,235,139,131,235,139,142,235,195,255,252,255,
  144,235,255,137,158,235,255,49,192,255,141,147,235,249,1,137,135,235,137,
  135,235,129,199,241,57,215,15,130,245,1,255,131,187,235,6,15,133,245,9,255,
  131,187,235,6,15,133,245,251,254,2,249,5,255,186,239,233,245,28,254,0,251,
  15,249,28,137,150,235,137,190,235,137,52,36,137,92,36,4,232,244,139,142,235,
  139,150,235,139,185,235,249,1,139,24,131,192,4,137,31,131,199,4,57,208,15,
  130,245,1,139,153,235,139,131,235,129,233,241,131,196,12,252,255,160,235,
  255,139,131,235,255,139,139,235,139,147,235,137,139,235,139,139,235,137,147,
  235,137,139,235,255,141,187,235,129,252,235,241,139,142,235,137,131,235,255,
  139,142,235,141,187,235,139,153,235,139,135,235,137,131,235,255,139,135,235,
  252,243,15,126,135,235,137,131,235,102,15,214,131,235,255,139,135,235,139,
  151,235,137,131,235,139,135,235,137,147,235,137,131,235,255,141,187,235,139,
  131,235,255,139,145,235,249,1,139,3,131,195,4,137,2,131,194,4,57,252,251,
  15,130,245,1,139,153,235,137,215,139,131,235,255,199,131,235,0,0,0,0,255,
  186,1,0,0,0,137,147,235,137,147,235,255,199,131,235,0,0,0,0,199,131,235,1,
  0,0,0,255,217,252,238,255,217,232,255,221,5,239,255,199,131,235,239,199,131,
  235,4,0,0,0,255,137,131,235,195,255,141,139,235,141,147,235,249,1,137,1,57,
  209,141,137,235,15,134,245,1,255,139,142,235,139,185,235,139,135,235,255,
  139,136,235,139,185,235,255,139,143,235,252,243,15,126,135,235,137,139,235,
  102,15,214,131,235,255,139,143,235,139,151,235,139,135,235,137,139,235,137,
  147,235,137,131,235,255,139,136,235,139,185,235,139,131,235,139,147,235,137,
  135,235,131,252,248,4,139,131,235,137,151,235,137,135,235,15,131,245,251,
  249,4,254,2,249,5,252,246,130,235,237,15,132,245,4,252,246,129,235,237,15,
  132,245,4,232,245,29,233,245,4,254,0,251,15,249,29,137,84,36,12,137,76,36,
  8,137,116,36,4,233,244,255,251,15,249,30,139,142,235,139,185,235,139,135,
  235,139,184,235,233,245,255,255,251,15,249,31,131,191,235,5,139,191,235,15,
  133,245,18,249,9,15,182,143,235,184,1,0,0,0,211,224,72,35,130,235,193,224,
  5,3,135,235,249,1,131,184,235,4,15,133,245,248,57,144,235,15,133,245,248,
  139,136,235,133,201,15,132,245,249,255,252,243,15,126,128,235,102,15,214,
  131,235,255,139,144,235,139,184,235,137,147,235,137,187,235,255,137,139,235,
  139,158,235,195,249,2,139,128,235,133,192,15,133,245,1,49,201,249,3,139,135,
  235,133,192,15,132,245,250,252,246,128,235,237,15,132,245,251,249,4,137,139,
  235,139,158,235,195,249,5,137,150,235,199,134,235,4,0,0,0,139,12,36,131,252,
  236,12,137,142,235,137,52,36,137,124,36,4,137,92,36,8,232,244,131,196,12,
  139,158,235,255,251,15,249,32,139,135,235,193,224,4,11,129,235,131,252,248,
  84,139,191,235,139,145,235,15,132,245,9,233,245,18,255,251,15,249,33,139,
  142,235,128,167,235,237,139,145,235,137,185,235,137,151,235,195,255,251,15,
  249,34,139,142,235,139,185,235,139,135,235,139,184,235,233,245,255,255,251,
  15,249,35,131,191,235,5,139,191,235,15,133,245,18,249,9,15,182,143,235,184,
  1,0,0,0,211,224,72,35,130,235,193,224,5,3,135,235,249,1,131,184,235,4,15,
  133,245,250,57,144,235,15,133,245,250,131,184,235,0,15,132,245,252,249,2,
  198,135,235,0,249,3,255,252,246,135,235,237,15,133,245,254,249,7,255,139,
  139,235,252,243,15,126,131,235,137,136,235,102,15,214,128,235,255,139,139,
  235,139,147,235,139,187,235,137,136,235,137,144,235,137,184,235,255,139,158,
  235,195,249,8,232,245,33,233,245,7,249,4,139,128,235,133,192,15,133,245,1,
  139,143,235,133,201,15,132,245,251,252,246,129,235,237,15,132,245,253,249,
  5,141,134,235,137,144,235,199,128,235,4,0,0,0,131,252,236,12,137,52,36,137,
  124,36,4,137,68,36,8,232,244,131,196,12,233,245,2,249,6,255,139,143,235,133,
  201,15,132,245,2,252,246,129,235,237,15,133,245,2,249,7,137,150,235,199,134,
  235,4,0,0,0,139,12,36,131,252,236,12,137,142,235,137,52,36,137,124,36,4,137,
  92,36,8,232,244,131,196,12,139,158,235,195,255,251,15,249,36,139,135,235,
  193,224,4,11,129,235,131,252,248,84,139,191,235,139,145,235,15,132,245,9,
  233,245,18,255,137,52,36,199,68,36,4,239,199,68,36,8,239,232,244,137,131,
  235,199,131,235,5,0,0,0,255,186,239,255,232,245,30,255,232,245,34,255,141,
  187,235,186,239,255,141,187,235,141,139,235,255,131,187,235,5,139,187,235,
  15,133,245,255,185,239,139,135,235,59,143,235,15,135,245,251,255,139,131,
  235,193,224,4,11,131,235,131,252,248,83,15,133,245,255,255,252,242,15,16,
  131,235,252,242,15,44,192,252,242,15,42,200,72,102,15,46,200,139,187,235,
  15,133,245,255,15,138,245,255,255,221,131,235,219,20,36,219,4,36,255,223,
  233,221,216,255,218,233,223,224,158,255,15,133,245,255,15,138,245,255,139,
  4,36,139,187,235,72,255,59,135,235,15,131,245,251,193,224,4,3,135,235,255,
  232,245,31,255,232,245,32,255,185,239,255,141,147,235,255,199,134,235,239,
  83,81,82,86,232,244,131,196,16,139,158,235,255,249,1,139,144,235,133,210,
  15,132,245,252,255,139,136,235,139,128,235,137,139,235,137,131,235,255,249,
  2,137,147,235,254,2,232,245,37,255,232,245,38,255,233,245,1,249,6,139,143,
  235,133,201,15,132,245,2,252,246,129,235,237,15,133,245,2,249,9,186,239,233,
  245,19,254,0,251,15,249,37,137,76,36,4,131,252,236,12,137,60,36,137,76,36,
  4,232,244,131,196,12,139,76,36,4,193,225,4,41,200,129,192,241,195,255,251,
  15,249,38,64,137,124,36,4,137,68,36,8,233,244,255,187,239,255,232,245,35,
  255,232,245,36,255,199,134,235,239,82,81,83,86,232,244,131,196,16,139,158,
  235,255,249,1,131,184,235,0,15,132,245,252,249,2,254,2,232,245,39,255,232,
  245,40,255,252,246,135,235,237,15,133,245,253,249,3,254,2,249,7,232,245,33,
  233,245,3,254,0,199,128,235,0,0,0,0,255,186,1,0,0,0,137,144,235,137,144,235,
  255,199,128,235,0,0,0,0,199,128,235,1,0,0,0,255,221,152,235,199,128,235,3,
  0,0,0,255,199,128,235,239,199,128,235,4,0,0,0,255,251,15,249,39,137,76,36,
  4,131,252,236,12,137,52,36,137,124,36,4,137,76,36,8,232,244,131,196,12,139,
  76,36,4,193,225,4,41,200,129,192,241,195,255,251,15,249,40,64,137,116,36,
  4,137,124,36,8,137,68,36,12,233,244,255,137,190,235,141,131,235,41,252,248,
  252,247,216,193,252,248,4,139,187,235,15,132,245,250,255,129,192,241,255,
  57,135,235,15,131,245,247,137,52,36,137,124,36,4,137,68,36,8,232,244,249,
  1,252,246,135,235,237,139,151,235,15,133,245,252,139,190,235,254,2,249,6,
  232,245,33,233,245,1,254,0,139,187,235,129,191,235,241,15,130,245,251,249,
  1,252,246,135,235,237,139,151,235,15,133,245,252,141,187,235,254,2,249,5,
  137,52,36,137,124,36,4,199,68,36,8,239,232,244,233,245,1,249,6,232,245,33,
  233,245,1,254,0,129,194,241,255,141,139,235,249,3,139,1,131,193,4,137,2,131,
  194,4,57,252,249,15,130,245,3,249,4,255,131,187,235,3,139,131,235,15,133,
  245,255,133,192,15,136,245,255,255,221,131,235,221,5,239,255,221,5,239,221,
  131,235,255,139,131,235,193,224,4,11,131,235,131,252,248,51,139,131,235,15,
  133,245,255,11,131,235,15,136,245,255,221,131,235,221,131,235,255,131,187,
  235,3,15,133,245,255,221,131,235,255,216,200,255,217,192,216,200,255,220,
  201,255,222,201,255,199,4,36,239,199,68,36,4,239,199,68,36,8,239,131,187,
  235,3,15,133,245,255,219,44,36,220,139,235,217,192,217,252,252,220,233,217,
  201,217,252,240,217,232,222,193,217,252,253,221,217,255,251,15,249,41,217,
  232,221,68,36,8,217,252,241,139,68,36,4,219,56,195,255,131,187,235,3,15,133,
  245,255,255,131,187,235,3,255,139,131,235,193,224,4,11,131,235,131,252,248,
  51,255,216,192,255,220,131,235,255,220,163,235,255,220,171,235,255,220,139,
  235,255,220,179,235,255,220,187,235,255,131,252,236,16,221,28,36,221,131,
  235,221,92,36,8,232,244,131,196,16,255,131,252,236,16,221,92,36,8,221,131,
  235,221,28,36,232,244,131,196,16,255,217,224,255,15,138,246,255,15,130,246,
  255,15,134,246,255,15,135,246,255,15,131,246,255,199,134,235,239,137,52,36,
  137,76,36,4,137,84,36,8,232,244,133,192,139,158,235,255,15,132,246,255,199,
  134,235,239,199,4,36,239,82,81,83,86,232,244,131,196,16,139,158,235,255,131,
  187,235,5,139,139,235,15,133,245,9,137,12,36,232,244,137,4,36,219,4,36,221,
  155,235,199,131,235,3,0,0,0,255,131,187,235,4,139,139,235,15,133,245,9,219,
  129,235,221,155,235,199,131,235,3,0,0,0,255,199,134,235,239,137,52,36,137,
  92,36,4,137,76,36,8,232,244,139,158,235,255,139,131,235,139,139,235,186,1,
  0,0,0,33,193,209,232,9,193,49,192,57,209,17,192,137,147,235,137,131,235,255,
  232,245,42,137,131,235,199,131,235,4,0,0,0,255,199,134,235,239,137,52,36,
  199,68,36,4,239,199,68,36,8,239,232,244,139,158,235,255,251,15,249,42,137,
  116,36,4,139,131,235,193,224,4,11,131,235,131,232,68,15,133,245,18,249,1,
  139,190,235,139,179,235,139,147,235,139,142,235,133,201,15,132,245,248,11,
  130,235,15,132,245,250,1,200,15,130,245,255,59,135,235,15,135,245,251,139,
  191,235,129,198,241,255,252,243,164,139,138,235,141,178,235,252,243,164,41,
  199,139,116,36,4,137,124,36,8,137,68,36,12,139,158,235,233,244,249,2,137,
  208,249,3,139,116,36,4,139,158,235,195,249,4,137,252,240,233,245,3,249,5,
  139,116,36,4,141,143,235,131,252,236,12,137,52,36,137,76,36,4,137,68,36,8,
  232,244,131,196,12,49,192,233,245,1,249,9,139,116,36,4,233,245,18,255,131,
  187,235,0,255,139,131,235,139,139,235,72,73,9,200,255,139,131,235,72,11,131,
  235,255,131,187,235,3,15,133,246,221,131,235,221,5,239,255,131,187,235,4,
  15,133,246,129,187,235,239,255,139,131,235,59,131,235,15,133,246,255,131,
  252,248,3,15,133,245,9,221,131,235,221,131,235,255,131,252,248,4,15,133,245,
  9,139,139,235,59,139,235,255,141,147,235,141,139,235,199,134,235,239,137,
  52,36,137,76,36,4,137,84,36,8,232,244,72,139,158,235,255,139,131,235,139,
  139,235,137,194,33,202,141,20,80,209,234,255,15,132,245,247,255,15,133,245,
  247,255,139,147,235,137,131,235,137,139,235,137,147,235,233,246,249,1,255,
  139,131,235,193,224,4,11,131,235,131,252,248,51,15,133,245,255,249,4,221,
  131,235,221,131,235,221,147,235,255,249,4,139,131,235,193,224,4,11,131,235,
  193,224,4,11,131,235,61,51,3,0,0,139,131,235,15,133,245,255,221,131,235,221,
  131,235,133,192,221,147,235,15,136,245,247,217,201,249,1,255,199,131,235,
  3,0,0,0,15,130,246,255,249,9,141,131,235,199,134,235,239,137,52,36,137,68,
  36,4,232,244,233,245,4,254,0,221,131,235,221,131,235,220,131,235,221,147,
  235,221,147,235,199,131,235,3,0,0,0,255,139,131,235,221,131,235,221,131,235,
  221,131,235,222,193,221,147,235,221,147,235,199,131,235,3,0,0,0,133,192,15,
  136,245,247,217,201,249,1,255,131,187,235,0,15,132,245,247,255,141,131,235,
  137,68,36,4,255,137,92,36,4,255,139,187,235,255,139,142,235,139,185,235,139,
  191,235,255,139,151,235,137,52,36,199,68,36,4,239,137,84,36,8,232,244,199,
  128,235,239,137,131,235,199,131,235,6,0,0,0,255,139,151,235,137,144,235,255,
  137,52,36,232,244,137,135,235,255,249,1,139,142,235,139,145,235,129,194,241,
  141,132,253,27,235,41,208,59,134,235,15,131,245,251,141,187,235,57,218,15,
  131,245,249,249,2,139,2,131,194,4,137,7,131,199,4,57,218,15,130,245,2,249,
  3,254,2,249,5,43,134,235,193,252,248,4,137,52,36,137,68,36,4,232,244,139,
  158,235,233,245,1,254,0,139,142,235,139,145,235,129,194,241,141,187,235,141,
  139,235,57,218,15,131,245,248,249,1,139,2,131,194,4,137,7,131,199,4,57,207,
  15,131,245,250,57,218,15,130,245,1,249,2,49,192,249,3,137,135,235,129,199,
  241,57,207,15,130,245,3,249,4,255
};

enum {
  JSUB_STACKPTR,
  JSUB_GATE_LJ,
  JSUB_GATE_JL,
  JSUB_GATE_JC,
  JSUB_GROW_STACK,
  JSUB_GROW_CI,
  JSUB_GATE_JC_PATCH,
  JSUB_GATE_JC_DEBUG,
  JSUB_DEOPTIMIZE_CALLER,
  JSUB_DEOPTIMIZE,
  JSUB_DEOPTIMIZE_OPEN,
  JSUB_HOOKINS,
  JSUB_GCSTEP,
  JSUB_STRING_SUB3,
  JSUB_STRING_SUB2,
  JSUB_HOOKCALL,
  JSUB_HOOKRET,
  JSUB_METACALL,
  JSUB_METATAILCALL,
  JSUB_BARRIERF,
  JSUB_GETGLOBAL,
  JSUB_GETTABLE_KSTR,
  JSUB_GETTABLE_STR,
  JSUB_BARRIERBACK,
  JSUB_SETGLOBAL,
  JSUB_SETTABLE_KSTR,
  JSUB_SETTABLE_STR,
  JSUB_GETTABLE_KNUM,
  JSUB_GETTABLE_NUM,
  JSUB_SETTABLE_KNUM,
  JSUB_SETTABLE_NUM,
  JSUB_LOG2_TWORD,
  JSUB_CONCAT_STR2,
  JSUB__MAX
};

/* ------------------------------------------------------------------------ */

/* Arch string. */
const char luaJIT_arch[] = "x86";

/* Forward declarations for C functions called from jsubs. */
static void jit_hookins(lua_State *L, const Instruction *newpc);
static void jit_gettable_fb(lua_State *L, Table *t, StkId dest);
static void jit_settable_fb(lua_State *L, Table *t, StkId val);

/* ------------------------------------------------------------------------ */

/* Detect CPU features and set JIT flags. */
static int jit_cpudetect(jit_State *J)
{
  void *mcode;
  size_t sz;
  int status;
  /* Some of the jsubs need the flags. So compile this separately. */
  unsigned int feature;
  dasm_setup(Dst, jit_actionlist);
  dasm_put(Dst, 0);
  (void)dasm_checkstep(Dst, DASM_SECTION_CODE);
  status = luaJIT_link(J, &mcode, &sz);
  if (status != JIT_S_OK)
    return status;
  /* Check feature bits. See the Intel/AMD manuals for the bit definitions. */
  feature = ((unsigned int (*)(void))mcode)();
  if (feature & (1<<15)) J->flags |= JIT_F_CPU_CMOV;
  if (feature & (1<<26)) J->flags |= JIT_F_CPU_SSE2;
  luaJIT_freemcode(J, mcode, sz);  /* We don't need this code anymore. */
  return JIT_S_OK;
}

/* Check some assumptions. Should compile to nop. */
static int jit_consistency_check(jit_State *J)
{
  do {
    /* Force a compiler error for inconsistent structure sizes. */
    /* Check LUA_TVALUE_ALIGN in luaconf.h, too. */
    int check_TVALUE_SIZE_in_ljit_x86_dash[1+16-sizeof(TValue)];
    int check_TVALUE_SIZE_in_ljit_x86_dash_[1+sizeof(TValue)-16];
    ((void)check_TVALUE_SIZE_in_ljit_x86_dash[0]);
    ((void)check_TVALUE_SIZE_in_ljit_x86_dash_[0]);
    if (LUA_TNIL != 0 || LUA_TBOOLEAN != 1 || PCRLUA != 0) break;
    if ((int)&(((Node *)0)->i_val) != (int)&(((StkId)0)->value)) break;
    return JIT_S_OK;
  } while (0);
  J->dasmstatus = 999999999;  /* Recognizable error. */
  return JIT_S_COMPILER_ERROR;
}

/* Compile JIT subroutines (once). */
static int jit_compile_jsub(jit_State *J)
{
  int status = jit_consistency_check(J);
  if (status != JIT_S_OK) return status;
  status = jit_cpudetect(J);
  if (status != JIT_S_OK) return status;
  dasm_setup(Dst, jit_actionlist);
  dasm_put(Dst, 34);
  dasm_put(Dst, 36, Dt1(->top), Dt2(->value), Dt1(->ci), Dt1(->nCcalls), Dt5(->jit_gate), Dt1(->ci), Dt1(->top), Dt4(->savedpc), Dt1(->savedpc), Dt4(->base), Dt1(->base), Dt1(->top), Dt3(->tt), sizeof(TValue));
  dasm_put(Dst, 145, Dt1(->nCcalls), PCRC, Dt5(->p), DtE(->jit_status), JIT_S_OK, DtE(->jit_mcode), Dt5(->jit_gate), Dt4(->savedpc), Dt1(->ci), Dt1(->top), Dt1(->savedpc), Dt1(->stack), (ptrdiff_t)(luaD_precall), (ptrdiff_t)(luaV_execute), Dt1(->stack));
  dasm_put(Dst, 262, Dt1(->top), Dt3([LUA_MINSTACK]), Dt1(->stack_last), Dt1(->end_ci), Dt4([1]), Dt1(->ci), Dt4(->func), Dt4(->top), Dt2(->value), sizeof(TValue), Dt1(->top), Dt1(->base), Dt4(->base), DtD(->f), Dt1(->ci));
  dasm_put(Dst, 336, Dt4(->func), Dt1(->top), Dt4(->func), sizeof(CallInfo), Dt1(->ci), Dt1(->hookmask), LUA_MASKCALL, DtD(->f), Dt1(->hookmask), LUA_MASKRET);
  dasm_put(Dst, 421, LUA_HOOKRET, (ptrdiff_t)(luaD_callhook), LUA_HOOKCALL, (ptrdiff_t)(luaD_callhook), Dt1(->top), Dt1(->stack), (ptrdiff_t)(luaD_growstack), Dt1(->stack), Dt1(->top), Dt2(->value), Dt5(->jit_gate), Dt1(->top), (ptrdiff_t)(luaD_growCI), Dt9([-1]));
  dasm_put(Dst, 547, Dt2(->value), Dt1(->ci), Dt5(->jit_gate));
  dasm_put(Dst, 602, Dt1(->hookmask), LUA_MASKLINE|LUA_MASKCOUNT, Dt1(->hookcount), Dt1(->hookmask), LUA_MASKLINE, (ptrdiff_t)(jit_hookins), Dt1(->base), Dt1(->top));
  dasm_put(Dst, 737, (ptrdiff_t)(luaC_step), Dt1(->base));
  dasm_put(Dst, 1026, Dt3([0].tt), Dt3([1].tt), Dt3([2].tt), Dt3([1].value), Dt3([2].value), Dt3([0].value), DtB(->tsv.len), sizeof(TString)-1, Dt1(->l_G), Dt6(->totalbytes));
  dasm_put(Dst, 1129, Dt6(->GCthreshold), (ptrdiff_t)(luaS_newlstr));
  dasm_put(Dst, 1191, Dt3([0].tt), Dt3([1].tt), Dt3([1].value), Dt3([0].value), DtB(->tsv.len), (ptrdiff_t)(luaC_step), Dt1(->base), (ptrdiff_t)(luaS_newlstr));
    dasm_put(Dst, 1755, Dt1(->ci), Dt4(->func), Dt3(->value), Dt5(->p), DtE(->code), Dt1(->savedpc), LUA_HOOKCALL, (ptrdiff_t)(luaD_callhook), DtE(->code), Dt1(->savedpc), Dt1(->base));
    dasm_put(Dst, 1886, Dt1(->savedpc), LUA_HOOKRET, (ptrdiff_t)(luaD_callhook), Dt1(->base), Dt1(->top));
    dasm_put(Dst, 2077, Dt1(->savedpc), Dt1(->top), (ptrdiff_t)(luaD_tryfuncTM), Dt1(->top), Dt2(->value), Dt1(->ci));
    dasm_put(Dst, 2178, Dt1(->savedpc), Dt1(->top), (ptrdiff_t)(luaD_tryfuncTM), Dt1(->ci), Dt1(->top), Dt4(->func), Dt4(->func), Dt2(->value), sizeof(CallInfo), Dt5(->jit_gate));
  dasm_put(Dst, 2570, (ptrdiff_t)(luaC_barrierf));
  dasm_put(Dst, 2589, Dt1(->ci), Dt4(->func), Dt3(->value), Dt5(->env));
  dasm_put(Dst, 2609, Dt3(->tt), Dt3(->value), DtC(->lsizenode), DtB(->tsv.hash), DtC(->node), Dt10(->i_key.nk.tt), Dt10(->i_key.nk.value), Dt10(->i_val.tt));
  if (J->flags & JIT_F_CPU_SSE2) {
  dasm_put(Dst, 2674, Dt10(->i_val.value), Dt2(->value));
  } else {
  dasm_put(Dst, 2686, Dt10(->i_val.value), Dt10(->i_val.value.na[1]), Dt2(->value), Dt2(->value.na[1]));
  }
  dasm_put(Dst, 2699, Dt2(->tt), Dt1(->base), Dt10(->i_key.nk.next), DtC(->metatable), DtC(->flags), 1<<TM_INDEX, Dt2([0].tt), Dt1(->base), Dt1(->env.value), Dt1(->env.tt), Dt1(->savedpc), (ptrdiff_t)(jit_gettable_fb), Dt1(->base));
  dasm_put(Dst, 32);
  dasm_put(Dst, 2790, Dt3(->tt), Dt7(->tt), Dt3(->value), Dt7(->value));
  dasm_put(Dst, 2821, Dt1(->l_G), DtC(->marked), (~bitmask(BLACKBIT))&0xff, Dt6(->grayagain), Dt6(->grayagain), DtC(->gclist));
  dasm_put(Dst, 2843, Dt1(->ci), Dt4(->func), Dt3(->value), Dt5(->env));
  dasm_put(Dst, 2863, Dt3(->tt), Dt3(->value), DtC(->lsizenode), DtB(->tsv.hash), DtC(->node), Dt10(->i_key.nk.tt), Dt10(->i_key.nk.value), Dt10(->i_val.tt), DtC(->flags));
  dasm_put(Dst, 2935, DtC(->marked), bitmask(BLACKBIT));
  if (J->flags & JIT_F_CPU_SSE2) {
  dasm_put(Dst, 2947, Dt2([0].tt), Dt2([0].value), Dt7([0].tt), Dt7([0].value));
  } else {
  dasm_put(Dst, 2965, Dt2([0].value), Dt2([0].value.na[1]), Dt2([0].tt), Dt7([0].value), Dt7([0].value.na[1]), Dt7([0].tt));
  }
  dasm_put(Dst, 2984, Dt1(->base), Dt10(->i_key.nk.next), DtC(->metatable), DtC(->flags), 1<<TM_NEWINDEX, Dt1(->env), Dt7([0].value), Dt7([0].tt), (ptrdiff_t)(luaH_newkey));
  dasm_put(Dst, 3066, DtC(->metatable), DtC(->flags), 1<<TM_NEWINDEX, Dt1(->env.value), Dt1(->env.tt), Dt1(->savedpc), (ptrdiff_t)(jit_settable_fb), Dt1(->base));
  dasm_put(Dst, 3127, Dt3(->tt), Dt7(->tt), Dt3(->value), Dt7(->value));
  dasm_put(Dst, 3438, (ptrdiff_t)(luaH_getnum), sizeof(TValue));
  dasm_put(Dst, 3476, (ptrdiff_t)(luaH_getnum));
  dasm_put(Dst, 3623, (ptrdiff_t)(luaH_setnum), sizeof(TValue));
  dasm_put(Dst, 3665, (ptrdiff_t)(luaH_setnum));
      dasm_put(Dst, 3992);
  dasm_put(Dst, 4325, Dt2([0].tt), Dt2([1].tt), Dt1(->l_G), Dt2([0].value), Dt2([1].value), DtB(->tsv.len), DtB(->tsv.len), Dt6(->buff.buffsize), Dt6(->buff.buffer), sizeof(TString));
  dasm_put(Dst, 4396, DtB(->tsv.len), DtB([1]), Dt1(->base), (ptrdiff_t)(luaS_newlstr), Dt1(->base), Dt6(->buff), (ptrdiff_t)(luaZ_openspace));
  dasm_put(Dst, 561, Dt1(->top), Dt1(->savedpc), (ptrdiff_t)(luaJIT_deoptimize), Dt1(->base), Dt1(->top));

  (void)dasm_checkstep(Dst, DASM_SECTION_CODE);
  status = luaJIT_link(J, &J->jsubmcode, &J->szjsubmcode);
  if (status != JIT_S_OK)
    return status;

  /* Copy the callgates from the globals to the global state. */
  G(J->L)->jit_gateLJ = (luaJIT_GateLJ)J->jsub[JSUB_GATE_LJ];
  G(J->L)->jit_gateJL = (lua_CFunction)J->jsub[JSUB_GATE_JL];
  G(J->L)->jit_gateJC = (lua_CFunction)J->jsub[JSUB_GATE_JC];
  return JIT_S_OK;
}

/* Match with number of nops above. Avoid confusing the instruction decoder. */
#define DEBUGPATCH_SIZE		6

/* Notify backend that the debug mode may have changed. */
void luaJIT_debugnotify(jit_State *J)
{
  unsigned char *patch = (unsigned char *)J->jsub[JSUB_GATE_JC_PATCH];
  unsigned char *target = (unsigned char *)J->jsub[JSUB_GATE_JC_DEBUG];
  /* Yep, this is self-modifying code -- don't tell anyone. */
  if (patch[0] == 0xe9) {  /* Debug patch is active. */
    if (!(J->flags & JIT_F_DEBUG_CALL))  /* Deactivate it. */
      memcpy(patch, target-DEBUGPATCH_SIZE, DEBUGPATCH_SIZE);
  } else {  /* Debug patch is inactive. */
    if (J->flags & JIT_F_DEBUG_CALL) {  /* Activate it. */
      int rel = target-(patch+5);
      memcpy(target-DEBUGPATCH_SIZE, patch, DEBUGPATCH_SIZE);
      patch[0] = 0xe9;  /* jmp */
      memcpy(patch+1, &rel, 4);  /* Relative address. */
      memset(patch+5, 0x90, DEBUGPATCH_SIZE-5);  /* nop */
    }
  }
}

/* Patch a jmp into existing mcode. */
static void jit_patch_jmp(jit_State *J, void *mcode, void *to)
{
  unsigned char *patch = (unsigned char *)mcode;
  int rel = ((unsigned char *)to)-(patch+5);
  patch[0] = 0xe9;  /* jmp */
  memcpy((void *)(patch+1), &rel, 4);  /* Relative addr. */
}

/* ------------------------------------------------------------------------ */

/* Call line/count hook. */
static void jit_hookins(lua_State *L, const Instruction *newpc)
{
  Proto *pt = ci_func(L->ci)->l.p;
  int pc = luaJIT_findpc(pt, newpc);  /* Sloooow with mcode addrs. */
  const Instruction *savedpc = L->savedpc;
  L->savedpc = pt->code + pc + 1;
  if (L->hookmask > LUA_MASKLINE && L->hookcount == 0) {
    resethookcount(L);
    luaD_callhook(L, LUA_HOOKCOUNT, -1);
  }
  if (L->hookmask & LUA_MASKLINE) {
    int newline = getline(pt, pc);
    if (pc != 0) {
      int oldpc = luaJIT_findpc(pt, savedpc);
      if (!(pc <= oldpc || newline != getline(pt, oldpc))) return;
    }
    luaD_callhook(L, LUA_HOOKLINE, newline);
  }
}

/* Insert hook check for each instruction in full debug mode. */
static void jit_ins_debug(jit_State *J, int openop)
{
  if (openop) {
    dasm_put(Dst, 594, Dt1(->top));
  }
  dasm_put(Dst, 598);

}

/* Called before every instruction. */
static void jit_ins_start(jit_State *J)
{
  dasm_put(Dst, 663, J->nextpc);
}

/* Chain to another instruction. */
static void jit_ins_chainto(jit_State *J, int pc)
{
  dasm_put(Dst, 665, pc);
}

/* Set PC label. */
static void jit_ins_setpc(jit_State *J, int pc, void *target)
{
  dasm_put(Dst, 668, pc, (ptrdiff_t)(target));
}

/* Called after the last instruction has been encoded. */
static void jit_ins_last(jit_State *J, int lastpc, int sizemfm)
{
  if (J->tflags & JIT_TF_USED_DEOPT) {  /* Deopt section has been used? */
    dasm_put(Dst, 671);
    dasm_put(Dst, 673);
  }
  dasm_put(Dst, 678, lastpc+1);
  dasm_put(Dst, 681, lastpc+2);
  dasm_put(Dst, 690, sizemfm);
}

/* Add a deoptimize target for the current instruction. */
static void jit_deopt_target(jit_State *J, int nargs)
{
  if (nargs != -1) {
    dasm_put(Dst, 671);
    dasm_put(Dst, 702, (ptrdiff_t)(J->nextins));
    J->tflags |= JIT_TF_USED_DEOPT;
  } else {
    dasm_put(Dst, 679);
    dasm_put(Dst, 709, (ptrdiff_t)(J->nextins));
  }
}

/* luaC_checkGC() inlined. Destroys caller-saves + TOP (edi). Uses label 7:. */
/* Use this only at the _end_ of an instruction. */
static void jit_checkGC(jit_State *J)
{
  dasm_put(Dst, 718, Dt1(->l_G), Dt6(->totalbytes), Dt6(->GCthreshold));

}

/* ------------------------------------------------------------------------ */



/*
** Function inlining support for x86 CPUs.
** Copyright (C) 2005-2008 Mike Pall. See Copyright Notice in luajit.h
*/

/* ------------------------------------------------------------------------ */

/* Private structure holding function inlining info. */
typedef struct jit_InlineInfo {
  int func;			/* Function slot. 1st arg slot = func+1. */
  int res;			/* 1st result slot. Overlaps func/ci->func. */
  int nargs;			/* Number of args. */
  int nresults;			/* Number of results. */
  int xnargs;			/* Expected number of args. */
  int xnresults;		/* Returned number of results. */
  int hidx;			/* Library/function index numbers. */
} jit_InlineInfo;

/* ------------------------------------------------------------------------ */

enum { TFOR_FUNC, TFOR_TAB, TFOR_CTL, TFOR_KEY, TFOR_VAL };

static void jit_inline_base(jit_State *J, jit_InlineInfo *ii)
{
  int func = ii->func;
  switch (JIT_IH_IDX(ii->hidx)) {
  case JIT_IH_BASE_PAIRS:
  case JIT_IH_BASE_IPAIRS:
    dasm_put(Dst, 753, Dt2([func+TFOR_TAB].tt), Dt2([func+TFOR_CTL].tt), Dt2([func+TFOR_CTL].value));
    dasm_put(Dst, 771, JIT_MFM_DEOPT_PAIRS, J->nextpc-1);
    break;
  default:
    jit_assert(0);
    break;
  }
}

/* ------------------------------------------------------------------------ */

#ifndef COCO_DISABLE

/* Helper function for inlined coroutine.resume(). */
static StkId jit_coroutine_resume(lua_State *L, StkId base, int nresults)
{
  lua_State *co = thvalue(base-1);
  /* Check for proper usage. Merge of lua_resume() and auxresume() checks. */
  if (co->status != LUA_YIELD) {
    if (co->status > LUA_YIELD) {
errdead:
      setsvalue(L, base-1, luaS_newliteral(L, "cannot resume dead coroutine"));
      goto err;
    } else if (co->ci != co->base_ci) {
      setsvalue(L, base-1,
	luaS_newliteral(L, "cannot resume non-suspended coroutine"));
      goto err;
    } else if (co->base == co->top) {
      goto errdead;
    }
  }
  {
    unsigned int ndelta = (char *)L->top - (char *)base;
    int nargs = ndelta/sizeof(TValue);  /* Compute nargs. */
    int status;
    if ((char *)co->stack_last-(char *)co->top <= (int)ndelta) {
      co->ci->top = (StkId)(((char *)co->top) + ndelta);  /* Ok before grow. */
      luaD_growstack(co, nargs);  /* Grow thread stack. */
    }
    /* Copy args. */
    co->top = (StkId)(((char *)co->top) + ndelta);
    { StkId t = co->top, f = L->top; while (f > base) setobj2s(co, --t, --f); }
    L->top = base;
    status = luaCOCO_resume(co, nargs);  /* Resume Coco thread. */
    if (status == 0 || status == LUA_YIELD) {  /* Ok. */
      StkId f;
      if (nresults == 0) return NULL;
      if (nresults == -1) {
	luaD_checkstack(L, co->top - co->base);  /* Grow own stack. */
      }
      base = L->top - 2;
      setbvalue(base++, 1);  /* true */
      /* Copy results. Fill unused result slots with nil. */
      f = co->base;
      while (--nresults != 0 && f < co->top) setobj2s(L, base++, f++);
      while (nresults-- > 0) setnilvalue(base++);
      co->top = co->base;
      return base;
    } else {  /* Error. */
      base = L->top;
      setobj2s(L, base-1, co->top-1);  /* Copy error object. */
err:
      setbvalue(base-2, 0);  /* false */
      nresults -= 2;
      while (--nresults >= 0) setnilvalue(base+nresults);  /* Fill results. */
      return base;
    }
  }
}

static void jit_inline_coroutine(jit_State *J, jit_InlineInfo *ii)
{
  int arg = ii->func+1;
  int res = ii->res;
  int i;
  switch (JIT_IH_IDX(ii->hidx)) {
  case JIT_IH_COROUTINE_YIELD:
    dasm_put(Dst, 775, ((int)&LHASCOCO((lua_State *)0)), Dt1(->savedpc), (ptrdiff_t)(J->nextins), arg*sizeof(TValue));
    if (ii->nargs >= 0) {  /* Previous op was not open and did not set TOP. */
      dasm_put(Dst, 791, Dt2([ii->nargs]));
    }
    dasm_put(Dst, 795, Dt1(->base), Dt1(->top), (ptrdiff_t)(luaCOCO_yield), Dt1(->base), Dt1(->top));
    jit_assert(ii->nresults >= 0 && ii->nresults <= EXTRA_STACK);
    for (i = 0; i < ii->nresults; i++) {
      dasm_put(Dst, 813, Dt3([i].tt));
      if (J->flags & JIT_F_CPU_SSE2) {
      dasm_put(Dst, 821, Dt2([arg+i].tt), Dt2([arg+i].value), Dt2([res+i].tt), Dt2([res+i].value));
      } else {
      dasm_put(Dst, 839, Dt2([arg+i].value), Dt2([arg+i].value.na[1]), Dt2([arg+i].tt), Dt2([res+i].value), Dt2([res+i].value.na[1]), Dt2([res+i].tt));
      }
    }
    ii->nargs = -1;  /* Force restore of L->top. */
    break;
  case JIT_IH_COROUTINE_RESUME:
    jit_assert(ii->nargs != 0 && ii->res == ii->func);
    dasm_put(Dst, 787, (arg+1)*sizeof(TValue));
    if (ii->nargs >= 0) {  /* Previous op was not open and did not set TOP. */
      dasm_put(Dst, 791, Dt2([ii->nargs-1]));
    } else {
      dasm_put(Dst, 858);
    }
    dasm_put(Dst, 865, Dt2([-1].tt), Dt2([-1].value), ((int)&LHASCOCO((lua_State *)0)), Dt1(->savedpc), (ptrdiff_t)(J->nextins), Dt1(->top), ii->nresults, (ptrdiff_t)(jit_coroutine_resume), Dt1(->base));
    if (ii->nresults == -1) {
      dasm_put(Dst, 909);
    }
    ii->nargs = -1;  /* Force restore of L->top. */
    break;
  default:
    jit_assert(0);
    break;
  }
}

#endif /* COCO_DISABLE */

/* ------------------------------------------------------------------------ */

static void jit_inline_string(jit_State *J, jit_InlineInfo *ii)
{
  int arg = ii->func+1;
  int res = ii->res;
  switch (JIT_IH_IDX(ii->hidx)) {
  case JIT_IH_STRING_LEN:
    dasm_put(Dst, 912, Dt2([arg].tt), Dt2([arg].value), DtB(->tsv.len), Dt2([res].tt), Dt2([res].value));
    break;
  case JIT_IH_STRING_SUB:
    /* TODO: inline numeric constants with help from the optimizer. */
    /*       But this would save only another 15-20% in a trivial loop. */
    jit_assert(ii->nargs >= 2);  /* Open op caveat is ok, too. */
    if (ii->nargs > 2) {
      dasm_put(Dst, 937, Dt2([arg]), Dt2([res].value), Dt2([res].tt));
    } else {
      dasm_put(Dst, 954, Dt2([arg]), Dt2([res].value), Dt2([res].tt));
    }
    break;
  case JIT_IH_STRING_CHAR:
    dasm_put(Dst, 971, Dt2([arg].tt), Dt1(->env), Dt2([arg].value), (ptrdiff_t)(luaS_newlstr), Dt2([res].value), Dt2([res].tt));
    break;
  default:
    jit_assert(0);
    break;
  }

}

/* ------------------------------------------------------------------------ */

/* Helper functions for inlined calls to table.*. */
static void jit_table_insert(lua_State *L, TValue *arg)
{
  setobj2t(L, luaH_setnum(L, hvalue(arg), luaH_getn(hvalue(arg))+1), arg+1);
  luaC_barriert(L, hvalue(arg), arg+1);
}

static TValue *jit_table_remove(lua_State *L, TValue *arg, TValue *res)
{
  int n = luaH_getn(hvalue(arg));
  if (n == 0) {
    setnilvalue(res);  /* For the nresults == 1 case. Harmless otherwise. */
    return res;  /* For the nresults == -1 case. */
  } else {
    TValue *val = luaH_setnum(L, hvalue(arg), n);
    setobj2s(L, res, val);
    setnilvalue(val);
    return res+1;  /* For the nresults == -1 case. */
  }
}

static void jit_inline_table(jit_State *J, jit_InlineInfo *ii)
{
  int arg = ii->func+1;
  int res = ii->res;
  dasm_put(Dst, 1250, Dt2([arg].tt));
  switch (JIT_IH_IDX(ii->hidx)) {
  case JIT_IH_TABLE_INSERT:
    jit_assert(ii->nargs == 2);
    dasm_put(Dst, 1259, Dt2([arg]), (ptrdiff_t)(jit_table_insert));
    break;
  case JIT_IH_TABLE_REMOVE:
    jit_assert(ii->nargs == 1);
    dasm_put(Dst, 1272, Dt2([arg]), Dt2([res]), (ptrdiff_t)(jit_table_remove));
    if (ii->nresults == -1) {
      ii->xnresults = -1;
      dasm_put(Dst, 909);
    }
    break;
  case JIT_IH_TABLE_GETN:
    dasm_put(Dst, 1292, Dt2([arg].value), (ptrdiff_t)(luaH_getn), Dt2([res].value), Dt2([res].tt));
    break;
  default:
    jit_assert(0);
    break;
  }
}

/* ------------------------------------------------------------------------ */

/* This typedef must match the libm function signature. */
/* Serves as a check against wrong lua_Number or wrong calling conventions. */
typedef lua_Number (*mathfunc_11)(lua_Number);

/* Partially inlined math functions. */
/* CHECK: must match with jit_hints.h and jit.opt_lib. */
static const mathfunc_11 jit_mathfuncs_11[JIT_IH_MATH_SIN] = {
  log, log10, exp,	sinh, cosh, tanh,	asin, acos, atan
};

/* FPU control words for ceil and floor (exceptions masked, full precision). */
static const unsigned short jit_fpucw[2] = { 0x0b7f, 0x077f };

static void jit_inline_math(jit_State *J, jit_InlineInfo *ii)
{
  int arg = ii->func+1;
  int res = ii->res;
  int idx = JIT_IH_IDX(ii->hidx);

  if (idx < JIT_IH_MATH__21) {
    dasm_put(Dst, 1317, Dt2([arg].tt), Dt2([arg].value));
  } else {
    jit_assert(idx < JIT_IH_MATH__LAST);
    dasm_put(Dst, 1329, Dt2([arg].tt), Dt2([arg+1].tt));
  }
  switch (idx) {
  /* We ignore sin/cos/tan range overflows (2^63 rad) just like -ffast-math. */
  case JIT_IH_MATH_SIN:
    dasm_put(Dst, 1347);
    break;
  case JIT_IH_MATH_COS:
    dasm_put(Dst, 1351);
    break;
  case JIT_IH_MATH_TAN:
    dasm_put(Dst, 1355);
    break;
  case JIT_IH_MATH_CEIL:
  case JIT_IH_MATH_FLOOR:
    dasm_put(Dst, 1361, (ptrdiff_t)&jit_fpucw[idx-JIT_IH_MATH_CEIL]);
    break;
  case JIT_IH_MATH_ABS:
    dasm_put(Dst, 1374);
    break;
  case JIT_IH_MATH_SQRT:
    dasm_put(Dst, 1377);
    break;
  case JIT_IH_MATH_FMOD:
    dasm_put(Dst, 1381, Dt2([arg+1].value), Dt2([arg].value));
    break;
  case JIT_IH_MATH_ATAN2:
    dasm_put(Dst, 1402, Dt2([arg].value), Dt2([arg+1].value));
    break;
  default:
    dasm_put(Dst, 1412, (ptrdiff_t)(jit_mathfuncs_11[idx]));
    break;
  }
  dasm_put(Dst, 926, Dt2([res].tt), Dt2([res].value));
}

/* ------------------------------------------------------------------------ */

/* Try to inline a CALL or TAILCALL instruction. */
static int jit_inline_call(jit_State *J, int func, int nargs, int nresults)
{
  const TValue *callable = hint_get(J, TYPE);  /* TYPE hint = callable. */
  int cltype = ttype(callable);
  const TValue *oidx;
  jit_InlineInfo ii;
  int idx;

  if (cltype != LUA_TFUNCTION) goto fail;
  if (J->flags & JIT_F_DEBUG) goto fail;  /* DWIM. */

  oidx = hint_get(J, INLINE);  /* INLINE hint = library/function index. */
  if (!ttisnumber(oidx)) goto fail;

  ii.hidx = (int)nvalue(oidx);
  idx = JIT_IH_IDX(ii.hidx);

  if (nresults == -2) {  /* Tailcall. */
    /* Tailcalls from vararg functions don't work with BASE[-1]. */
    if (J->pt->is_vararg) goto fail;  /* So forget about this rare case. */
    ii.res = -1;  /* Careful: 2nd result overlaps 1st stack slot. */
    ii.nresults = -1;
  } else {
    ii.res = func;
    ii.nresults = nresults;
  }
  ii.func = func;
  ii.nargs = nargs;
  ii.xnargs = ii.xnresults = 1;  /* Default: 1 arg, 1 result. */

  /* Check for the currently supported cases. */
  switch (JIT_IH_LIB(ii.hidx)) {
  case JIT_IHLIB_BASE:
    switch (idx) {
    case JIT_IH_BASE_PAIRS:
    case JIT_IH_BASE_IPAIRS:
      if (nresults == -2) goto fail;  /* Not useful for tailcalls. */
      ii.xnresults = 3;
      goto check;
    }
    break;
#ifndef COCO_DISABLE
  case JIT_IHLIB_COROUTINE:
    switch (idx) {
    case JIT_IH_COROUTINE_YIELD:
      /* Only support common cases: no tailcalls, low number of results. */
      if (nresults < 0 || nresults > EXTRA_STACK) goto fail;
      ii.xnargs = ii.xnresults = -1;
      goto ok;  /* Anything else is ok. */
    case JIT_IH_COROUTINE_RESUME:
      /* Only support common cases: no tailcalls, not with 0 args (error). */
      if (nresults == -2 || nargs == 0) goto fail;
      ii.xnargs = ii.xnresults = -1;
      goto ok;  /* Anything else is ok. */
    }
    break;
#endif
  case JIT_IHLIB_STRING:
    switch (idx) {
    case JIT_IH_STRING_LEN:
      goto check;
    case JIT_IH_STRING_SUB:
      if (nargs < 2) goto fail;  /* No support for open calls, too. */
      goto ok;  /* 2 or more args are ok. */
    case JIT_IH_STRING_CHAR:
      goto check;  /* Only single arg supported. */
    }
    break;
  case JIT_IHLIB_TABLE:
    switch (idx) {
    case JIT_IH_TABLE_INSERT:
      ii.xnargs = 2;
      goto check;  /* Only push (append) supported. */
    case JIT_IH_TABLE_REMOVE:
      goto check;  /* Only pop supported. */
    case JIT_IH_TABLE_GETN:
      goto check;
    }
    break;
  case JIT_IHLIB_MATH:
    if (idx >= JIT_IH_MATH__LAST) goto fail;
    if (idx >= JIT_IH_MATH__21) ii.xnargs = 2;
    goto check;
  }
fail:
  return cltype;  /* Call could not be inlined. Return type of callable. */

check:
  if (nargs != ii.xnargs && nargs != -1) goto fail;
  /* The optimizer already checks the number of results (avoid setnil). */

ok:  /* Whew, all checks done. Go for it! */

  /* Start with the common leadin for inlined calls. */
  jit_deopt_target(J, nargs);
  dasm_put(Dst, 1418, Dt2([func].tt), Dt2([func].value), (ptrdiff_t)(clvalue(callable)));
  if (nargs == -1 && ii.xnargs >= 0) {
    dasm_put(Dst, 1435, Dt2([func+1+ii.xnargs]));
  }

  /* Now inline the function itself. */
  switch (JIT_IH_LIB(ii.hidx)) {
  case JIT_IHLIB_BASE: jit_inline_base(J, &ii); break;
#ifndef COCO_DISABLE
  case JIT_IHLIB_COROUTINE: jit_inline_coroutine(J, &ii); break;
#endif
  case JIT_IHLIB_STRING: jit_inline_string(J, &ii); break;
  case JIT_IHLIB_TABLE:  jit_inline_table(J, &ii); break;
  case JIT_IHLIB_MATH:   jit_inline_math(J, &ii); break;
  default: jit_assert(0); break;
  }

  /* And add the common leadout for inlined calls. */
  if (ii.nresults == -1) {
    if (ii.xnresults >= 0) {
      dasm_put(Dst, 791, Dt2([ii.res+ii.xnresults]));
    }
  } else if (ii.nargs == -1) {  /* Restore L->top only if needed. */
    dasm_put(Dst, 1445, Dt2([J->pt->maxstacksize]), Dt1(->top));
  }

  if (nresults == -2) {  /* Results are in place. Add return for tailcalls. */
    dasm_put(Dst, 1452, sizeof(TValue), Dt1(->ci), sizeof(CallInfo));
  }

  return -1;  /* Success, call has been inlined. */
}

/* ------------------------------------------------------------------------ */

/* Helper function for inlined iterator code. Paraphrased from luaH_next. */
/* TODO: GCC has trouble optimizing this. */
static int jit_table_next(lua_State *L, TValue *ra)
{
  Table *t = hvalue(&ra[TFOR_TAB]);
  int i = ra[TFOR_CTL].value.b;  /* Hidden control variable. */
  for (; i < t->sizearray; i++) {  /* First the array part. */
    if (!ttisnil(&t->array[i])) {
      setnvalue(&ra[TFOR_KEY], cast_num(i+1));
      setobj2s(L, &ra[TFOR_VAL], &t->array[i]);
      ra[TFOR_CTL].value.b = i+1;
      return 1;
    }
  }
  for (i -= t->sizearray; i < sizenode(t); i++) {  /* Then the hash part. */
    if (!ttisnil(gval(gnode(t, i)))) {
      setobj2s(L, &ra[TFOR_KEY], key2tval(gnode(t, i)));
      setobj2s(L, &ra[TFOR_VAL], gval(gnode(t, i)));
      ra[TFOR_CTL].value.b = i+1+t->sizearray;
      return 1;
    }
  }
  return 0;  /* End of iteration. */
}

/* Try to inline a TFORLOOP instruction. */
static int jit_inline_tforloop(jit_State *J, int ra, int nresults, int target)
{
  const TValue *oidx = hint_get(J, INLINE);  /* INLINE hint = lib/func idx. */
  int idx;

  if (!ttisnumber(oidx)) return 0;  /* No hint: don't inline anything. */
  idx = (int)nvalue(oidx);
  if (J->flags & JIT_F_DEBUG) return 0;  /* DWIM. */

  switch (idx) {
  case JIT_IH_MKIDX(JIT_IHLIB_BASE, JIT_IH_BASE_PAIRS):
    dasm_put(Dst, 1465, Dt2([ra]), (ptrdiff_t)(jit_table_next), target);
    return 1;  /* Success, iterator has been inlined. */
  case JIT_IH_MKIDX(JIT_IHLIB_BASE, JIT_IH_BASE_IPAIRS):
    dasm_put(Dst, 1483, Dt2([ra+TFOR_CTL].value), Dt2([ra+TFOR_TAB].value), Dt2([ra+TFOR_CTL].value), (ptrdiff_t)(luaH_getnum), Dt7(->tt), Dt2([ra+TFOR_CTL].value), Dt2([ra+TFOR_KEY].tt), Dt2([ra+TFOR_KEY].value), Dt7(->value), Dt7(->value.na[1]), Dt2([ra+TFOR_VAL].tt), Dt2([ra+TFOR_VAL].value), Dt2([ra+TFOR_VAL].value.na[1]), target);
    return 1;  /* Success, iterator has been inlined. */
  }

  return 0;  /* No support for inlining any other iterators. */
}

/* ------------------------------------------------------------------------ */



#ifdef LUA_COMPAT_VARARG
static void jit_vararg_table(lua_State *L)
{
  Table *tab;
  StkId base, func;
  int i, num, numparams;
  luaC_checkGC(L);
  base = L->base;
  func = L->ci->func;
  numparams = clvalue(func)->l.p->numparams;
  num = base - func - numparams - 1;
  tab = luaH_new(L, num, 1);
  for (i = 0; i < num; i++)
    setobj2n(L, luaH_setnum(L, tab, i+1), base - num + i);
  setnvalue(luaH_setstr(L, tab, luaS_newliteral(L, "n")), (lua_Number)num);
  sethvalue(L, base + numparams, tab);
}
#endif

/* Encode JIT function prologue. */
static void jit_prologue(jit_State *J)
{
  Proto *pt = J->pt;
  int numparams = pt->numparams;
  int stacksize = pt->maxstacksize;

  dasm_put(Dst, 1544, Dt3([stacksize]), Dt1(->stack_last), Dt1(->end_ci), Dt4([1]), Dt4(->func), sizeof(TValue), Dt1(->ci));

  if (numparams > 0) {
    dasm_put(Dst, 1580, Dt2([numparams]));
  }

  if (!pt->is_vararg) {  /* Fixarg function. */
    /* Must cap L->top at L->base+numparams because 1st LOADNIL is omitted. */
    if (numparams == 0) {
      dasm_put(Dst, 1586);
    } else if (J->flags & JIT_F_CPU_CMOV) {
      dasm_put(Dst, 1589);
    } else {
      dasm_put(Dst, 1594);
    }
    dasm_put(Dst, 1603, Dt2([stacksize]), Dt4(->tailcalls), Dt4(->top), Dt1(->top), Dt1(->base), Dt4(->base));
  } else {  /* Vararg function. */
    int i;
    if (numparams > 0) {
      dasm_put(Dst, 1622);
      dasm_put(Dst, 1630, Dt3(->tt), sizeof(TValue));
    }
    dasm_put(Dst, 1649, Dt1(->base), Dt4(->base), Dt4(->tailcalls));
    for (i = 0; i < numparams; i++) {  /* Move/clear fixargs. */
      if (J->flags & JIT_F_CPU_SSE2) {
      dasm_put(Dst, 1659, Dt2([i].tt), Dt2([i].value), Dt3([i].tt), Dt3([i].value));
      } else {
      dasm_put(Dst, 1677, Dt2([i].value), Dt2([i].value.na[1]), Dt3([i].value), Dt2([i].tt), Dt3([i].value.na[1]), Dt3([i].tt));
      }
      dasm_put(Dst, 854, Dt2([i].tt));
    }
    if (numparams > 0) {
      dasm_put(Dst, 332, Dt1(->ci));
    }
    dasm_put(Dst, 1696, Dt2([stacksize]), Dt2([numparams]), Dt4(->top), Dt1(->top));
    stacksize -= numparams;		/* Fixargs are already cleared. */
  }

  /* Clear undefined args and all vars. Still assumes eax = LUA_TNIL = 0. */
  /* Note: cannot clear only args because L->top has grown. */
  if (stacksize <= EXTRA_STACK) {  /* Loopless clear. May use EXTRA_STACK. */
    int i;
    for (i = 0; i < stacksize; i++) {
      dasm_put(Dst, 1712, Dt3([i].tt));
    }
  } else {  /* Standard loop. */
    dasm_put(Dst, 1716, Dt3([0].tt), Dt3([1].tt), 2*sizeof(TValue));
  }

#ifdef LUA_COMPAT_VARARG
  if (pt->is_vararg & VARARG_NEEDSARG) {
    dasm_put(Dst, 1734, (ptrdiff_t)(jit_vararg_table));
  }
#endif

  /* Call hook check. */
  if (J->flags & JIT_F_DEBUG_CALL) {
    dasm_put(Dst, 1740, Dt1(->hookmask), LUA_MASKCALL);

  }
}

/* Check if we can combine 'return const'. */
static int jit_return_k(jit_State *J)
{
  if (!J->combine) return 0;  /* COMBINE hint set? */
  /* May need to close open upvalues. */
  if (!fhint_isset(J, NOCLOSE)) {
    dasm_put(Dst, 1820, (ptrdiff_t)(luaF_close));
  }
  if (!J->pt->is_vararg) {  /* Fixarg function. */
    dasm_put(Dst, 1830, Dt1(->ci), sizeof(CallInfo), sizeof(TValue));
  } else {  /* Vararg function. */
    dasm_put(Dst, 1844, Dt1(->ci), Dt4(->func), sizeof(CallInfo), Dt1(->ci), Dt2([1]));
  }
  jit_assert(J->combine == 1);  /* Required to skip next RETURN instruction. */
  return 1;
}

static void jit_op_return(jit_State *J, int rbase, int nresults)
{
  /* Return hook check. */
  if (J->flags & JIT_F_DEBUG_CALL) {
    if (nresults < 0 && !(J->flags & JIT_F_DEBUG_INS)) {
      dasm_put(Dst, 594, Dt1(->top));
    }
    dasm_put(Dst, 1863, Dt1(->hookmask), LUA_MASKRET);
    if (J->flags & JIT_F_DEBUG_INS) {
      dasm_put(Dst, 1878, Dt1(->savedpc));
    }

  }

  /* May need to close open upvalues. */
  if (!fhint_isset(J, NOCLOSE)) {
    dasm_put(Dst, 1820, (ptrdiff_t)(luaF_close));
  }

  /* Previous op was open: 'return f()' or 'return ...' */
  if (nresults < 0) {
    dasm_put(Dst, 332, Dt1(->ci));
    if (rbase) {
    dasm_put(Dst, 787, rbase*sizeof(TValue));
    }
    dasm_put(Dst, 1933, Dt4(->func), Dt4(->func), sizeof(CallInfo), Dt1(->ci));
    return;
  }

  if (!J->pt->is_vararg) {  /* Fixarg function, nresults >= 0. */
    int i;
    dasm_put(Dst, 1980, Dt1(->ci), sizeof(CallInfo), sizeof(TValue));
    for (i = 0; i < nresults; i++) {
      if (J->flags & JIT_F_CPU_SSE2) {
      dasm_put(Dst, 821, Dt2([rbase+i+1].tt), Dt2([rbase+i+1].value), Dt2([i].tt), Dt2([i].value));
      } else {
      dasm_put(Dst, 839, Dt2([rbase+i+1].value), Dt2([rbase+i+1].value.na[1]), Dt2([rbase+i+1].tt), Dt2([i].value), Dt2([i].value.na[1]), Dt2([i].tt));
      }
    }
    dasm_put(Dst, 1989, Dt2([nresults]));
  } else {  /* Vararg function, nresults >= 0. */
    int i;
    dasm_put(Dst, 1997, Dt1(->ci), Dt4(->func), sizeof(CallInfo), Dt1(->ci));
    for (i = 0; i < nresults; i++) {
      if (J->flags & JIT_F_CPU_SSE2) {
      dasm_put(Dst, 1659, Dt2([rbase+i].tt), Dt2([rbase+i].value), Dt3([i].tt), Dt3([i].value));
      } else {
      dasm_put(Dst, 2010, Dt2([rbase+i].value), Dt2([rbase+i].value.na[1]), Dt2([rbase+i].tt), Dt3([i].value), Dt3([i].value.na[1]), Dt3([i].tt));
      }
    }
    dasm_put(Dst, 2029);
    if (nresults) {
    dasm_put(Dst, 2036, nresults*sizeof(TValue));
    }
    dasm_put(Dst, 32);
  }
}

static void jit_op_call(jit_State *J, int func, int nargs, int nresults)
{
  int cltype = jit_inline_call(J, func, nargs, nresults);
  if (cltype < 0) return;  /* Inlined? */

  if (func) {
  dasm_put(Dst, 787, func*sizeof(TValue));
  }
  dasm_put(Dst, 2040, Dt1(->ci), Dt2([0].tt));
  if (nargs >= 0) {  /* Previous op was not open and did not set TOP. */
    dasm_put(Dst, 791, Dt2([1+nargs]));
  }
  dasm_put(Dst, 2048, Dt2(->value), (ptrdiff_t)(J->nextins), Dt4(->savedpc));
  if (cltype == LUA_TFUNCTION) {
    if (nargs == -1) {
      dasm_put(Dst, 2057);
    } else {
      dasm_put(Dst, 2062);
    }
  } else {
    dasm_put(Dst, 2067);

  }
  dasm_put(Dst, 2116, Dt5(->jit_gate));
  if (func) {
  dasm_put(Dst, 1984, func*sizeof(TValue));
  }
  dasm_put(Dst, 2121, Dt1(->base));

  /* Clear undefined results TOP <= o < func+nresults. */
  if (nresults > 0) {
    dasm_put(Dst, 2125);
    if (nresults <= EXTRA_STACK) {  /* Loopless clear. May use EXTRA_STACK. */
      int i;
      for (i = 0; i < nresults; i++) {
	dasm_put(Dst, 1712, Dt3([i].tt));
      }
    } else {  /* Standard loop. TODO: move to .tail? */
      dasm_put(Dst, 2128, Dt2([func+nresults]), Dt3([0].tt), Dt3([1].tt), 2*sizeof(TValue));
    }
  }

  if (nresults >= 0) {  /* Not an open ins. Restore L->top. */
    dasm_put(Dst, 1445, Dt2([J->pt->maxstacksize]), Dt1(->top));
  }  /* Otherwise keep TOP for next instruction. */
}

static void jit_op_tailcall(jit_State *J, int func, int nargs)
{
  int cltype;

  if (!fhint_isset(J, NOCLOSE)) {  /* May need to close open upvalues. */
    dasm_put(Dst, 1820, (ptrdiff_t)(luaF_close));
  }

  cltype = jit_inline_call(J, func, nargs, -2);
  if (cltype < 0) goto finish;  /* Inlined? */

  if (cltype == LUA_TFUNCTION) {
    jit_deopt_target(J, nargs);
    dasm_put(Dst, 2149, Dt2([func].tt));
  } else {
    dasm_put(Dst, 2158, Dt2([func].tt));
    dasm_put(Dst, 2168);
    if (func) {
    dasm_put(Dst, 787, func*sizeof(TValue));
    }
    if (nargs >= 0) {
      dasm_put(Dst, 791, Dt2([1+nargs]));
    }
    dasm_put(Dst, 2171, (ptrdiff_t)(J->nextins));

  }

  if (nargs >= 0) {  /* Previous op was not open and did not set TOP. */
    int i;
    /* Relocate [BASE+func, BASE+func+nargs] -> [ci->func, ci->func+nargs]. */
    /* TODO: loop for large nargs? */
    if (!J->pt->is_vararg) {  /* Fixarg function. */
      dasm_put(Dst, 2241, Dt2([func].value));
      for (i = 0; i < nargs; i++) {
	if (J->flags & JIT_F_CPU_SSE2) {
	dasm_put(Dst, 821, Dt2([func+1+i].tt), Dt2([func+1+i].value), Dt2([i].tt), Dt2([i].value));
	} else {
	dasm_put(Dst, 2245, Dt2([func+1+i].value), Dt2([func+1+i].value.na[1]), Dt2([i].value), Dt2([func+1+i].tt), Dt2([i].value.na[1]), Dt2([i].tt));
	}
      }
      dasm_put(Dst, 2264, Dt2([nargs]), sizeof(TValue), Dt1(->ci), Dt2(->value));
    } else {  /* Vararg function. */
      dasm_put(Dst, 2278, Dt1(->ci), Dt2([func]), Dt4(->func), Dt3(->value), Dt2(->value));
      for (i = 0; i < nargs; i++) {
	if (J->flags & JIT_F_CPU_SSE2) {
	dasm_put(Dst, 2294, Dt3([i+1].tt), Dt3([i+1].value), Dt2([i+1].tt), Dt2([i+1].value));
	} else {
	dasm_put(Dst, 2312, Dt3([i+1].value), Dt3([i+1].value.na[1]), Dt2([i+1].value), Dt3([i+1].tt), Dt2([i+1].value.na[1]), Dt2([i+1].tt));
	}
      }
      dasm_put(Dst, 2331, Dt2([1+nargs]), Dt2(->value));
    }
  } else {  /* Previous op was open and set TOP. */
    dasm_put(Dst, 332, Dt1(->ci));
    if (func) {
    dasm_put(Dst, 787, func*sizeof(TValue));
    }
    dasm_put(Dst, 2338, Dt4(->func), Dt4(->func), Dt2(->value));
  }
  dasm_put(Dst, 2230, sizeof(CallInfo), Dt5(->jit_gate));

finish:
  J->combine++;  /* Combine with following return instruction. */
}

/* ------------------------------------------------------------------------ */

static void jit_op_move(jit_State *J, int dest, int src)
{
  if (J->flags & JIT_F_CPU_SSE2) {
  dasm_put(Dst, 821, Dt2([src].tt), Dt2([src].value), Dt2([dest].tt), Dt2([dest].value));
  } else {
  dasm_put(Dst, 839, Dt2([src].value), Dt2([src].value.na[1]), Dt2([src].tt), Dt2([dest].value), Dt2([dest].value.na[1]), Dt2([dest].tt));
  }
}

static void jit_op_loadk(jit_State *J, int dest, int kidx)
{
  const TValue *kk = &J->pt->k[kidx];
  int rk = jit_return_k(J);
  if (rk) dest = 0;
  switch (ttype(kk)) {
  case 0:
  dasm_put(Dst, 2369, Dt2([dest].tt));
    break;
  case 1:
  if (bvalue(kk)) {  /* true */
  dasm_put(Dst, 2377, Dt2([dest].value), Dt2([dest].tt));
  } else {  /* false */
  dasm_put(Dst, 2389, Dt2([dest].value), Dt2([dest].tt));
  }
    break;
  case 3: {
  if ((&(kk)->value)->n == (lua_Number)0) {
  dasm_put(Dst, 2404);
  } else if ((&(kk)->value)->n == (lua_Number)1) {
  dasm_put(Dst, 2408);
  } else {
  dasm_put(Dst, 2411, &(kk)->value);
  }
  dasm_put(Dst, 1306, Dt2([dest].value), Dt2([dest].tt));
    break;
  }
  case 4:
  dasm_put(Dst, 2415, Dt2([dest].value), (ptrdiff_t)(gcvalue(kk)), Dt2([dest].tt));
    break;
  default: lua_assert(0); break;
  }
  if (rk) {
    dasm_put(Dst, 32);
  }
}

static void jit_op_loadnil(jit_State *J, int first, int last)
{
  int idx, num = last - first + 1;
  int rk = jit_return_k(J);
  dasm_put(Dst, 2125);
  if (rk) {
    dasm_put(Dst, 2427, Dt2([0].tt));
  } else if (num <= 8) {
    for (idx = first; idx <= last; idx++) {
      dasm_put(Dst, 854, Dt2([idx].tt));
    }
  } else {
    dasm_put(Dst, 2432, Dt2([first].tt), Dt2([last].tt), sizeof(TValue));
  }
}

static void jit_op_loadbool(jit_State *J, int dest, int b, int dojump)
{
  int rk = jit_return_k(J);
  if (rk) dest = 0;
  if (b) {  /* true */
  dasm_put(Dst, 2377, Dt2([dest].value), Dt2([dest].tt));
  } else {  /* false */
  dasm_put(Dst, 2389, Dt2([dest].value), Dt2([dest].tt));
  }
  if (rk) {
    dasm_put(Dst, 32);
  } else if (dojump) {
    const TValue *h = hint_getpc(J, COMBINE, J->nextpc);
    if (!(ttisboolean(h) && bvalue(h) == 0)) {  /* Avoid jmp around dead ins. */
      dasm_put(Dst, 665, J->nextpc+1);
    }
  }
}

/* ------------------------------------------------------------------------ */

static void jit_op_getupval(jit_State *J, int dest, int uvidx)
{
  if (!J->pt->is_vararg) {
  dasm_put(Dst, 2241, Dt2([-1].value));
  } else {
  dasm_put(Dst, 2452, Dt1(->ci), Dt4(->func), Dt3(->value));
  }
  dasm_put(Dst, 2462, Dt5(->upvals[uvidx]), DtF(->v));
  if (J->flags & JIT_F_CPU_SSE2) {
  dasm_put(Dst, 2469, Dt3([0].tt), Dt3([0].value), Dt2([dest].tt), Dt2([dest].value));
  } else {
  dasm_put(Dst, 2487, Dt3([0].value), Dt3([0].value.na[1]), Dt3([0].tt), Dt2([dest].value), Dt2([dest].value.na[1]), Dt2([dest].tt));
  }
}

static void jit_op_setupval(jit_State *J, int src, int uvidx)
{
  if (!J->pt->is_vararg) {
  dasm_put(Dst, 2241, Dt2([-1].value));
  } else {
  dasm_put(Dst, 2452, Dt1(->ci), Dt4(->func), Dt3(->value));
  }
  dasm_put(Dst, 2506, Dt5(->upvals[uvidx]), DtF(->v), Dt2([src].tt), Dt2([src].value), Dt3(->tt), Dt2([src].value.na[1]), Dt3(->value), Dt3(->value.na[1]));
  dasm_put(Dst, 2542, DtA(->gch.marked), WHITEBITS, DtF(->marked), bitmask(BLACKBIT));

}

/* ------------------------------------------------------------------------ */

/* Optimized table lookup routines. Enter via jsub, fallback to C. */

/* Fallback for GETTABLE_*. Temporary key is in L->env. */
static void jit_gettable_fb(lua_State *L, Table *t, StkId dest)
{
  Table *mt = t->metatable;
  const TValue *tm = luaH_getstr(mt, G(L)->tmname[TM_INDEX]);
  if (ttisnil(tm)) {  /* No __index method? */
    mt->flags |= 1<<TM_INDEX;  /* Cache this fact. */
    setnilvalue(dest);
  } else if (ttisfunction(tm)) {  /* __index function? */
    ptrdiff_t destr = savestack(L, dest);
    setobj2s(L, L->top, tm);
    sethvalue(L, L->top+1, t);
    setobj2s(L, L->top+2, &L->env);
    luaD_checkstack(L, 3);
    L->top += 3;
    luaD_call(L, L->top - 3, 1);
    dest = restorestack(L, destr);
    L->top--;
    setobjs2s(L, dest, L->top);
  } else {  /* Let luaV_gettable() continue with the __index object. */
    luaV_gettable(L, tm, &L->env, dest);
  }

}

/* Fallback for SETTABLE_*STR. Temporary (string) key is in L->env. */
static void jit_settable_fb(lua_State *L, Table *t, StkId val)
{
  Table *mt = t->metatable;
  const TValue *tm = luaH_getstr(mt, G(L)->tmname[TM_NEWINDEX]);
  if (ttisnil(tm)) {  /* No __newindex method? */
    mt->flags |= 1<<TM_NEWINDEX;  /* Cache this fact. */
    t->flags = 0;  /* But need to clear the cache for the table itself. */
    setobj2t(L, luaH_setstr(L, t, rawtsvalue(&L->env)), val);
    luaC_barriert(L, t, val);
  } else if (ttisfunction(tm)) {  /* __newindex function? */
    setobj2s(L, L->top, tm);
    sethvalue(L, L->top+1, t);
    setobj2s(L, L->top+2, &L->env);
    setobj2s(L, L->top+3, val);
    luaD_checkstack(L, 4);
    L->top += 4;
    luaD_call(L, L->top - 4, 0);
  } else {  /* Let luaV_settable() continue with the __newindex object. */
    luaV_settable(L, tm, &L->env, val);
  }

}

/* ------------------------------------------------------------------------ */

static void jit_op_newtable(jit_State *J, int dest, int lnarray, int lnhash)
{
  dasm_put(Dst, 3158, luaO_fb2int(lnarray), luaO_fb2int(lnhash), (ptrdiff_t)(luaH_new), Dt2([dest].value), Dt2([dest].tt));
  jit_checkGC(J);
}

static void jit_op_getglobal(jit_State *J, int dest, int kidx)
{
  const TValue *kk = &J->pt->k[kidx];
  jit_assert(ttisstring(kk));
  dasm_put(Dst, 3184, (ptrdiff_t)(&kk->value.gc->ts));
  if (dest) {
  dasm_put(Dst, 787, dest*sizeof(TValue));
  }
  dasm_put(Dst, 3187);
}

static void jit_op_setglobal(jit_State *J, int rval, int kidx)
{
  const TValue *kk = &J->pt->k[kidx];
  jit_assert(ttisstring(kk));
  dasm_put(Dst, 3184, (ptrdiff_t)(&kk->value.gc->ts));
  if (rval) {
  dasm_put(Dst, 787, rval*sizeof(TValue));
  }
  dasm_put(Dst, 3191);
}

enum { TKEY_KSTR = -2, TKEY_STR = -1, TKEY_ANY = 0 };

/* Optimize key lookup depending on consts or hints type. */
static int jit_keylookup(jit_State *J, int tab, int rkey)
{
  const TValue *tabt = hint_get(J, TYPE);
  const TValue *key;
  if (!ttistable(tabt)) return TKEY_ANY;  /* Not a table? Use fallback. */
  key = ISK(rkey) ? &J->pt->k[INDEXK(rkey)] : hint_get(J, TYPEKEY);
  if (ttisstring(key)) {  /* String key? */
    if (ISK(rkey)) {
      dasm_put(Dst, 3195, Dt2([tab]), (ptrdiff_t)(&key->value.gc->ts));
      return TKEY_KSTR;  /* Const string key. */
    } else {
      dasm_put(Dst, 3201, Dt2([tab]), Dt2([rkey]));
      return TKEY_STR;  /* Var string key. */
    }
  } else if (ttisnumber(key)) {  /* Number key? */
    lua_Number n = nvalue(key);
    int k;
    lua_number2int(k, n);
    if (!(k >= 1 && k < (1 << 26) && (lua_Number)k == n))
      return TKEY_ANY;  /* Not a proper array key? Use fallback. */
    if (ISK(rkey)) {
      dasm_put(Dst, 3208, Dt2([tab].tt), Dt2([tab].value), k, DtC(->array), DtC(->sizearray));
      return k;  /* Const array key (>= 1). */
    } else {
      dasm_put(Dst, 3232, Dt2([tab].tt), Dt2([rkey].tt));
      if (J->flags & JIT_F_CPU_SSE2) {
	dasm_put(Dst, 3250, Dt2([rkey]), Dt2([tab].value));
      } else {
	dasm_put(Dst, 3283, Dt2([rkey].value));
	if (J->flags & JIT_F_CPU_CMOV) {
	dasm_put(Dst, 3293);
	} else {
	dasm_put(Dst, 3298);
	}
	dasm_put(Dst, 3304, Dt2([tab].value));
      }
      dasm_put(Dst, 3320, DtC(->sizearray), DtC(->array));
      return 1;  /* Variable array key. */
    }
  }
  return TKEY_ANY;  /* Use fallback. */
}

static void jit_op_gettable(jit_State *J, int dest, int tab, int rkey)
{
  int k = jit_keylookup(J, tab, rkey);
  switch (k) {
  case TKEY_KSTR:  /* Const string key. */
    if (dest) {
    dasm_put(Dst, 787, dest*sizeof(TValue));
    }
    dasm_put(Dst, 3334);
    break;
  case TKEY_STR:  /* Variable string key. */
    if (dest) {
    dasm_put(Dst, 787, dest*sizeof(TValue));
    }
    dasm_put(Dst, 3338);
    break;
  case TKEY_ANY:  /* Generic gettable fallback. */
    if (ISK(rkey)) {
      dasm_put(Dst, 3342, (ptrdiff_t)(&J->pt->k[INDEXK(rkey)]));
    } else {
      dasm_put(Dst, 3204, Dt2([rkey]));
    }
    dasm_put(Dst, 3345, Dt2([tab]));
    if (dest) {
    dasm_put(Dst, 787, dest*sizeof(TValue));
    }
    dasm_put(Dst, 3349, Dt1(->savedpc), (ptrdiff_t)(J->nextins), (ptrdiff_t)(luaV_gettable), Dt1(->base));
    break;
  default:  /* Array key. */
    dasm_put(Dst, 3366, Dt7([k-1].tt));
    if (J->flags & JIT_F_CPU_SSE2) {
      dasm_put(Dst, 2674, Dt7([k-1].value), Dt2([dest].value));
    } else {
      dasm_put(Dst, 3378, Dt7([k-1].value), Dt7([k-1].value.na[1]), Dt2([dest].value), Dt2([dest].value.na[1]));
    }
    dasm_put(Dst, 3391, Dt2([dest].tt));
    dasm_put(Dst, 2168);
    if (ISK(rkey)) {
      dasm_put(Dst, 3398);
    } else {
      dasm_put(Dst, 3402);
    }
    dasm_put(Dst, 3406, DtC(->metatable), DtC(->flags), 1<<TM_INDEX, (ptrdiff_t)(J->nextins));
    break;
  }

}

static void jit_op_settable(jit_State *J, int tab, int rkey, int rval)
{
  const TValue *val = ISK(rval) ? &J->pt->k[INDEXK(rval)] : NULL;
  int k = jit_keylookup(J, tab, rkey);
  switch (k) {
  case TKEY_KSTR:  /* Const string key. */
  case TKEY_STR:  /* Variable string key. */
    if (ISK(rval)) {
      dasm_put(Dst, 3492, (ptrdiff_t)(val));
    } else {
      if (rval) {
      dasm_put(Dst, 787, rval*sizeof(TValue));
      }
    }
    if (k == TKEY_KSTR) {
      dasm_put(Dst, 3495);
    } else {
      dasm_put(Dst, 3499);
    }
    break;
  case TKEY_ANY:  /* Generic settable fallback. */
    if (ISK(rkey)) {
      dasm_put(Dst, 3342, (ptrdiff_t)(&J->pt->k[INDEXK(rkey)]));
    } else {
      dasm_put(Dst, 3204, Dt2([rkey]));
    }
    if (ISK(rval)) {
      dasm_put(Dst, 3184, (ptrdiff_t)(val));
    } else {
      dasm_put(Dst, 3345, Dt2([rval]));
    }
    if (tab) {
    dasm_put(Dst, 787, tab*sizeof(TValue));
    }
    dasm_put(Dst, 3503, Dt1(->savedpc), (ptrdiff_t)(J->nextins), (ptrdiff_t)(luaV_settable), Dt1(->base));
    break;
  default:  /* Array key. */
    dasm_put(Dst, 3520, Dt7([k-1].tt));
    dasm_put(Dst, 2168);
    if (ISK(rkey)) {
      dasm_put(Dst, 3534);
    } else {
      dasm_put(Dst, 3538);
    }
    dasm_put(Dst, 3406, DtC(->metatable), DtC(->flags), 1<<TM_NEWINDEX, (ptrdiff_t)(J->nextins));
    if (!ISK(rval) || iscollectable(val)) {
      dasm_put(Dst, 3542, DtC(->marked), bitmask(BLACKBIT));
      dasm_put(Dst, 3555);
    }
    if (ISK(rval)) {
      switch (ttype(val)) {
      case 0:
      dasm_put(Dst, 3565, Dt7([k-1].tt));
        break;
      case 1:
      if (bvalue(val)) {  /* true */
      dasm_put(Dst, 3573, Dt7([k-1].value), Dt7([k-1].tt));
      } else {  /* false */
      dasm_put(Dst, 3585, Dt7([k-1].value), Dt7([k-1].tt));
      }
        break;
      case 3: {
      if ((&(val)->value)->n == (lua_Number)0) {
      dasm_put(Dst, 2404);
      } else if ((&(val)->value)->n == (lua_Number)1) {
      dasm_put(Dst, 2408);
      } else {
      dasm_put(Dst, 2411, &(val)->value);
      }
      dasm_put(Dst, 3600, Dt7([k-1].value), Dt7([k-1].tt));
        break;
      }
      case 4:
      dasm_put(Dst, 3611, Dt7([k-1].value), (ptrdiff_t)(gcvalue(val)), Dt7([k-1].tt));
        break;
      default: lua_assert(0); break;
      }
    } else {
      if (J->flags & JIT_F_CPU_SSE2) {
      dasm_put(Dst, 2947, Dt2([rval].tt), Dt2([rval].value), Dt7([k-1].tt), Dt7([k-1].value));
      } else {
      dasm_put(Dst, 2965, Dt2([rval].value), Dt2([rval].value.na[1]), Dt2([rval].tt), Dt7([k-1].value), Dt7([k-1].value.na[1]), Dt7([k-1].tt));
      }
    }
    break;
  }

}

static void jit_op_self(jit_State *J, int dest, int tab, int rkey)
{
  if (J->flags & JIT_F_CPU_SSE2) {
  dasm_put(Dst, 821, Dt2([tab].tt), Dt2([tab].value), Dt2([dest+1].tt), Dt2([dest+1].value));
  } else {
  dasm_put(Dst, 839, Dt2([tab].value), Dt2([tab].value.na[1]), Dt2([tab].tt), Dt2([dest+1].value), Dt2([dest+1].value.na[1]), Dt2([dest+1].tt));
  }
  jit_op_gettable(J, dest, tab, rkey);
}

/* ------------------------------------------------------------------------ */

static void jit_op_setlist(jit_State *J, int ra, int num, int batch)
{
  if (batch == 0) { batch = (int)(*J->nextins); J->combine++; }
  batch = (batch-1)*LFIELDS_PER_FLUSH;
  if (num == 0) {  /* Previous op was open and set TOP: {f()} or {...}. */
    dasm_put(Dst, 3685, Dt1(->env.value), Dt2([ra+1]), Dt2([ra].value));
    if (batch > 0) {
      dasm_put(Dst, 3709, batch);
    }
    dasm_put(Dst, 3713, DtC(->sizearray), (ptrdiff_t)(luaH_resizearray), DtC(->marked), bitmask(BLACKBIT), DtC(->array), Dt1(->env.value));
    dasm_put(Dst, 3752);
  } else {  /* Set fixed number of args. */
    dasm_put(Dst, 3762, Dt2([ra].value), DtC(->sizearray), batch+num, DtC(->marked), bitmask(BLACKBIT), DtC(->array), Dt2([ra+1+num]));
    dasm_put(Dst, 3792, batch+num, (ptrdiff_t)(luaH_resizearray));
  }
  if (batch > 0) {
    dasm_put(Dst, 3821, batch*sizeof(TValue));
  }
  dasm_put(Dst, 3825, Dt2([ra+1]));
  if (num == 0) {  /* Previous op was open. Restore L->top. */
    dasm_put(Dst, 1445, Dt2([J->pt->maxstacksize]), Dt1(->top));
  }
}

/* ------------------------------------------------------------------------ */

static void jit_op_arith(jit_State *J, int dest, int rkb, int rkc, int ev)
{
  const TValue *kkb = ISK(rkb) ? &J->pt->k[INDEXK(rkb)] : NULL;
  const TValue *kkc = ISK(rkc) ? &J->pt->k[INDEXK(rkc)] : NULL;
  const Value *kval;
  int idx, rev;
  int target = (ev == TM_LT || ev == TM_LE) ? jit_jmp_target(J) : 0;
  int hastail = 0;

  /* The bytecode compiler already folds constants except for: k/0, k%0, */
  /* NaN results, k1<k2, k1<=k2. No point in optimizing these cases. */
  if (ISK(rkb&rkc)) goto fallback;

  /* Avoid optimization when non-numeric constants are present. */
  if (kkb ? !ttisnumber(kkb) : (kkc && !ttisnumber(kkc))) goto fallback;

  /* The TYPE hint selects numeric inlining and/or fallback encoding. */
  switch (ttype(hint_get(J, TYPE))) {
  case LUA_TNIL: hastail = 1; break;  /* No hint: numeric + fallback. */
  case LUA_TNUMBER: break;	      /* Numbers: numeric + deoptimization. */
  default: goto fallback;	      /* Mixed/other types: fallback only. */
  }

  /* The checks above ensure: at most one of the operands is a constant. */
  /* Reverse operation and swap operands so the 2nd operand is a variable. */
  if (kkc) { kval = &kkc->value; idx = rkb; rev = 1; }
  else { kval = kkb ? &kkb->value : NULL; idx = rkc; rev = 0; }

  /* Special handling for some operators. */
  switch (ev) {
  case TM_MOD:
    /* Check for modulo with positive numbers, so we can use fprem. */
    if (kval) {
      if (kval->na[1] < 0) { hastail = 0; goto fallback; }  /* x%-k, -k%x */
      dasm_put(Dst, 3850, Dt2([idx].tt), Dt2([idx].value.na[1]));
      if (kkb) {
	dasm_put(Dst, 3868, Dt2([rkc].value), kval);
      } else {
	dasm_put(Dst, 3875, kval, Dt2([rkb].value));
      }
    } else {
      dasm_put(Dst, 3882, Dt2([rkb].tt), Dt2([rkc].tt), Dt2([rkb].value.na[1]), Dt2([rkc].value.na[1]), Dt2([rkc].value), Dt2([rkb].value));
    }
    dasm_put(Dst, 1387);
    goto fpstore;
  case TM_POW:
    if (hastail || !kval) break;  /* Avoid this if not optimizing. */
    if (rev) {  /* x^k for k > 0, k integer. */
      lua_Number n = kval->n;
      int k;
      lua_number2int(k, n);
      /* All positive integers would work. But need to limit code explosion. */
      if (k > 0 && k <= 65536 && (lua_Number)k == n) {
	dasm_put(Dst, 3916, Dt2([idx].tt), Dt2([idx]));
	for (; (k & 1) == 0; k >>= 1) {  /* Handle leading zeroes (2^k). */
	  dasm_put(Dst, 3928);
	}
	if ((k >>= 1) != 0) {  /* Handle trailing bits. */
	  dasm_put(Dst, 3931);
	  for (; k != 1; k >>= 1) {
	    if (k & 1) {
	      dasm_put(Dst, 3936);
	    }
	    dasm_put(Dst, 3928);
	  }
	  dasm_put(Dst, 3939);
	}
	goto fpstore;
      }
    } else if (kval->n > (lua_Number)0) {  /* k^x for k > 0. */
      int log2kval[3];  /* Enough storage for a tword (80 bits). */
      log2kval[2] = 0;  /* Avoid leaking garbage. */
      /* Double precision log2(k) doesn't cut it (3^x != 3 for x = 1). */
      ((void (*)(int *, double))J->jsub[JSUB_LOG2_TWORD])(log2kval, kval->n);
      dasm_put(Dst, 3942, log2kval[0], log2kval[1], log2kval[2], Dt2([idx].tt), Dt2([idx].value));

      goto fpstore;
    }
    break;
  }

  /* Check number type and load 1st operand. */
  if (kval) {
    dasm_put(Dst, 4013, Dt2([idx].tt));
    if ((kval)->n == (lua_Number)0) {
    dasm_put(Dst, 2404);
    } else if ((kval)->n == (lua_Number)1) {
    dasm_put(Dst, 2408);
    } else {
    dasm_put(Dst, 2411, kval);
    }
  } else {
    if (rkb == rkc) {
      dasm_put(Dst, 4022, Dt2([rkb].tt));
    } else {
      dasm_put(Dst, 4027, Dt2([rkb].tt), Dt2([rkc].tt));
    }
    dasm_put(Dst, 3920, Dt2([rkb].value));
  }

  /* Encode arithmetic operation with 2nd operand. */
  switch ((ev<<1)+rev) {
  case TM_ADD<<1: case (TM_ADD<<1)+1:
    if (rkb == rkc) {
      dasm_put(Dst, 4041);
    } else {
      dasm_put(Dst, 4044, Dt2([idx].value));
    }
    break;
  case TM_SUB<<1:
    dasm_put(Dst, 4048, Dt2([idx].value));
    break;
  case (TM_SUB<<1)+1:
    dasm_put(Dst, 4052, Dt2([idx].value));
    break;
  case TM_MUL<<1: case (TM_MUL<<1)+1:
    if (rkb == rkc) {
      dasm_put(Dst, 3928);
    } else {
      dasm_put(Dst, 4056, Dt2([idx].value));
    }
    break;
  case TM_DIV<<1:
    dasm_put(Dst, 4060, Dt2([idx].value));
    break;
  case (TM_DIV<<1)+1:
    dasm_put(Dst, 4064, Dt2([idx].value));
    break;
  case TM_POW<<1:
    dasm_put(Dst, 4068, Dt2([idx].value), (ptrdiff_t)(pow));
    break;
  case (TM_POW<<1)+1:
    dasm_put(Dst, 4088, Dt2([idx].value), (ptrdiff_t)(pow));
    break;
  case TM_UNM<<1: case (TM_UNM<<1)+1:
    dasm_put(Dst, 4108);
    break;
  default:  /* TM_LT or TM_LE. */
    dasm_put(Dst, 1325, Dt2([idx].value));
    if (J->flags & JIT_F_CPU_CMOV) {
    dasm_put(Dst, 3293);
    } else {
    dasm_put(Dst, 3298);
    }
    dasm_put(Dst, 4111, dest?(J->nextpc+1):target);
    jit_assert(dest == 0 || dest == 1);  /* Really cond. */
    switch (((rev^dest)<<1)+(dest^(ev == TM_LT))) {
    case 0:
      dasm_put(Dst, 4115, target);
      break;
    case 1:
      dasm_put(Dst, 4119, target);
      break;
    case 2:
      dasm_put(Dst, 4123, target);
      break;
    case 3:
      dasm_put(Dst, 4127, target);
      break;
    }
    goto skipstore;
  }
fpstore:
  /* Store result and set result type (if necessary). */
  dasm_put(Dst, 933, Dt2([dest].value));
  if (dest != rkb && dest != rkc) {
    dasm_put(Dst, 1309, Dt2([dest].tt));
  }

skipstore:
  if (!hastail) {
    jit_deopt_target(J, 0);
    return;
  }

  dasm_put(Dst, 1626);
  dasm_put(Dst, 1541);

fallback:
  /* Generic fallback for arithmetic ops. */
  if (kkb) {
    dasm_put(Dst, 3342, (ptrdiff_t)(kkb));
  } else {
    dasm_put(Dst, 3204, Dt2([rkb]));
  }
  if (kkc) {
    dasm_put(Dst, 3184, (ptrdiff_t)(kkc));
  } else {
    dasm_put(Dst, 3345, Dt2([rkc]));
  }
  if (target) {  /* TM_LT or TM_LE. */
    dasm_put(Dst, 4131, Dt1(->savedpc), (ptrdiff_t)((J->nextins+1)), (ptrdiff_t)(ev==TM_LT?luaV_lessthan:luaV_lessequal), Dt1(->base));
    if (dest) {  /* cond */
      dasm_put(Dst, 1479, target);
    } else {
      dasm_put(Dst, 4154, target);
    }
  } else {
    if (dest) {
    dasm_put(Dst, 787, dest*sizeof(TValue));
    }
    dasm_put(Dst, 4158, Dt1(->savedpc), (ptrdiff_t)(J->nextins), ev, (ptrdiff_t)(luaV_arith), Dt1(->base));
  }

  if (hastail) {
    dasm_put(Dst, 1644);
  }
}

/* ------------------------------------------------------------------------ */

static void jit_fallback_len(lua_State *L, StkId ra, const TValue *rb)
{
  switch (ttype(rb)) {
  case LUA_TTABLE:
    setnvalue(ra, cast_num(luaH_getn(hvalue(rb))));
    break;
  case LUA_TSTRING:
    setnvalue(ra, cast_num(tsvalue(rb)->len));
    break;
  default: {
    const TValue *tm = luaT_gettmbyobj(L, rb, TM_LEN);
    if (ttisfunction(tm)) {
      ptrdiff_t rasave = savestack(L, ra);
      setobj2s(L, L->top, tm);
      setobj2s(L, L->top+1, rb);
      luaD_checkstack(L, 2);
      L->top += 2;
      luaD_call(L, L->top - 2, 1);
      ra = restorestack(L, rasave);
      L->top--;
      setobjs2s(L, ra, L->top);
    } else {
      luaG_typeerror(L, rb, "get length of");
    }
    break;
  }
  }
}

static void jit_op_len(jit_State *J, int dest, int rb)
{
  switch (ttype(hint_get(J, TYPE))) {
  case LUA_TTABLE:
    jit_deopt_target(J, 0);
    dasm_put(Dst, 4179, Dt2([rb].tt), Dt2([rb].value), (ptrdiff_t)(luaH_getn), Dt2([dest].value), Dt2([dest].tt));
    break;
  case LUA_TSTRING:
    jit_deopt_target(J, 0);
    dasm_put(Dst, 4212, Dt2([rb].tt), Dt2([rb].value), DtB(->tsv.len), Dt2([dest].value), Dt2([dest].tt));
    break;
  default:
    dasm_put(Dst, 3204, Dt2([rb]));
    if (dest) {
    dasm_put(Dst, 787, dest*sizeof(TValue));
    }
    dasm_put(Dst, 4237, Dt1(->savedpc), (ptrdiff_t)(J->nextins), (ptrdiff_t)(jit_fallback_len), Dt1(->base));
    break;
  }
}

static void jit_op_not(jit_State *J, int dest, int rb)
{
  /* l_isfalse() without a branch -- truly devious. */
  /* ((value & tt) | (tt>>1)) is only zero for nil/false. */
  /* Assumes: LUA_TNIL == 0, LUA_TBOOLEAN == 1, bvalue() == 0/1 */
  dasm_put(Dst, 4258, Dt2([rb].tt), Dt2([rb].value), Dt2([dest].tt), Dt2([dest].value));
}

/* ------------------------------------------------------------------------ */

static void jit_op_concat(jit_State *J, int dest, int first, int last)
{
  int num = last-first+1;
  if (num == 2 && ttisstring(hint_get(J, TYPE))) {  /* Optimize common case. */
    if (first) {
    dasm_put(Dst, 787, first*sizeof(TValue));
    }
    dasm_put(Dst, 4288, Dt2([dest].value), Dt2([dest].tt));
  } else {  /* Generic fallback. */
    dasm_put(Dst, 4302, Dt1(->savedpc), (ptrdiff_t)(J->nextins), num, last, (ptrdiff_t)(luaV_concat), Dt1(->base));
    if (dest != first) {
      if (J->flags & JIT_F_CPU_SSE2) {
      dasm_put(Dst, 821, Dt2([first].tt), Dt2([first].value), Dt2([dest].tt), Dt2([dest].value));
      } else {
      dasm_put(Dst, 839, Dt2([first].value), Dt2([first].value.na[1]), Dt2([first].tt), Dt2([dest].value), Dt2([dest].value.na[1]), Dt2([dest].tt));
      }
    }
  }
  jit_checkGC(J);  /* Always do this, even for the optimized variant. */

}

/* ------------------------------------------------------------------------ */

static void jit_op_eq(jit_State *J, int cond, int rkb, int rkc)
{
  int target = jit_jmp_target(J);
  int condtarget = cond ? (J->nextpc+1) : target;
  jit_assert(cond == 0 || cond == 1);

  /* Comparison of two constants. Evaluate at compile time. */
  if (ISK(rkb&rkc)) {
    if ((rkb == rkc) == cond) {  /* Constants are already unique. */
      dasm_put(Dst, 665, target);
    }
    return;
  }

  if (ISK(rkb|rkc)) {  /* Compare a variable and a constant. */
    const TValue *kk;
    if (ISK(rkb)) { int t = rkc; rkc = rkb; rkb = t; }  /* rkc holds const. */
    kk = &J->pt->k[INDEXK(rkc)];
    switch (ttype(kk)) {
    case LUA_TNIL:
      dasm_put(Dst, 4493, Dt2([rkb].tt));
      break;
    case LUA_TBOOLEAN:
      if (bvalue(kk)) {
	dasm_put(Dst, 4498, Dt2([rkb].tt), Dt2([rkb].value));
      } else {
	dasm_put(Dst, 4509, Dt2([rkb].tt), Dt2([rkb].value));
      }
      break;
    case LUA_TNUMBER:
      dasm_put(Dst, 4517, Dt2([rkb].tt), condtarget, Dt2([rkb].value), &kk->value);
      if (J->flags & JIT_F_CPU_CMOV) {
      dasm_put(Dst, 3293);
      } else {
      dasm_put(Dst, 3298);
      }
      dasm_put(Dst, 4111, condtarget);
      break;
    case LUA_TSTRING:
      dasm_put(Dst, 4531, Dt2([rkb].tt), condtarget, Dt2([rkb].value), (ptrdiff_t)(rawtsvalue(kk)));
      break;
    default: jit_assert(0); break;
    }
  } else {  /* Compare two variables. */
    dasm_put(Dst, 4543, Dt2([rkb].tt), Dt2([rkc].tt), condtarget);
    switch (ttype(hint_get(J, TYPE))) {
    case LUA_TNUMBER:
      jit_deopt_target(J, 0);
      dasm_put(Dst, 4553, Dt2([rkb].value), Dt2([rkc].value));
      if (J->flags & JIT_F_CPU_CMOV) {
      dasm_put(Dst, 3293);
      } else {
      dasm_put(Dst, 3298);
      }
      dasm_put(Dst, 4111, condtarget);
      break;
    case LUA_TSTRING:
      jit_deopt_target(J, 0);
      dasm_put(Dst, 4568, Dt2([rkb].value), Dt2([rkc].value));
      break;
    default:
      dasm_put(Dst, 4583, Dt2([rkc]), Dt2([rkb]), Dt1(->savedpc), (ptrdiff_t)(J->nextins), (ptrdiff_t)(luaV_equalval), Dt1(->base));
      break;
    }
  }
  if (cond) {
    dasm_put(Dst, 4154, target);
  } else {
    dasm_put(Dst, 1479, target);
  }
}

/* ------------------------------------------------------------------------ */

static void jit_op_test(jit_State *J, int cond, int dest, int src)
{
  int target = jit_jmp_target(J);

  /* l_isfalse() without a branch. But this time preserve tt/value. */
  /* (((value & tt) * 2 + tt) >> 1) is only zero for nil/false. */
  /* Assumes: 3*tt < 2^32, LUA_TNIL == 0, LUA_TBOOLEAN == 1, bvalue() == 0/1 */
  dasm_put(Dst, 4611, Dt2([src].tt), Dt2([src].value));

  /* Check if we can omit the stack copy. */
  if (dest == src) {  /* Yes, invert branch condition. */
    if (cond) {
      dasm_put(Dst, 1479, target);
    } else {
      dasm_put(Dst, 4154, target);
    }
  } else {  /* No, jump around copy code. */
    if (cond) {
      dasm_put(Dst, 4627);
    } else {
      dasm_put(Dst, 4632);
    }
    dasm_put(Dst, 4637, Dt2([src].value.na[1]), Dt2([dest].tt), Dt2([dest].value), Dt2([dest].value.na[1]), target);
  }
}

static void jit_op_jmp(jit_State *J, int target)
{
  dasm_put(Dst, 665, target);
}

/* ------------------------------------------------------------------------ */

enum { FOR_IDX, FOR_LIM, FOR_STP, FOR_EXT };

static const char *const jit_for_coerce_error[] = {
  LUA_QL("for") " initial value must be a number",
  LUA_QL("for") " limit must be a number",
  LUA_QL("for") " step must be a number",
};

/* Try to coerce for slots with strings to numbers in place or complain. */
static void jit_for_coerce(lua_State *L, TValue *o)
{
  int i;
  for (i = FOR_IDX; i <= FOR_STP; i++, o++) {
    lua_Number num;
    if (ttisnumber(o)) continue;
    if (ttisstring(o) && luaO_str2d(svalue(o), &num)) {
      setnvalue(o, num);
    } else {
      luaG_runerror(L, jit_for_coerce_error[i]);
    }
  }
}

static void jit_op_forprep(jit_State *J, int ra, int target)
{
  const TValue *step = hint_get(J, FOR_STEP_K);
  if (ttisnumber(step)) {
    dasm_put(Dst, 4654, Dt2([ra+FOR_IDX].tt), Dt2([ra+FOR_LIM].tt), Dt2([ra+FOR_LIM].value), Dt2([ra+FOR_IDX].value), Dt2([ra+FOR_EXT].value));
    if (J->flags & JIT_F_CPU_CMOV) {
    dasm_put(Dst, 3293);
    } else {
    dasm_put(Dst, 3298);
    }
    dasm_put(Dst, 1309, Dt2([ra+FOR_EXT].tt));
    if (nvalue(step) < (lua_Number)0) {
      dasm_put(Dst, 4115, target+1);
    } else {
      dasm_put(Dst, 4123, target+1);
    }
  } else {
    dasm_put(Dst, 4683, Dt2([ra+FOR_IDX].tt), Dt2([ra+FOR_LIM].tt), Dt2([ra+FOR_STP].tt), Dt2([ra+FOR_STP].value.na[1]), Dt2([ra+FOR_LIM].value), Dt2([ra+FOR_IDX].value), Dt2([ra+FOR_EXT].value));
    if (J->flags & JIT_F_CPU_CMOV) {
    dasm_put(Dst, 3293);
    } else {
    dasm_put(Dst, 3298);
    }
    dasm_put(Dst, 4732, Dt2([ra+FOR_EXT].tt), target+1);
  }
  if (ttisnumber(hint_get(J, TYPE))) {
    jit_deopt_target(J, 0);
  } else {
    dasm_put(Dst, 679);
    dasm_put(Dst, 4743, Dt2([ra]), Dt1(->savedpc), (ptrdiff_t)(J->nextins), (ptrdiff_t)(jit_for_coerce));
  }
}

static void jit_op_forloop(jit_State *J, int ra, int target)
{
  const TValue *step = hint_getpc(J, FOR_STEP_K, target-1);
  if (ttisnumber(step)) {
    dasm_put(Dst, 4766, Dt2([ra+FOR_LIM].value), Dt2([ra+FOR_IDX].value), Dt2([ra+FOR_STP].value), Dt2([ra+FOR_EXT].value), Dt2([ra+FOR_IDX].value), Dt2([ra+FOR_EXT].tt));
    if (J->flags & JIT_F_CPU_CMOV) {
    dasm_put(Dst, 3293);
    } else {
    dasm_put(Dst, 3298);
    }
    if (nvalue(step) < (lua_Number)0) {
      dasm_put(Dst, 4127, target);
    } else {
      dasm_put(Dst, 4119, target);
    }
  } else {
    dasm_put(Dst, 4789, Dt2([ra+FOR_STP].value.na[1]), Dt2([ra+FOR_LIM].value), Dt2([ra+FOR_IDX].value), Dt2([ra+FOR_STP].value), Dt2([ra+FOR_IDX].value), Dt2([ra+FOR_EXT].value), Dt2([ra+FOR_EXT].tt));
    if (J->flags & JIT_F_CPU_CMOV) {
    dasm_put(Dst, 3293);
    } else {
    dasm_put(Dst, 3298);
    }
    dasm_put(Dst, 4127, target);
  }
}

/* ------------------------------------------------------------------------ */

static void jit_op_tforloop(jit_State *J, int ra, int nresults)
{
  int target = jit_jmp_target(J);
  int i;
  if (jit_inline_tforloop(J, ra, nresults, target)) return;  /* Inlined? */
  for (i = 2; i >= 0; i--) {
    if (J->flags & JIT_F_CPU_SSE2) {
    dasm_put(Dst, 821, Dt2([ra+i].tt), Dt2([ra+i].value), Dt2([ra+i+3].tt), Dt2([ra+i+3].value));
    } else {
    dasm_put(Dst, 839, Dt2([ra+i].value), Dt2([ra+i].value.na[1]), Dt2([ra+i].tt), Dt2([ra+i+3].value), Dt2([ra+i+3].value.na[1]), Dt2([ra+i+3].tt));
    }
  }
  jit_op_call(J, ra+3, 2, nresults);
  dasm_put(Dst, 4827, Dt2([ra+3].tt));
  if (J->flags & JIT_F_CPU_SSE2) {
  dasm_put(Dst, 821, Dt2([ra+3].tt), Dt2([ra+3].value), Dt2([ra+2].tt), Dt2([ra+2].value));
  } else {
  dasm_put(Dst, 839, Dt2([ra+3].value), Dt2([ra+3].value.na[1]), Dt2([ra+3].tt), Dt2([ra+2].value), Dt2([ra+2].value.na[1]), Dt2([ra+2].tt));
  }
  dasm_put(Dst, 4649, target);
}

/* ------------------------------------------------------------------------ */

static void jit_op_close(jit_State *J, int ra)
{
  if (ra) {
    dasm_put(Dst, 4836, Dt2([ra]));
  } else {
    dasm_put(Dst, 4844);
  }
  dasm_put(Dst, 1734, (ptrdiff_t)(luaF_close));
}

static void jit_op_closure(jit_State *J, int dest, int ptidx)
{
  Proto *npt = J->pt->p[ptidx];
  int nup = npt->nups;
  if (!J->pt->is_vararg) {
  dasm_put(Dst, 4849, Dt2([-1].value));
  } else {
  dasm_put(Dst, 4853, Dt1(->ci), Dt4(->func), Dt3(->value));
  }
  dasm_put(Dst, 4863, Dt5(->env), nup, (ptrdiff_t)(luaF_newLclosure), Dt5(->p), (ptrdiff_t)(npt), Dt2([dest].value), Dt2([dest].tt));
  /* Process pseudo-instructions for upvalues. */
  if (nup > 0) {
    const Instruction *uvcode = J->nextins;
    int i, uvuv;
    /* Check which of the two types we need. */
    for (i = 0, uvuv = 0; i < nup; i++)
      if (GET_OPCODE(uvcode[i]) == OP_GETUPVAL) uvuv++;
    /* Copy upvalues from parent first. */
    if (uvuv) {
      /* LCL:eax->upvals (new closure) <-- LCL:edi->upvals (own closure). */
      for (i = 0; i < nup; i++)
	if (GET_OPCODE(uvcode[i]) == OP_GETUPVAL) {
	  dasm_put(Dst, 4895, Dt5(->upvals[GETARG_B(uvcode[i])]), Dt5(->upvals[i]));
	}
    }
    /* Next find or create upvalues for our own stack slots. */
    if (nup > uvuv) {
      dasm_put(Dst, 909);
      /* LCL:edi->upvals (new closure) <-- upvalue for stack slot. */
      for (i = 0; i < nup; i++)
	if (GET_OPCODE(uvcode[i]) == OP_MOVE) {
	  int rb = GETARG_B(uvcode[i]);
	  if (rb) {
	    dasm_put(Dst, 4836, Dt2([rb]));
	  } else {
	    dasm_put(Dst, 4844);
	  }
	  dasm_put(Dst, 4902, (ptrdiff_t)(luaF_findupval), Dt5(->upvals[i]));
	}
    }
    J->combine += nup;  /* Skip pseudo-instructions. */
  }
  jit_checkGC(J);
}

/* ------------------------------------------------------------------------ */

static void jit_op_vararg(jit_State *J, int dest, int num)
{
  if (num < 0) {  /* Copy all varargs. */
    dasm_put(Dst, 4911, Dt1(->ci), Dt4(->func), (1+J->pt->numparams)*sizeof(TValue), J->pt->maxstacksize*sizeof(TValue), Dt1(->stack_last), Dt2([dest]));
    dasm_put(Dst, 4967, Dt1(->top), (ptrdiff_t)(luaD_growstack), Dt1(->base));
  } else if (num > 0) {  /* Copy limited number of varargs. */
    dasm_put(Dst, 4993, Dt1(->ci), Dt4(->func), (1+J->pt->numparams)*sizeof(TValue), Dt2([dest]), Dt2([dest+num]), Dt3([0].tt), sizeof(TValue));
  }
}

/* ------------------------------------------------------------------------ */

