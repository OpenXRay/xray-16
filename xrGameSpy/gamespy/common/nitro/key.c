#include "..\nonport.h"
#include "key.h"

#define KEY_REPEAT_START    25      // Number of frames before start of key repeat
#define KEY_REPEAT_SPAN     10      // Number of frames of key repeat interval

static KeyInformation KeyInfo;

/*---------------------------------------------------------------------------*
  Name:         KeyRead

  Description:  Edit key input information
                Detect pressing trigger, releasing trigger, and pressing hold repeat
 *---------------------------------------------------------------------------*/
const KeyInformation * KeyRead(void)
{
    static u16  repeat_count[12];
    int         i;
    u16         r;

    r = PAD_Read();
    KeyInfo.trg = 0x0000;
    KeyInfo.up = 0x0000;
    KeyInfo.rep = 0x0000;

    for( i = 0 ; i < 12 ; i ++ )
    {
        if( r & ( 0x0001 << i ) )
        {
            if( !( KeyInfo.cnt & ( 0x0001 << i ) ) )
            {
                KeyInfo.trg |= ( 0x0001 << i );       // Pressing trigger input
                repeat_count[ i ] = 1;
            }
            else
            {
                if( repeat_count[ i ] > KEY_REPEAT_START )
                {
                    KeyInfo.rep |= ( 0x0001 << i );       // Pressing hold repeat
                    repeat_count[ i ] = KEY_REPEAT_START - KEY_REPEAT_SPAN;
                }
                else
                {
                    repeat_count[ i ] ++;
                }
            }
        }
        else
        {
            if( KeyInfo.cnt & ( 0x0001 << i ) )
            {
                KeyInfo.up |= ( 0x0001 << i );        // Releasing trigger input
            }
        }
    }
    KeyInfo.cnt = r;      // Unprocessed key input
    
    return &KeyInfo;
}

void WaitForA(void)
{
	while(1)
	{
		SVC_WaitVBlankIntr();
		KeyRead();
		if(KEY_A_PRESSED(&KeyInfo))
			break;
	}
}

void KeyInit(void)
{
	// empty call of key input information acquisition (pushing the A button in IPL)
	KeyRead();
}