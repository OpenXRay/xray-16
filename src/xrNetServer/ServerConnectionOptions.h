#pragma once
struct ServerConnectionOptions
{
	string4096 session_name;
	string64 server_pass = "";
	u32 dwMaxPlayers = 0;
	u32 dwServerPort;
	bool bPortWasSet = false;
};
