**sbctest**

Command line inputs: none

Summary: This cross-platform test app runs a server browser, which queries the GameSpy Master Server for 
all running servers of the specified game ('gmtest' in this case).  After retrieving and outputing the
basic server information for all listed servers, the server list is then sorted by ping time and
redisplayed.  After this, the server list is refreshed, this time using a server filter to narrow down
the servers retrieved.  Then, if the QR2 sample is running, an AuxUpdate is done to get and output all 
of its server/player/team keys. 

Dependencies: should be run concurrently with qr2csample (wait 6-10 seconds after starting 
qr2csample to ensure it is listed on the Master Server) 

For debug output, add GSI_COMMON_DEBUG to the preprocessor definitions.

For Unicode, add GSI_UNICODE to the preprocessor definitions (or use the Visual Studio Project's Unicode
configuration).

Internal functions used:
 _tprintf   - Unicode compatible version of printf
 _tcscpy    - Unicode compatible version of strcpy
 _tcscmp    - Unicode compatible version of strcmp
 _T         - used on all string literals for Unicode compatibility
 msleep     - Cross-platform compatible version of sleep
 GSI_UNUSED - used to avoid unreferenced variable compiler warnings 
 
 
 