////////////////////////////////////////////////////////////////////////////
//	Module 		: script_bind_macroses.h
//	Created 	: 06.02.2004
//  Modified 	: 06.02.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script bind
////////////////////////////////////////////////////////////////////////////

#pragma once

#define _DECLARE_FUNCTION10(A,D)		D A () const
#define _DECLARE_FUNCTION11(A,D,F)		D A (F f)
#define _DECLARE_FUNCTION12(A,D,F,G)	D A (F f, G g)
#define _DECLARE_FUNCTION13(A,D,F,G,H)	D A (F f, G g, H h)

#define DECLARE_FUNCTION10(A,D)			_DECLARE_FUNCTION10(A,D)		{
#define DECLARE_FUNCTION11(A,D,F)		_DECLARE_FUNCTION11(A,D,F)		{
#define DECLARE_FUNCTION12(A,D,F,G)		_DECLARE_FUNCTION12(A,D,F,G)	{
#define DECLARE_FUNCTION13(A,D,F,G,H)	_DECLARE_FUNCTION13(A,D,F,G,H)	{

#define CAST_OBJECT(Z,A,B)\
		B				*l_tpEntity = smart_cast<B*>(Z);\
		if (!l_tpEntity) {\
			ai().script_engine().script_log	(ScriptStorage::eLuaMessageTypeError,"%s : cannot access class member %s!",#B,#A);

#define CAST_OBJECT0(Z,A,B)\
		CAST_OBJECT(Z,A,B)\
			return;\
		}

#define CAST_OBJECT1(Z,A,B,D,E)\
		CAST_OBJECT(Z,A,B)\
			return		((D)(E));\
		}

#define GET_MEMBER(C,D)\
		return			((D)(l_tpEntity->C));\
	}

#define CALL_FUNCTION10(C,D)\
		return			((D)(l_tpEntity->C()));\
	}

#define CALL_FUNCTION11(C,D,F)\
		return			((D)(l_tpEntity->C((F)(f))));\
	}

#define CALL_FUNCTION01(C,F)\
		l_tpEntity->C((F)(f));\
	}

#define CALL_FUNCTION02(C,F,G)\
		l_tpEntity->C((F)(f),(G)(g));\
	}

#define CALL_FUNCTION03(C,F,G,H)\
		l_tpEntity->C((F)(f),(G)(g),(H)(h));\
	}

#define CALL_FUNCTION00(C)\
		l_tpEntity->C();\
	}

#define BIND_MEMBER(Z,A,B,C,D,E) \
	DECLARE_FUNCTION10	(A,D)\
	CAST_OBJECT1		(Z,A,B,D,E)\
	GET_MEMBER			(C,D)

#define BIND_FUNCTION00(Z,A,B,C) \
	DECLARE_FUNCTION10	(A,void)\
	CAST_OBJECT0		(Z,A,B)\
	CALL_FUNCTION00		(C)

#define BIND_FUNCTION10(Z,A,B,C,D,E) \
	DECLARE_FUNCTION10	(A,D)\
	CAST_OBJECT1		(Z,A,B,D,E)\
	CALL_FUNCTION10		(C,D)

#define BIND_FUNCTION11(Z,A,B,C,D,E,F,I) \
	DECLARE_FUNCTION11	(A,D,F)\
	CAST_OBJECT1		(Z,A,B,D,E)\
	CALL_FUNCTION11		(C,D,I)

#define BIND_FUNCTION01(Z,A,B,C,F,I) \
	DECLARE_FUNCTION11	(A,void,F)\
	CAST_OBJECT0		(Z,A,B)\
	CALL_FUNCTION01		(C,I)

#define BIND_FUNCTION02(Z,A,B,C,F,G,I,J) \
	DECLARE_FUNCTION12	(A,void,F,G)\
	CAST_OBJECT0		(Z,A,B)\
	CALL_FUNCTION02		(C,I,J)

#define BIND_FUNCTION03(Z,A,B,C,F,G,H,I,J,K) \
	DECLARE_FUNCTION13	(A,void,F,G,H)\
	CAST_OBJECT0		(Z,A,B)\
	CALL_FUNCTION03		(C,I,J,K)

