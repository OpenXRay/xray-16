#include "keyboard.h"


int arr[256];

int GetKB(lua_State *L){
	lua_newtable(L);
	for(int i=0; i<256; ++i){
		lua_pushinteger(L, i);
		lua_pushboolean(L, arr[i]);
		lua_settable(L, -3);
	}
	return 1;
}

void upd(void *arg){
	while(1){
		for(int i=0; i<256; ++i){
			arr[i] = GetKeyState(i) & 0x8000;
		}
		Sleep(25);
	}
}

bool thread = true;

void kb_start(){
	if(thread){
		memset(&arr[0], 0, 256);
		_beginthread(upd, 0, 0);
	}
	thread = false;
}


int open_kb(lua_State *L){
	kb_start();
	lua_register(L, "GetKB", GetKB);
	return 0;
}