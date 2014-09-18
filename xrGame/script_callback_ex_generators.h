////////////////////////////////////////////////////////////////////////////
//	Module 		: script_callback_ex_generators.h
//	Created 	: 06.02.2004
//  Modified 	: 11.01.2005
//	Author		: Sergey Zhemeitsev and Dmitriy Iassenev
//	Description : Script callbacks with return value generators
////////////////////////////////////////////////////////////////////////////

#ifndef SCRIPT_CALLBACK_EX_GENERATORS
#define SCRIPT_CALLBACK_EX_GENERATORS

#define param_generator0(a,b,c) 
function_body(,,param_generator0(left_comment,right_comment,typename _),param_generator0(_,,__),,param_generator0(left_comment,right_comment,__))

#define param_generator1(a,b,c)								 param_generator(a,1,b,c)
function_body(template <,>,param_generator1(left_comment,right_comment,typename _),param_generator1(_,,__),comma,param_generator1(left_comment,right_comment,__))

#define param_generator2(a,b,c)		param_generator1(a,b,c), param_generator(a,2,b,c)
function_body(template <,>,param_generator2(left_comment,right_comment,typename _),param_generator2(_,,__),comma,param_generator2(left_comment,right_comment,__))

#define param_generator3(a,b,c)		param_generator2(a,b,c), param_generator(a,3,b,c)
function_body(template <,>,param_generator3(left_comment,right_comment,typename _),param_generator3(_,,__),comma,param_generator3(left_comment,right_comment,__))

#define param_generator4(a,b,c)		param_generator3(a,b,c), param_generator(a,4,b,c)
function_body(template <,>,param_generator4(left_comment,right_comment,typename _),param_generator4(_,,__),comma,param_generator4(left_comment,right_comment,__))

#define param_generator5(a,b,c)		param_generator4(a,b,c), param_generator(a,5,b,c)
function_body(template <,>,param_generator5(left_comment,right_comment,typename _),param_generator5(_,,__),comma,param_generator5(left_comment,right_comment,__))

#define param_generator6(a,b,c)		param_generator5(a,b,c), param_generator(a,6,b,c)
function_body(template <,>,param_generator6(left_comment,right_comment,typename _),param_generator6(_,,__),comma,param_generator6(left_comment,right_comment,__))

#define param_generator7(a,b,c)		param_generator6(a,b,c), param_generator(a,7,b,c)
function_body(template <,>,param_generator7(left_comment,right_comment,typename _),param_generator7(_,,__),comma,param_generator7(left_comment,right_comment,__))

#define param_generator8(a,b,c)		param_generator7(a,b,c), param_generator(a,8,b,c)
function_body(template <,>,param_generator8(left_comment,right_comment,typename _),param_generator8(_,,__),comma,param_generator8(left_comment,right_comment,__))

#undef param_generator8
#undef param_generator7
#undef param_generator6
#undef param_generator5
#undef param_generator4
#undef param_generator3
#undef param_generator2
#undef param_generator1
#undef param_generator0
#endif // SCRIPT_CALLBACK_EX_GENERATORS