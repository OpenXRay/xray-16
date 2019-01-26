;; rdrand.asm - written and placed in public domain by Jeffrey Walton and Uri Blumenthal.
;;              Copyright assigned to the Crypto++ project.

;; This ASM file provides RDRAND and RDSEED to downlevel Microsoft tool chains.
;; Everything "just works" under Visual Studio. Other platforms will have to
;; run MASM/MASM-64 and then link to the object files.

;; set ASFLAGS=/nologo /D_M_X86 /W3 /Cx /Zi /safeseh
;; set ASFLAGS64=/nologo /D_M_X64 /W3 /Cx /Zi
;; "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\bin\ml.exe" %ASFLAGS% /Fo rdrand-x86.obj /c rdrand.asm
;; "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\bin\amd64\ml64.exe" %ASFLAGS64% /Fo rdrand-x64.obj /c rdrand.asm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TITLE    MASM_RDRAND_GenerateBlock and MASM_RDSEED_GenerateBlock
SUBTITLE Microsoft specific ASM code to utilize RDRAND and RDSEED for down level Microsoft toolchains

PUBLIC MASM_RDRAND_GenerateBlock
PUBLIC MASM_RDSEED_GenerateBlock

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; C/C++ Function prototypes (both are fastcall)
;;   X86:
;;      extern "C" void __fastcall MASM_RDRAND_GenerateBlock(byte* ptr, size_t size);
;;   X64:
;;      extern "C" void __fastcall MASM_RDRAND_GenerateBlock(byte* ptr, size_t size);

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFDEF _M_X86    ;; Set via the command line

.486
.MODEL FLAT

;; Fastcall calling conventions exports
ALIAS <@MASM_RDRAND_GenerateBlock@8> = <MASM_RDRAND_GenerateBlock>
ALIAS <@MASM_RDSEED_GenerateBlock@8> = <MASM_RDSEED_GenerateBlock>

ENDIF

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFDEF _M_X86    ;; Set via the command line

.CODE
ALIGN   8
OPTION PROLOGUE:NONE
OPTION EPILOGUE:NONE

;; No need for Load_Arguments due to fastcall
;;   ECX (in): arg1, byte* buffer
;;   EDX (in): arg2, size_t bsize

MASM_RDRAND_GenerateBlock PROC   ;; arg1:DWORD, arg2:DWORD

    MWSIZE EQU 04h    ;; machine word size
    buffer EQU ecx
    bsize  EQU edx

            ;; Top of While loop
GenerateBlock_Top:

            ;; Check remaining size
    cmp     bsize, 0
    je      GenerateBlock_Return

Call_RDRAND_EAX:
            ;; RDRAND is not available prior to VS2012. Just emit
            ;;   the byte codes using DB. This is `rdrand eax`.
    DB      0Fh, 0C7h, 0F0h

            ;; If CF=1, the number returned by RDRAND is valid.
            ;; If CF=0, a random number was not available.

            ;; Retry immediately
    jnc     Call_RDRAND_EAX

RDRAND_succeeded:

    cmp     bsize, MWSIZE
    jb      Partial_Machine_Word

Full_Machine_Word:

    mov     DWORD PTR [buffer], eax
    add     buffer, MWSIZE        ;; No need for Intel Core 2 slow workarounds, like
    sub     bsize, MWSIZE         ;;   `lea buffer,[buffer+MWSIZE]` for faster adds

            ;; Continue
    jmp     GenerateBlock_Top

            ;; 1,2,3 bytes remain
Partial_Machine_Word:

            ;; Test bit 1 to see if size is at least 2
    test    bsize, 2
    jz      Bit_1_Not_Set

    mov     WORD PTR [buffer], ax
    shr     eax, 16
    add     buffer, 2

Bit_1_Not_Set:

            ;; Test bit 0 to see if size is at least 1
    test    bsize, 1
    jz      Bit_0_Not_Set

    mov     BYTE PTR [buffer], al

Bit_0_Not_Set:

            ;; We've hit all the bits

GenerateBlock_Return:

            ;; Clear artifacts
    xor     eax, eax
    ret

MASM_RDRAND_GenerateBlock ENDP

ENDIF    ;; _M_X86

OPTION PROLOGUE:PrologueDef
OPTION EPILOGUE:EpilogueDef

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFDEF _M_X64    ;; Set via the command line

.CODE
ALIGN   16
OPTION PROLOGUE:NONE
OPTION EPILOGUE:NONE

;; No need for Load_Arguments due to fastcall
;;   RCX (in): arg1, byte* buffer
;;   RDX (in): arg2, size_t bsize

MASM_RDRAND_GenerateBlock PROC   ;; arg1:QWORD, arg2:QWORD

    MWSIZE EQU 08h    ;; machine word size
    buffer EQU rcx
    bsize  EQU rdx

            ;; Top of While loop
GenerateBlock_Top:

            ;; Check remaining size
    cmp     bsize, 0
    je      GenerateBlock_Return

Call_RDRAND_RAX:
            ;; RDRAND is not available prior to VS2012. Just emit
            ;;   the byte codes using DB. This is `rdrand rax`.
    DB      048h, 0Fh, 0C7h, 0F0h

            ;; If CF=1, the number returned by RDRAND is valid.
            ;; If CF=0, a random number was not available.

            ;; Retry immediately
    jnc     Call_RDRAND_RAX

RDRAND_succeeded:

    cmp     bsize, MWSIZE
    jb      Partial_Machine_Word

Full_Machine_Word:

    mov     QWORD PTR [buffer], rax
    add     buffer, MWSIZE
    sub     bsize, MWSIZE

            ;; Continue
    jmp     GenerateBlock_Top

            ;; 1,2,3,4,5,6,7 bytes remain
Partial_Machine_Word:

            ;; Test bit 2 to see if size is at least 4
    test    bsize, 4
    jz      Bit_2_Not_Set

    mov     DWORD PTR [buffer], eax
    shr     rax, 32
    add     buffer, 4

Bit_2_Not_Set:

            ;; Test bit 1 to see if size is at least 2
    test    bsize, 2
    jz      Bit_1_Not_Set

    mov     WORD PTR [buffer], ax
    shr     eax, 16
    add     buffer, 2

Bit_1_Not_Set:

            ;; Test bit 0 to see if size is at least 1
    test    bsize, 1
    jz      Bit_0_Not_Set

    mov     BYTE PTR [buffer], al

Bit_0_Not_Set:

            ;; We've hit all the bits

GenerateBlock_Return:

            ;; Clear artifacts
    xor     rax, rax
	ret

MASM_RDRAND_GenerateBlock ENDP

ENDIF    ;; _M_X64

OPTION PROLOGUE:PrologueDef
OPTION EPILOGUE:EpilogueDef

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFDEF _M_X86    ;; Set via the command line

.CODE
ALIGN   8
OPTION PROLOGUE:NONE
OPTION EPILOGUE:NONE

;; No need for Load_Arguments due to fastcall
;;   ECX (in): arg1, byte* buffer
;;   EDX (in): arg2, size_t bsize

MASM_RDSEED_GenerateBlock PROC   ;; arg1:DWORD, arg2:DWORD

    MWSIZE EQU 04h    ;; machine word size
    buffer EQU ecx
    bsize  EQU edx

            ;; Top of While loop
GenerateBlock_Top:

            ;; Check remaining size
    cmp     bsize, 0
    je      GenerateBlock_Return

Call_RDSEED_EAX:
            ;; RDSEED is not available prior to VS2012. Just emit
            ;;   the byte codes using DB. This is `rdseed eax`.
    DB      0Fh, 0C7h, 0F8h

            ;; If CF=1, the number returned by RDSEED is valid.
            ;; If CF=0, a random number was not available.

            ;; Retry immediately
    jnc     Call_RDSEED_EAX

RDSEED_succeeded:

    cmp     bsize, MWSIZE
    jb      Partial_Machine_Word

Full_Machine_Word:

    mov     DWORD PTR [buffer], eax
    add     buffer, MWSIZE        ;; No need for Intel Core 2 slow workarounds, like
    sub     bsize, MWSIZE         ;;   `lea buffer,[buffer+MWSIZE]` for faster adds

            ;; Continue
    jmp     GenerateBlock_Top

            ;; 1,2,3 bytes remain
Partial_Machine_Word:

            ;; Test bit 1 to see if size is at least 2
    test    bsize, 2
    jz      Bit_1_Not_Set

    mov     WORD PTR [buffer], ax
    shr     eax, 16
    add     buffer, 2

Bit_1_Not_Set:

            ;; Test bit 0 to see if size is at least 1
    test    bsize, 1
    jz      Bit_0_Not_Set

    mov     BYTE PTR [buffer], al

Bit_0_Not_Set:

            ;; We've hit all the bits

GenerateBlock_Return:

            ;; Clear artifacts
    xor     eax, eax
	ret

MASM_RDSEED_GenerateBlock ENDP

ENDIF    ;; _M_X86

OPTION PROLOGUE:PrologueDef
OPTION EPILOGUE:EpilogueDef

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFDEF _M_X64    ;; Set via the command line

.CODE
ALIGN   16
OPTION PROLOGUE:NONE
OPTION EPILOGUE:NONE

;; No need for Load_Arguments due to fastcall
;;   RCX (in): arg1, byte* buffer
;;   RDX (in): arg2, size_t bsize

MASM_RDSEED_GenerateBlock PROC   ;; arg1:QWORD, arg2:QWORD

    MWSIZE EQU 08h    ;; machine word size
    buffer EQU rcx
    bsize  EQU rdx

            ;; Top of While loop
GenerateBlock_Top:

            ;; Check remaining size
    cmp     bsize, 0
    je      GenerateBlock_Return

Call_RDSEED_RAX:
            ;; RDSEED is not available prior to VS2012. Just emit
            ;;   the byte codes using DB. This is `rdseed rax`.
    DB      048h, 0Fh, 0C7h, 0F8h

            ;; If CF=1, the number returned by RDSEED is valid.
            ;; If CF=0, a random number was not available.

            ;; Retry immediately
    jnc     Call_RDSEED_RAX

RDSEED_succeeded:

    cmp     bsize, MWSIZE
    jb      Partial_Machine_Word

Full_Machine_Word:

    mov     QWORD PTR [buffer], rax
    add     buffer, MWSIZE
    sub     bsize, MWSIZE

            ;; Continue
    jmp     GenerateBlock_Top

            ;; 1,2,3,4,5,6,7 bytes remain
Partial_Machine_Word:

            ;; Test bit 2 to see if size is at least 4
    test    bsize, 4
    jz      Bit_2_Not_Set

    mov     DWORD PTR [buffer], eax
    shr     rax, 32
    add     buffer, 4

Bit_2_Not_Set:

            ;; Test bit 1 to see if size is at least 2
    test    bsize, 2
    jz      Bit_1_Not_Set

    mov     WORD PTR [buffer], ax
    shr     eax, 16
    add     buffer, 2

Bit_1_Not_Set:

            ;; Test bit 0 to see if size is at least 1
    test    bsize, 1
    jz      Bit_0_Not_Set

    mov     BYTE PTR [buffer], al

Bit_0_Not_Set:

            ;; We've hit all the bits

GenerateBlock_Return:

            ;; Clear artifacts
    xor     rax, rax
    ret

MASM_RDSEED_GenerateBlock ENDP

ENDIF    ;; _M_X64

OPTION PROLOGUE:PrologueDef
OPTION EPILOGUE:EpilogueDef

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

END
