/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2001                                                               *
 *  Contents: interface to encoding/decoding routines                       *
 *  Comments: this file can be used as an interface to PPMd module          *
 *  (consisting of Model.cpp) from external program               *
 ****************************************************************************/
#if !defined(_PPMD_H_)
#define _PPMD_H_

#include "PPMdType.h"

#ifdef  __cplusplus
extern "C" {
#endif

BOOL  _STDCALL StartSubAllocator(UINT SubAllocatorSize);
void  _STDCALL StopSubAllocator();          /* it can be called once        */
DWORD _STDCALL GetUsedMemory();             /* for information only         */

/****************************************************************************
 * Method of model restoration at memory insufficiency:                     *
 *     MRM_RESTART - restart model from scratch (default)                   *
 *     MRM_CUT_OFF - cut off model (nearly twice slower)                    *
 *     MRM_FREEZE  - freeze context tree (dangerous)                        */
enum MR_METHOD { MRM_RESTART, MRM_CUT_OFF, MRM_FREEZE };

/****************************************************************************
 * (MaxOrder == 1) parameter value has special meaning, it does not restart *
 * model and can be used for solid mode archives;                           *
 * Call sequence:                                                           *
 *     StartSubAllocator(SubAllocatorSize);                                 *
 *     EncodeFile(SolidArcFile,File1,MaxOrder,MRM_RESTART);                 *
 *     EncodeFile(SolidArcFile,File2,       1,MRM_RESTART);                 *
 *     ...                                                                  *
 *     EncodeFile(SolidArcFile,FileN,       1,MRM_RESTART);                 *
 *     StopSubAllocator();                                                  *
 ****************************************************************************/
void _STDCALL EncodeFile(_PPMD_FILE* EncodedFile,_PPMD_FILE* DecodedFile,
                        int MaxOrder,MR_METHOD MRMethod);
void _STDCALL DecodeFile(_PPMD_FILE* DecodedFile,_PPMD_FILE* EncodedFile,
                        int MaxOrder,MR_METHOD MRMethod);

/*  imported function                                                       */
void _STDCALL  PrintInfo(_PPMD_FILE* DecodedFile,_PPMD_FILE* EncodedFile);

#ifdef  __cplusplus
}
#endif

#endif /* !defined(_PPMD_H_) */
