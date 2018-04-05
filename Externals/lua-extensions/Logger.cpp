#include "logger.h"

std::ofstream LogFile;

char fmt[64];
char fmt_def[] = "[%x %X]\t";

void _cdecl Logger(const char *s){
	time_t t = time(NULL);
	tm* ti = localtime(&t);
	char buf[64];
	strftime(buf, 64, fmt, ti);

	LogFile << buf << s << "\n";
	LogFile.flush();
}

int SetLog(lua_State *L){
	if(!LogFile.is_open()){
		switch(lua_gettop(L)){
			case 1:
				LogFile.open(luaL_checkstring(L, 1));
				strcpy(fmt, fmt_def);
				break;
			case 2:
				LogFile.open(luaL_checkstring(L, 1));
				strcpy(fmt, luaL_checkstring(L, 2));
				break;
			default: return luaL_error(L, "SetLog: wrong number of arguments");
		}
		if(!LogFile.is_open()){
			Log("!!Cannot open log file");
		}
	}
	SetLogCB(Logger);
	return 0;
}

int DelLog(lua_State *L){
	SetLogCB(0);
	return 0;
}

int log123(lua_State *L){
	int n = lua_gettop(L);
	for(int i=0; i<n; ++i){
		Log(luaL_checkstring(L, i+1));
	}
	return 0;
}


int open_log(lua_State *L){
	lua_register(L, "log123", log123);
	lua_register(L, "SetLog", SetLog);
	lua_register(L, "DelLog", DelLog);
	return 0;
}