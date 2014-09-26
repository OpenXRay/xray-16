#include "stdafx.h"

// D3D Matrix offsets
#define a_11 0
#define a_12 1*4
#define a_13 2*4
#define a_14 3*4
#define a_21 4*4
#define a_22 5*4
#define a_23 6*4
#define a_24 7*4
#define a_31 8*4
#define a_32 9*4
#define a_33 10*4
#define a_34 11*4
#define a_41 12*4
#define a_42 13*4
#define a_43 14*4
#define a_44 15*4

void	__stdcall	xrM44_Mul_3DNow	(_matrix<float>* D, _matrix<float>* M1, _matrix<float>* M2)
{
    __asm
    {
        femms
        mov         eax,[M1]			//;source a
        mov         ecx,[M2]			//;source b
        mov         edx,[D]				//;result r

        movd        mm0,[eax + a_21]    //;       | a_21
        movd        mm1,[eax + a_11]    //;       | a_11
        movd        mm6,[eax + a_12]    //;       | a_12
        punpckldq   mm1,mm0             //; a_21  | a_11  
        movd        mm5,[eax + a_22]    //;       | a_22
        pfmul       mm1,[ecx]           //; a_21 * b_12 | a_11 * b_11     
        punpckldq   mm6,mm5             //; a_22  | a_12      
        movd        mm7,[eax + a_32]    //;       | a_32
        movd        mm5,[eax + a_42]    //;       | a_42
        pfmul       mm6,[ecx]           //; a_22 * b_12 | a_12 * b_11     
        movd        mm2,[eax + a_31]    //;       | a_31
        punpckldq   mm7,mm5             //; a_42  | a_32
        movd        mm0,[eax + a_41]    //;       | a_41
        pfmul       mm7,[ecx+8]         //; a_42 * b_14 | a_32 * b13
        punpckldq   mm2,mm0             //; a_41  | a_31
        pfadd       mm6,mm7				//; a_42 * b_14 + a_22 * b_12 | a_32 * b13 + a_12 * b_11
        pfmul       mm2,[ecx+8]         //; a_41 * b_14 | a_31 * b13
        pfacc       mm6,mm6				//;		| a_12 * b_11 + a_22 * b_12 + a_32 * b_13 + a_42 * b_14  
        pfadd       mm1,mm2				//; a_21 * b_12 + a_41 * b_14 | a_11 * b_11 + a_31 * b13
        movd        [edx+4],mm6         //; T_12   
        pfacc       mm1,mm1				//;       |  a_21 * b_12 + a_41 * b_14 + a_11 * b_11 + a_31 * b13
        movd        [edx],mm1           //; T_11

        movd        mm0,[eax + a_23]    //;       | a_23
        movd        mm1,[eax + a_13]    //;       | a_13
        movd        mm6,[eax + a_14]    //;       | a_14
        punpckldq   mm1,mm0             //; a_23  | a_13  
        movd        mm5,[eax + a_24]    //;       | a_24
        pfmul       mm1,[ecx]           //; a_23 * b_12 | a_13 * b_11     
        punpckldq   mm6,mm5             //; a_24  | a_14      
        movd        mm7,[eax + a_34]    //;       | a_34
        movd        mm5,[eax + a_44]    //;       | a_44
        pfmul       mm6,[ecx]           //; a_24 * b_12 | a_14 * b_11     
        movd        mm2,[eax + a_33]    //;       | a_33
        punpckldq   mm7,mm5             //; a_44  | a_34
        movd        mm0,[eax + a_43]    //;       | a_43
        pfmul       mm7,[ecx+8]         //; a_44 * b_14 | a_34 * b_13
        punpckldq   mm2,mm0             //; a_43  | a_33
        pfadd       mm6,mm7				//; a_44 * b_14 + a_24 * b_12 | a_34 * b_13 + a_14 * b_11
        pfmul       mm2,[ecx+8]         //; a_43 * b_12 | a_33 * b11
        pfacc       mm6,mm6				//;		| a_44 * b_14 + a_24 * b_12 + a_34 * b_13 + a_14 * b_11
        pfadd       mm1,mm2				//; a_43 * b_12 + a_23 * b_12 | a_33 * b11 + a_13 * b_11
        movd        [edx+12],mm6		//; T_14
        pfacc       mm1,mm1				//;		| a_43 * b_12 + a_23 * b_12 + a_33 * b11 + a_13 * b_11
        movd        [edx+8],mm1			//; T_13

        movd        mm0,[eax + a_21]    //;       | a_21
        movd        mm1,[eax + a_11]    //;       | a_11
        movd        mm6,[eax + a_12]    //;       | a_12
        punpckldq   mm1,mm0             //; a_21  | a_11  
        movd        mm5,[eax + a_22]    //;       | a_22
        pfmul       mm1,[ecx+16]        //; a_21 * b_22 | a_11 * b_21     
        punpckldq   mm6,mm5             //; a_22  | a_12      
        movd        mm7,[eax + a_32]    //;       | a_32
        movd        mm5,[eax + a_42]    //;       | a_42
        pfmul       mm6,[ecx+16]        //; a_22 * b_22 | a_12 * b_21     
        movd        mm2,[eax + a_31]    //;       | a_31
        punpckldq   mm7,mm5             //; a_42  | a_32
        movd        mm0,[eax + a_41]    //;       | a_41
        pfmul       mm7,[ecx+24]        //; a_42 * b_24 | a_32 * b_23
		punpckldq   mm2,mm0             //; a_41  | a_31
        pfadd       mm6,mm7				//; a_42 * b_24 + a_22 * b_22 | a_32 * b_23 + a_12 * b_21
        pfmul       mm2,[ecx+24]        //; a_41 * b_24 | a_31 * b_23
        pfacc       mm6,mm6				//;       | a_42 * b_24 + a_22 * b_22 + a_32 * b_23 + a_12 * b_21
        pfadd       mm1,mm2				//; a_41 * b_24 + a_21 * b_22 | a_31 * b_23 + a_11 * b_21
        movd        [edx+20],mm6		//; T_22
        pfacc       mm1,mm1				//;		|a_41 * b_24 + a_21 * b_22 + a_31 * b_23 + a_11 * b_21
        movd        [edx+16],mm1		//; T_21

        movd        mm0,[eax + a_23]    //;       | a_23
        movd        mm1,[eax + a_13]    //;       | a_13
        movd        mm6,[eax + a_14]    //;       | a_14
        punpckldq   mm1,mm0             //; a_23  | a_13  
        movd        mm5,[eax + a_24]    //;       | a_24
        pfmul       mm1,[ecx+16]        //; a_23 * b_22 | a_13 * b_21 
        punpckldq   mm6,mm5             //; a_24  | a_14      
        movd        mm7,[eax + a_34]    //;       | a_34
        movd        mm5,[eax + a_44]    //;       | a_44
        pfmul       mm6,[ecx+16]        //; a_24 * b_22 | a_14 * b_21     
        movd        mm2,[eax + a_33]    //;       | a_33
        punpckldq   mm7,mm5             //; a_44  | a_34
        movd        mm0,[eax + a_43]    //;       | a_43
        pfmul       mm7,[ecx+24]        //; a_44 * b_24 | a_34 * b_23
        punpckldq   mm2,mm0             //; a_43  | a_33
        pfadd       mm6,mm7				//; a_24 * b_22 + a_44 * b_24 | a_14 * b_21 + a_34 * b_23
        pfmul       mm2,[ecx+24]        //; a_43 * b_24 | a_33 * b_23
        pfacc       mm6,mm6				//;		|a_24 * b_22 + a_44 * b_24 + a_14 * b_21 + a_34 * b_23
        pfadd       mm1,mm2				//; a_43 * b_24 + a_23 * b_22 | a_33 * b_23 + a_14 * b_21
        movd        [edx+28],mm6		//; T_24
        pfacc       mm1,mm1				//;		| a_43 * b_24 + a_23 * b_22 + a_33 * b_23 + a_14 * b_21
        movd        [edx+24],mm1		//; T_23

        movd        mm0,[eax + a_21]    //;       | a_21
        movd        mm1,[eax + a_11]    //;       | a_11
        movd        mm6,[eax + a_12]    //;       | a_12
        punpckldq   mm1,mm0             //; a_21  | a_11  
        movd        mm5,[eax + a_22]    //;       | a_22
        pfmul       mm1,[ecx+32]        //; a_21 * b_32 | a_11 * b_31     
        punpckldq   mm6,mm5             //; a_22  | a_12      
        movd        mm7,[eax + a_32]    //;       | a_32
        movd        mm5,[eax + a_42]    //;       | a_42
        pfmul       mm6,[ecx+32]        //; a_22 * b_32 | a_12 * b_31 
        movd        mm2,[eax + a_31]    //;       | a_31
        punpckldq   mm7,mm5             //; a_42  | a_32
        movd        mm0,[eax + a_41]    //;       | a_41
        pfmul       mm7,[ecx+40]        //; a_42 * b_34 | a_32 * b33
        punpckldq   mm2,mm0             //; a_41  | a_31
        pfadd       mm6,mm7				//; a_42 * b_34 + a_22 * b_32 | a_32 * b33 + a_12 * b_31 
        pfmul       mm2,[ecx+40]        //; a_41 * b_34 | a_31 * b33
        pfacc       mm6,mm6				//;		|a_42 * b_34 + a_22 * b_32 + a_32 * b33 + a_12 * b_31 
        pfadd       mm1,mm2				//; a_41 * b_34 + a_21 * b_32 | a_31 * b33 + a_11 * b_31
        movd        [edx+36],mm6		//; T_32
        pfacc       mm1,mm1				//;		|a_41 * b_34 + a_21 * b_32 + a_31 * b33 + a_11 * b_31
        movd        [edx+32],mm1		//; T_31

        movd        mm0,[eax + a_23]    //;       | a_23
        movd        mm1,[eax + a_13]    //;       | a_13
        movd        mm6,[eax + a_14]    //;       | a_14
        punpckldq   mm1,mm0             //; a_23  | a_13  
        movd        mm5,[eax + a_24]    //;       | a_24
        pfmul       mm1,[ecx+32]        //; a_21 * b_32 | a_11 * b_31     
        punpckldq   mm6,mm5             //; a_22  | a_12      
        movd        mm7,[eax + a_34]    //;       | a_34
        movd        mm5,[eax + a_44]    //;       | a_44
        pfmul       mm6,[ecx+32]        //; a_22 * b_32 | a_12 * b_31     
        movd        mm2,[eax + a_33]    //;       | a_33
        punpckldq   mm7,mm5             //; a_42  | a_32
        movd        mm0,[eax + a_43]    //;       | a_43
        pfmul       mm7,[ecx+40]        //; a_42 * b_34 | a_32 * b_33
        punpckldq   mm2,mm0             //; a_43  | a_33
        pfadd       mm6,mm7				//; a_42 * b_34 + a_22 * b_32 | a_32 * b_33 + a_12 * b_31
        pfmul       mm2,[ecx+40]        //; a_41 * b_34 | a_31 * b_33
        pfacc       mm6,mm6				//;		|a_42 * b_34 + a_22 * b_32 + a_32 * b_33 + a_12 * b_31
        pfadd       mm1,mm2				//; a_41 * b_34 + a_21 * b_32 | a_31 * b_33 + a_11 * b_31
        movd        [edx+44],mm6		//; T_34
        pfacc       mm1,mm1				//;		|a_41 * b_34 + a_21 * b_32 + a_31 * b_33 + a_11 * b_31
        movd        [edx+40],mm1		//; T_33

        movd        mm0,[eax + a_21]    //;       | a_21
        movd        mm1,[eax + a_11]    //;       | a_11
        movd        mm6,[eax + a_12]    //;       | a_12
        punpckldq   mm1,mm0             //; a_21  | a_11  
        movd        mm5,[eax + a_22]    //;       | a_22
        pfmul       mm1,[ecx+48]        //; a_21 * b_42 | a_11 * b_41     
        punpckldq   mm6,mm5             //; a_22  | a_12      
        movd        mm7,[eax + a_32]    //;       | a_32
        movd        mm5,[eax + a_42]    //;       | a_42
        pfmul       mm6,[ecx+48]        //; a_22 * b_42 | a_12 * b_41     
        movd        mm2,[eax + a_31]    //;       | a_31
        punpckldq   mm7,mm5             //; a_42  | a_32
        movd        mm0,[eax + a_41]    //;       | a_41
        pfmul       mm7,[ecx+56]        //; a_42 * b_44 | a_32 * b_43
        punpckldq   mm2,mm0             //; a_41  | a_31
        pfadd       mm6,mm7				//; a_42 * b_44 + a_22 * b_42 | a_32 * b_43 + a_12 * b_41
        pfmul       mm2,[ecx+56]        //; a_41 * b_44 | a_31 * b_43
        pfacc       mm6,mm6				//;		|a_42 * b_44 + a_22 * b_42 + a_32 * b_43 + a_12 * b_41
        pfadd       mm1,mm2				//; a_41 * b_44 + a_21 * b_42 | a_31 * b_43 + a_11 * b_41
        movd        [edx+52],mm6		//; T_42
        pfacc       mm1,mm1				//;		| a_41 * b_44 + a_21 * b_42 + a_31 * b_43 + a_11 * b_41
        movd        [edx+48],mm1		//; T_41


        movd        mm0,[eax + a_23]    //;       | a_23
        movd        mm1,[eax + a_13]    //;       | a_13
        movd        mm6,[eax + a_14]    //;       | a_14
        punpckldq   mm1,mm0             //; a_23  | a_13  
        movd        mm5,[eax + a_24]    //;       | a_24
        pfmul       mm1,[ecx+48]        //; a_21 * b_42 | a_11 * b_41     
        punpckldq   mm6,mm5             //; a_22  | a_12      
        movd        mm7,[eax + a_34]    //;       | a_34
        movd        mm5,[eax + a_44]    //;       | a_44
        pfmul       mm6,[ecx+48]        //; a_22 * b_42 | a_12 * b_41     
        movd        mm2,[eax + a_33]    //;       | a_33
        punpckldq   mm7,mm5             //; a_42  | a_32
        movd        mm0,[eax + a_43]    //;       | a_43
        pfmul       mm7,[ecx+56]        //; a_42 * b_44 | a_32 * b_43
        punpckldq   mm2,mm0             //; a_43  | a_33
        pfadd       mm6,mm7				//; a_42 * b_44 + a_22 * b_42 | a_32 * b_43 + a_12 * b_41
        pfmul       mm2,[ecx+56]        //; a_41 * b_44 | a_31 * b_43
        pfacc       mm6,mm6				//;		|a_42 * b_44 + a_22 * b_42 + a_32 * b_43 + a_12 * b_41
        pfadd       mm1,mm2				//; a_41 * b_44 + a_21 * b_42 | a_31 * b_43 + a_11 * b_41 
        movd        [edx+60],mm6		//; T_44
        pfacc       mm1,mm1				//; a_41 * b_44 + a_21 * b_42 + a_31 * b_43 + a_11 * b_41 
        movd        [edx+56],mm1		//; T_43

        femms
    }
}
