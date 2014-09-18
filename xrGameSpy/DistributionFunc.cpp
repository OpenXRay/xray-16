// DistributionFunc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "xrGameSpy_MainDefs.h"

// WORD: Bit masks for languages
#define SKU_HAS_E 0x0001	// English
#define SKU_HAS_F 0x0002	// French
#define SKU_HAS_I 0x0004	// Italian
#define SKU_HAS_G 0x0008	// German
#define SKU_HAS_S 0x0010	// Spanish
#define SKU_HAS_R 0x0020	// Russian
#define SKU_HAS_P 0x0040	// Polish
#define SKU_HAS_C 0x0080	// Czech
#define SKU_HAS_H 0x0100	// China

// BYTE: Bit masks for protection
#define SKU_PRT_NONE 0x10	// Without protection
#define SKU_PRT_SECU 0x20	// SecuROM
#define SKU_PRT_STAR 0x40	// StarForce
#define SKU_PRT_PACK 0x80	// Different DB packing algorithm
// and subprotection
#define SKU_SUB_KEYDISK 0x01	// Binding to key-disk
#define SKU_SUB_ONLINE 0x02		// Binding to hardware with online activation

int GetGameDistribution	()
{
	HKEY KeyCDKey = 0;
	
	long res = RegOpenKeyEx(REGISTRY_BASE, 
		REGISTRY_PATH, 0, KEY_READ, &KeyCDKey);

//	char	KeyValue[1024] = "";
	int		KeyValue;
	DWORD KeyValueSize = 1024;
	DWORD KeyValueType = REG_DWORD;
	if (res == ERROR_SUCCESS && KeyCDKey != 0)
	{
		res = RegQueryValueEx(KeyCDKey, REGISTRY_VALUE_INSTALL_PATCH_ID, NULL, &KeyValueType, (LPBYTE)&KeyValue, &KeyValueSize);
	};
	if (KeyCDKey != 0) RegCloseKey(KeyCDKey);

	if (res == ERROR_PATH_NOT_FOUND ||
		res == ERROR_FILE_NOT_FOUND ||
		KeyValueSize == 0)
	{
		return int(0);
	};
	
	return KeyValue;
}

/*
int _tmain(int argc, _TCHAR* argv[])
{
	char *SKUs[] = {
		"stk-for-pack-securom-activ-e" ,
		"stk-for-pack-securom-keydisk-efis" ,
		"stk-for-pack-securom-keydisk-eg" ,
		"stk-for-pack-securom-keydisk-p" ,
		"stk-for-pack-securom-keydisk-c" ,
		"stk-for-pack-securom-keydisk-h" ,
		"stk-rus-pack-starforce-keydisk-r" ,
		"stk-for-pack-noprot-efis"
	};

	for ( int i = 0; i < 8; i ++ )
		printf( "%s : %i\n" , SKUs[ i ] , GetCodeSKU( SKUs[ i ] ) );

	return 0;
}
*/

