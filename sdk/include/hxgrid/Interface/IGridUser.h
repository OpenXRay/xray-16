#pragma once

#include "VECOM.h"
#include "INOUT.h"
#include "IGenericStream.h"
#include "IAgent.h"

using namespace VITALENGINE;

//====================================================
// Task callback
//====================================================
//This callback is called by Agents on remote nodes to run distributed task.
//
// IAgent - interface to agent we are running at;
//
// sessioId - unique sessionId. Agent can run tasks from diferent users 
//            at the same time.
//
// IAgent and sessionId can be used to request global data.
//
// inStream - stream with input data, passed to IGridUser->RunTask().
// inSteam is owned by the library. Callback should not call Release() on it.
// inStream is seeked to 0, and ready to read data.
//
// outStream - stream to write output data.
// outStream is created and owned by the library. 
//
// Callback should return false, if job has been aborted (see IAgent->TestConnection()).
typedef bool (__cdecl TTaskProc)(DWORD sessionId, IGenericStream* inStream, IGenericStream* outStream);

//====================================================
// EndSession callback
//====================================================
// Called by Agents on remote nodes after ending session.
// Usually used to release any global session data.
//
// IAgent - agent we are running at;
// sessionId - unique session id;
typedef void (__cdecl EndSession)(IAgent* agent, DWORD sessionId);

//====================================================
// Finalize callback
//====================================================
// This callback is called on local workstation to process output data
// from tasks
//
// outStream - data, returned from remote task.
// 
// GridUser will release outStream just after the call returns.
// Finalize is allowed to add reference to keep stream in memory
// as long as needed. See GridGMP example.
typedef void (__cdecl TFinalizeProc)(IGenericStream* outStream);

//====================================================
// GetData callback
//====================================================
//This callback is called on local workstation to request
//global data, as a result of call IAgent->GetData().
//
// dataDesc - simbolic data Id, passed to IAgent->GetData()
//
// stream  - variable to receive address of stream, filled with data.
//
// Callback should create stream and fill it with data.
// Ownership of stream is transferred to library.
typedef void (__cdecl TGetDataProc)(const char* dataDesc, IGenericStream** outStream);

//===========================================================
//===========================================================
#pragma pack(1)
typedef struct 
{ 
 //maximum number of tasks, allowed to be sent to agent,
 //without waiting for completion of curent task
 //
 //used to "hide" network transfer time
 //
 //recommened value for long tasks (transfer time << run task) is 1
 //
 //recommended value for shorts tasks (transfer time*~0.5 ==  run time) is 10
 //
 //default is 1

 DWORD maxSendAheadTasks;

 //maximum number of tasks, buffered by IGridUser,
 //before RunTask() method will block until at least one task is completed
 //
 //larger values are commended if task formation time is significant
 //
 //value of 1 can be used for debugging
 //
 //default is 40

 DWORD maxQueqedTasks;

 //memory limit for tasks queue of IGridUser
 //(summ of inoutStream lengths)
 //queue length is limited by number of tasks (maxQueqedTasks) and this memory limit
 //
 //default is max( physicalMemory/8, 100Mb )

 DWORD userMaxMemoryUsage;

 //do not send tasks to agent, if it has not enought free physical memory
 //
 //default is 50Mb

 DWORD agentMinFreeMemory;

 //do not send task to agent, if it has less free physical memory then (task stream size)*Factor
 //
 //default is 1.5x

 float agentMinFreeMemoryFactor;

 //send dublicate tasks to free agent 
 //
 //this allows not to wait for slow agent
 //
 //default is 1 (enabled)

 bool  sendDublicateTasks;

 //swap tasks from queqe to disk if userMaxMemoryUsage reached.
 //allows to queque more huge tasks(stream size 100Mb and large)
 //
 //default is 1 (enabled)

 bool  allowSwapping;

 //if stream size if larger then this value, library will compress it before sending.
 //compression speed is about 10-20 MB/sec - this will speedup network transfer significanly 
 //on 100MBit and slower networks
 //aslo lowers memory usage, since tasks streams are kept compressed until task is run
 //
 //default is 65K

 DWORD compressThreshold;

 char  coordinator_ip[20];
 WORD  coordinator_port;

 WORD  user_data_port;
 WORD  agent_lobby_port;

 WORD  coordinator_broadcast_port;
 WORD  user_broadcast_port;

 //Enable debug output
 //default is 0

 bool enableDebugOutput;

 //suspend sending tasks to agent if it failed last task, ms
 //default is 10000
 //if agent has failed task for some reason, we better suspend sending 
 //tasks to it; let other agents do the job
 DWORD failSuspendTimeout;

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
 bool allowDiscardCoordinatorIp;

} TGridUserSettings;
#pragma pack()

#pragma pack(1)

//====================================================
//====================================================
typedef struct
{
 bool bConnectedToCoordinator;
 DWORD connectedAgentsCount; 
} TGridUserConnectionStatus;
#pragma pack()

//===========================================================
// IGridUser
//===========================================================
DECLARE_INTERFACE_(IGridUser, IUnknown)
{
 IUNKNOWN_METHODS_PURE(0x5623F256,0x00010000)

  //return IGridUser::VESION
  virtual HRESULT __stdcall GetVersion(OUT DWORD* version) = 0;

 //moduleName - dll filename with function code
 //
 //Please note, that some dependent DLLs, for example, 
 //VC++ runtime libraries, can be missing on remote workstation. 
 //It is advised to build with static libraries, 
 //and always check dependencies with tdump utility.  
 //To send additional DLLs to workstation, 
 //their names should be specified in RunTask() method, 
 //separated with comma:
 //
 //user.RunTask('GridGMP_task.dll,GMPPort.dll','RunTask',stream,Finalize,d,true);

 //taskProcName - Symbolic name of function 
 //(function should be exported from DLL by name)

 //inStream - input stream. 
 //Library is receiving ownership of the stream object.

 //finalizeProc - completion callback address

 //taskId - address of variable to receive unique id of task

 //blocking - blocking flag.

 //If task can not be added to queue immediately 
 //(due to limitations to queue length or queue input streams size), 
 //and blocking flag is set, method will not return until task 
 //is added to queue. Otherwise method will return S_FALSE, 
 //and application can wait for good moment 
 //with User->WaitForCompletionEvent())

 virtual HRESULT __stdcall RunTask(IN const char* moduleName,
  IN const char* taskProcName,
  IGenericStream* inStream,
  TFinalizeProc* finalizeProc,
  OUT DWORD* taskId,
  bool blocking = true) = 0;

 //Wait for completion of all queued tasks
 virtual HRESULT __stdcall WaitForCompletion() = 0;

 //Check whether specified task is finalized
 virtual HRESULT __stdcall IsComplete(OUT bool* complete) = 0;

 //attach GetData() callback
 virtual void __stdcall BindGetDataCallback(TGetDataProc* callback) = 0;

 //return structure with current user settings
 //(as read from ini file)
 virtual void __stdcall GetSettings(TGridUserSettings* settings) = 0;

 //set settings and save to ini file
 virtual void __stdcall SetSettings(TGridUserSettings* settings) = 0;

 //Wait for some task to complete.
 //Used to wait for the good moment to call
 //RunTask(blocking = false).
 //timeout - maximum wait time in milliseconds.
 //Returns S_OK if wait is successfull.
 //Note - RunTask() can still return S_FALSE.
 virtual  HRESULT __stdcall WaitForCompletionEvent(DWORD timeout) = 0;

 //Compress specified stream with ZLIB.
 //Signature bytes are put at the start of the stream.
 //If user or agent is receiving compressed stream, it will
 //decompress it on other side automatically.
 virtual HRESULT __stdcall CompressStream(IGenericStream* stream) = 0;

 //Cancel all queued tasks.
 //Will not return untill queue is empty.
 //After cancelation, IGridUser object
 //is still ready to queue orher tasks.
 virtual HRESULT __stdcall CancelTasks() = 0;

 //return current connection status
 //used for progress monitoring
 //see \Mandelbrot\GridGMP sample 
 virtual void __stdcall GetConnectionStatus(OUT TGridUserConnectionStatus* status) = 0;
};
