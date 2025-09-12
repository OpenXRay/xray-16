/*
gpiTransfer.h
GameSpy Presence SDK 
Dan "Mr. Pants" Schoenblum

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

***********************************************************************
Please see the GameSpy Presence SDK documentation for more information
**********************************************************************/

#ifndef _GPITRANSFER_H_
#define _GPITRANSFER_H_

//INCLUDES
//////////
#include "gpi.h"

//DEFINES
/////////
#define GPI_FILE_DIRECTORY     (1 << 1)
#define GPI_FILE_SKIP          (1 << 2)
#define GPI_FILE_FAILED        (1 << 3)
#define GPI_FILE_COMPLETED     (1 << 4)
#define GPI_FILE_CONFIRMING    (1 << 5)

#define GPI_ACCEPTED           0
#define GPI_REJECTED           1
#define GPI_NOT_ACCEPTING      2

#define GPI_SKIP_READ_ERROR    0
#define GPI_SKIP_WRITE_ERROR   1
#define GPI_SKIP_USER_SKIP     2

//TYPES
///////
typedef enum
{
	GPITransferPinging,
	GPITransferWaiting,
	GPITransferTransferring,
	GPITransferComplete,
	GPITransferCancelled,
	GPITransferNoConnection
} GPITransferState;

typedef struct GPITransferID_s
{
	int profileid;
	unsigned int count;
	unsigned int time;
} GPITransferID;

typedef struct
{
	GPITransferState state;
	DArray files;
	GPITransferID transferID;
	int localID;
	GPIBool sender;
	GPProfile profile;
	GPIPeer * peer;
	int currentFile;
	int throttle;
	char * baseDirectory;
	unsigned long lastSend;
	char * message;
	int totalSize;
	int progress;
	void * userData;
} GPITransfer;

typedef struct
{
	char * path;
	char * name;
	
#ifdef GSI_UNICODE
	unsigned short* name_W; // must have this since developers are given pointers to internal memory
	unsigned short* path_W;
#endif

	int progress;
	int size;
	int acknowledged;
	FILE * file;
	int flags;
	gsi_time modTime;
	MD5_CTX md5;
	int reason;
} GPIFile;

//FUNCTIONS
///////////
#ifndef NOFILE
GPResult gpiInitTransfers(
  GPConnection * connection
);

void gpiCleanupTransfers(
  GPConnection * connection
);

GPResult gpiProcessTransfers
(
  GPConnection * connection
);

GPResult gpiNewSenderTransfer
(
  GPConnection * connection,
  GPITransfer ** transfer,
  GPProfile profile
);

void gpiFreeTransfer
(
  GPConnection * connection,
  GPITransfer * transfer
);

void gpiCancelTransfer
(
  GPConnection * connection,
  GPITransfer * transfer
);

void gpiTransferError
(
  GPConnection * connection,
  const GPITransfer * transfer
);

GPITransfer * gpiFindTransferByLocalID
(
  GPConnection * connection,
  int localID
);

int gpiGetTransferLocalIDByIndex
(
  GPConnection * connection,
  int index
);

GPIFile * gpiAddFileToTransfer
(
  GPITransfer * transfer,
  const char * path,
  const char * name
);

void gpiSkipFile
(
  GPConnection * connection,
  GPITransfer * transfer,
  int file,
  int reason
);

void gpiSkipCurrentFile
(
  GPConnection * connection,
  GPITransfer * transfer,
  int reason
);

GPIBool gpiGetTransferFileInfo
(
  FILE * file,
  int * size,
  gsi_time * modTime
);

void gpiTransferPeerDestroyed
(
  GPConnection * connection,
  GPIPeer * peer
);

void gpiTransfersHandlePong
(
  GPConnection * connection,
  GPProfile profile,
  GPIPeer * peer
);
#endif

GPResult gpiSendTransferReply
(
  GPConnection * connection,
  const GPITransferID * transferID,
  GPIPeer * peer,
  int result,
  const char * message
);

void gpiHandleTransferMessage
(
  GPConnection * connection,
  GPIPeer * peer,
  int type,
  const char * headers,
  const char * buffer,
  int len
);

#endif
