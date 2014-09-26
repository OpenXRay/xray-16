{===CRYPTDEF.INC======================================================

Compiler defines for the CRYPTOCard Encryption Component Library.

CRYPTDEF.INC is Copyright (c) 1996 by CRYPTOCard Corportation

VERSION HISTORY
10Feb96 1.00 initial release
{=====================================================================}

{Notes: to deactivate a compiler define, place a period '.' between the
        left curly brace and the '$' of '$DEFINE'.}


{Undefine this if you don't want debugging info}

{$DEFINE DEBUG}
{$DEFINE DELPHI}     {If you are using DELPHI 32 you MUST Keep DELPHI defined too}
{$DEFINE ORDER_DCBA} {INTEL DO NOT CHANGE UNLESS YOU ARE ON A MAC}
{.$DEFINE ORDER_ABCD} {MAC}
{.$DEFINE BP7}
{.$DEFINE DELPHI32}
{Only define one of the next two}
{$DEFINE i286}         {These are for the assembler routines}
{.$DEFINE i386}         {Delphi32 allows 386 instructions and no longer
                       supports inline asm functions}

{===DO NOT MODIFY ANYTHING BEYOND THIS POINT==========================}

{------Fixed compiler switches----------------------------------------}
{$B-   Short-circuit boolean expressions }
{$G+   80286+ type instructions }
{$I+   I/O checking via exceptions }
{$V-   Disable var string checking }
{$W-   No Windows realmode stack frame }
{$X+   Enable extended syntax }
{$Q-   NO Integer overflow checking }
{$R-   NO Range checking }
{$IFDEF DEBUG}
{$D+,L+,Y+  Enable debug information }
{$ELSE}
{$D-,L-,Y-  Disable debug information }
{$ENDIF}
{---------------------------------------------------------------------}
