#pragma once

struct ClientConnectionOptions
{
	string256 server_name = "";
	string64 server_pass = "";
	string64 user_name = "";
	string64 user_pass = "";
	int sv_port = 0;
	int cl_port = 0;
	bool bClPortWasSet = false;
};
