**qr2csample**

Command line inputs: Optional
  - ip (x.x.x.x:port) => specify only if you want to override your IP (otherwise qr2 will set for you)

Summary: This cross-platform test app reports a sample server to the backend.  Once reporting, the server info 
(including player/team info) can be retrieved using the ServerBrowsing SDK. After 30 seconds, the 
mapname is updated and qr2_send_statechanged is called so that the master retrieves it. The sample 
shuts down after 60 seconds, removing the server from the master list. To ensure that the server is 
reporting correctly, you can check the master server list here (look for hostname "Gamespy QR2 Sample"):
http://net.gamespy.com/masterserver/?gamename=gmtest&fields=%5Chostname%5Chostport%5Cgamever%5Cmapname%5Cgametype%5Cgamemode%5Cnumplayers%5Cmaxplayers%5Cgravity%5Crankingon%5Cnumteams&overridemaster=&filter=

Dependencies: use master server list site for verification, or you can run concurrently with sbctest to
have all the reported keys queried and displayed (wait 6-10 seconds after starting qr2csample to ensure
it is listed on the Master Server before running sbctest) 

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

