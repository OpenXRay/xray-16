//==============================================================================
// hxGrid framework
// Copyright (C) 2007 by Roman Lut
// hax@deep-shadows.com
// http://www.deep-shadows.com/hax/
//==============================================================================

unit I_Agent;

interface
uses Windows, Classes, I_GenericStream;

//====================================================
//====================================================
type TAgentSettings = packed record
      bind_ip              : array [0..19] of char;

      bind_port            : WORD;

      coordinator_ip       : array [0..19] of char;
      coordinator_port     : WORD;

      user_data_port       : WORD;

      //todo: handle this
      useHT                : boolean;

      explicitCPU          : boolean;

      freeCPUCount         : DWORD;

      suspend_timeout      : DWORD;

      enableDebugOutput    : boolean;

      agent_broadcast_port       : DWORD;
      coordinator_broadcast_port : DWORD;

      //agent is caching data, requested by IAgent->GetData()
      //do not grow datacache large then this value
      maxDataCacheSize     : DWORD;

      //number of threads in thread pool,
      //derived from CPUCount and freeCPUCount
      poolSize             : DWORD;

      //allow to discard specified coordinator IP,
      //if user(or agent) is unable to connect to coordinator.
      //default is 1 (allow)

      //There are four modes:
      //
      //coordinator_ip='', allowDiscardCoordinatorIp=1
      //user will broadcast network to find coordinator address.
      //If valid address is found, it will be stored in 'coordinator_ip'
      //field and used next time.
      //
      //coordinator_ip='xx.xx.xx.xx', allowDiscardCoordinatorIp=1
      //actually same as first mode. First, will try to connect to specified address.
      //If no success, will discard specified address and broadcast
      //network to find coordinator. Valid address will be stored in
      //'coordinator_ip' fiend to be used next time.
      //
      //coordinator_ip='xx.xx.xx.xx', allowDiscardCoordinatorIp=0
      //Will constantly try to connect to specified address.
      //This mode is recommended is coordinator address is fixed,
      //or network does not support broadcasting.
      //
      //coordinator_ip='', allowDiscardCoordinatorIp=0
      //Will broadcast network, find coordinator and
      //connect to it. On next time, also start brom broadcasting.
      allowDiscardCoordinatorIp: boolean;
     end;

const IID_IAGENT      = $85343215;
const IAGENT_VERSION  = $00010001;

//===========================================================
// IAgent
//===========================================================
type IAgent = interface(IUnknown)

   function GetVersion(var version: DWORD): HRESULT; stdcall;

   //Request global data from user.

   //sessionId - sessionId value, passed to TaskProc()

   //dataDesc - symbolic identifier of data, f.e. 'geometry', zero-terminated string, case sensetive.
   //Agent can cache data internally during the session to minimize network transfer.

   //stream - address of variable to receive address of stream with data.
   //Task should release stream after using data (call stream->Release()).

   //Returns S_OK if success.
   //Returns S_FALSE if there is network problem (then task should exit with false result).
   //Returns S_FALSE if GetDataCallback did not return data for specified.
   //Returns S_FALSE if GetDataCallback is not binded on user.

   //Stream is read-only; task should not modify it, except chaging current position.
   //Returned stream is seeked to zero.

   //What happends internally is that agent returns T_AgentGenericStreamWrapper class
   //instance, which is read-only IGenericStream wrapper aroung block of memory in agent
   //data cache.

   //IAgent->GetData(stream); IAgent->PurgeCache(); stream->Release() aequences are correctly handled.

   //(NOTE: this behaviour has been implemented since version 1.09; now stream access
   //should not be guarded by global critical section).

   function GetData(sessionId: DWORD; dataDesc: pchar; var stream:IGenericStream): HRESULT; stdcall;

   //Remove specified data from agent data cache.
   //If task is copying received data to manually implemented global cache,
   //and so is sure that data will not be requested again by tasks on this agent,
   //it is recommended to free cache to lower memory usage.

   //For example, Normalmapeer_task is requesting hi-poly mesh quadtree,
   //maked instance of quadtreeclass and places it to manually implemented cache.
   //Others tasks will not call GetData(). notmalmampper_Task can
   //call FreeCachedData() to free cache.

   //Note: data cache will not grow more than maxDataCacheSize settings.

   function FreeCachedData(sessionId: DWORD; dataDesc: pchar): HRESULT; stdcall;

   //TaskProc should periodically call this method to allow agent to:
   //- check for connection;
   //- check to cancelation;
   //- suspend execution;

   //returns S_OK if ok,
   //returns S_FALSE if connection has been lost or task has been cancelled.
   //If the case of S_FALSE, taskproc should exit and return false.
   //
   //This is very fast method and can be called very frequently.
   //
   //I agent wants to suspend execution, it will sleep() inside method.
   function TestConnection(sessionId :DWORD): HRESULT; stdcall;

   //Return path to directory with task dll and data files.
   //If additional files have been specified in IGridUser->RunTask()
   //for transferring along with task dll,
   //TaskProc can search for additional files there.
   //Path contains trailing \.
   //[Path] should point to char[MAX_PATH] buffer.
   //TaskProc should not relay on current directory. Agents can run several tasks simultaneously.
   //TaskProc should use absolute paths only.
   function GetSessionCacheDirectory(sessionId :DWORD; path: pchar): HRESULT; stdcall;

   //Agent can run several tasks simultaneously.
   //If task is manipulating some global settings, like current directory,
   //or is accessing non-thread-safe library,
   //it should use global critical section.
   //Example:
   // - enter global cs;
   // - chdir()
   // - delete file in current directory;
   // - leave global cs;
   function GetGlobalCriticalSection(var cs: PRTLCriticalSection): HRESULT; stdcall;


end;


implementation

end.
