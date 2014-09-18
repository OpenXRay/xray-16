/******
gbucket.h
GameSpy Stats/Tracking SDK 
  
Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

******

Please see the GameSpy Stats and Tracking SDK for more info
You should not need to use the functions in this file, they
are used to manage the buckets by the gstats SDK.
Use the type-safe bucket functions in the gstats SDK instead.
******/ 


#ifndef _GBUCKET_H_
#define _GBUCKET_H_


#ifdef __cplusplus
extern "C" {
#endif

typedef struct bucketset_s *bucketset_t;
typedef enum {bt_int, bt_float, bt_string} BucketType;

bucketset_t NewBucketSet(void);
void FreeBucketSet(bucketset_t set);
char *DumpBucketSet(bucketset_t set);

void *BucketNew(bucketset_t set, char *name, BucketType type, void *initialvalue);
void *BucketSet(bucketset_t set, char *name,void *value);
void *BucketAdd(bucketset_t set, char *name, void *value);
void *BucketSub(bucketset_t set, char *name, void *value);
void *BucketMult(bucketset_t set, char *name, void *value);
void *BucketDiv(bucketset_t set, char *name, void *value);
void *BucketConcat(bucketset_t set, char *name, void *value);
void *BucketAvg(bucketset_t set, char *name, void *value);
void *BucketGet(bucketset_t set, char *name);

/* Helper functions */
void *bint(int i);
void *bfloat(double f);
#define bstring(a) ((void *)a)

#ifdef __cplusplus
}
#endif

#endif
