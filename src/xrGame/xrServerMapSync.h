#ifndef XRSERVER_MAP_SYNC_H
#define XRSERVER_MAP_SYNC_H

//this is a response tag that server puts in the first byte of response packet.
enum MapSyncResponse
{
	SuccessSync			= 0,		// in this case, all is OK :)
	InvalidChecksum		= 1,		// in this case, client has corrupted map geometry (checksum error)
	YouHaveOtherMap		= 2			// in this case, client has other map
}; //enum MapSyncResponse

#endif //XRSERVER_MAP_SYNC_H