/******
gbucket.c
GameSpy Stats/Tracking SDK 
  
Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

******/

#ifdef XRAY_DISABLE_GAMESPY_WARNINGS
#pragma warning(disable: 4267) //lines: 260, 357
#endif //#ifdef XRAY_DISABLE_GAMESPY_WARNINGS


/********
INCLUDES
********/
#include "../common/gsCommon.h"
#include "gbucket.h"
#include "../hashtable.h"
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

/********
TYPEDEFS
********/
struct bucketset_s
{
	HashTable buckets;
};


typedef struct bucket_s
{
	char *name;
	BucketType type;
	int nvals; //for averaging 
	union
	{
		int ival;
		double fval;
		char *sval;
	} vals;
} bucket_t;

typedef struct dumpdata_s
{
	char *data;
	unsigned int maxlen;
	unsigned int len;
} dumpdata_t;

/********
PROTOTYPES
********/
static void DumpMap(void *elem, void *clientData);
static void BucketFree(void *elem);
static int BucketCompare(const void *entry1, const void *entry2);
static int BucketHash(const void *elem, int numbuckets);
static void *DoSet(bucket_t *pbucket, void *value);
static void *DoGet(bucket_t *pbucket);
static bucket_t *DoFind(bucketset_t set, char *name);


/********
VARS
********/
bucketset_t g_buckets;

/****************************************************************************/
/* PUBLIC FUNCTIONS */
/****************************************************************************/
bucketset_t NewBucketSet(void)
{
	bucketset_t set;

	set = (bucketset_t)gsimalloc(sizeof (struct bucketset_s));
	assert(set);
	set->buckets = TableNew(sizeof(bucket_t),32,BucketHash, BucketCompare, BucketFree);

	g_buckets = set;
	return set;
}

void FreeBucketSet(bucketset_t set)
{
	assert(set);
	assert(set->buckets);
	TableFree(set->buckets);
	gsifree(set);
}

char *DumpBucketSet(bucketset_t set)
{
	dumpdata_t data;
	if (set == NULL)
		set = g_buckets;
	assert(set);
	data.data = (char *)gsimalloc(128); //alloc an initial buffer
	data.data[0] = 0;
	data.len = 0;
	data.maxlen = 128;
	TableMap(set->buckets,DumpMap, &data);
	return data.data;
}

void *BucketNew(bucketset_t set, char *name, BucketType type, void *initialvalue)
{
	bucket_t bucket;

	if (set == NULL)
		set = g_buckets;
	assert(set);
	bucket.name = goastrdup(name);
	bucket.type = type;
	bucket.vals.sval = NULL;
	bucket.nvals = 1;
	DoSet(&bucket, initialvalue);
	TableEnter(set->buckets,&bucket);
	return DoGet(DoFind(set, name));
}

void *BucketSet(bucketset_t set, char *name,void *value)
{
	bucket_t *pbucket = DoFind(set, name);	
	if (!pbucket)
		return NULL;

	pbucket->nvals = 0;
	return DoSet(pbucket,value);
}

void *BucketGet(bucketset_t set, char *name)
{
	return DoGet(DoFind(set,name));
}

void *BucketAdd(bucketset_t set, char *name, void *value)
{
	bucket_t *pbucket = DoFind(set, name);	
	if (!pbucket)
		return NULL;

	if (pbucket->type == bt_int)
		return DoSet(pbucket, bint( (*(int *)DoGet(pbucket)) + (*(int *)value)));
	if (pbucket->type == bt_float)
		return DoSet(pbucket, bfloat( (*(double *)DoGet(pbucket)) + (*(double *)value)));
	//else, string -- just concat
	return BucketConcat(set, name, value);
}

void *BucketSub(bucketset_t set, char *name, void *value)
{
	bucket_t *pbucket = DoFind(set, name);	
	if (!pbucket)
		return NULL;

	if (pbucket->type == bt_int)
		return DoSet(pbucket, bint( (*(int *)DoGet(pbucket)) - (*(int *)value)));
	if (pbucket->type == bt_float)
		return DoSet(pbucket, bfloat( (*(double *)DoGet(pbucket)) - (*(double *)value)));
	//else, string -- just  ignore
	return DoGet(pbucket);

}

void *BucketMult(bucketset_t set, char *name, void *value)
{
	bucket_t *pbucket = DoFind(set, name);	
	if (!pbucket)
		return NULL;

	if (pbucket->type == bt_int)
		return DoSet(pbucket, bint( (*(int *)DoGet(pbucket)) * (*(int *)value)));
	if (pbucket->type == bt_float)
		return DoSet(pbucket, bfloat( (*(double *)DoGet(pbucket)) * (*(double *)value)));
	//else, string -- just  ignore
	return DoGet(pbucket);
}

void *BucketDiv(bucketset_t set, char *name, void *value)
{
	bucket_t *pbucket = DoFind(set, name);	
	if (!pbucket)
		return NULL;

	if (pbucket->type == bt_int)
		return DoSet(pbucket, bint( (*(int *)DoGet(pbucket)) / (*(int *)value)));
	if (pbucket->type == bt_float)
		return DoSet(pbucket, bfloat( (*(double *)DoGet(pbucket)) / (*(double *)value)));
	//else, string -- just  ignore
	return DoGet(pbucket);
}

void *BucketConcat(bucketset_t set, char *name, void *value)
{
	bucket_t *pbucket = DoFind(set, name);	
	char *temp, *s;
	if (!pbucket)
		return NULL;

	assert(pbucket->type == bt_string);
	s = DoGet(pbucket);
	temp = (char *)gsimalloc(strlen(s) + strlen(value) + 1);
	strcpy(temp,s);
	strcat(temp, value);

	DoSet(pbucket, temp);
	gsifree(temp);
	
	return DoGet(pbucket);
}

#define AVG(cur, new, num) (((cur * num) + new) / (++num))
void *BucketAvg(bucketset_t set, char *name, void *value)
{
	bucket_t *pbucket = DoFind(set, name);	
	if (!pbucket)
		return NULL;
	
	if (pbucket->type == bt_int)
		return DoSet(pbucket, bint( AVG((*(int *)DoGet(pbucket)), (*(int *)value), pbucket->nvals)));
	if (pbucket->type == bt_float)
		return DoSet(pbucket, bfloat( AVG((*(double *)DoGet(pbucket)), (*(double *)value), pbucket->nvals)));
	//else, string -- just  ignore
	return DoGet(pbucket);
}

/* Note: these are NOT thread safe! */
void *bint(int i)
{
	static int j;
	j=i;
	return &j;
}

void *bfloat(double f)
{
	static double g;
	g=f;
	return &g;
}



/***********
 * UTILITY FUNCTIONS
 **********/
static void DumpMap(void *elem, void *clientData)
{
	bucket_t *bucket = (bucket_t *)elem;
	dumpdata_t *data = (dumpdata_t *)clientData;
	unsigned int minlen;

	//find out if we need to resize!
	minlen = strlen(bucket->name) + 3;
	if (bucket->type == bt_int || bucket->type == bt_float)
		minlen += data->len + 16;
	else if (bucket->type == bt_string)
		minlen += data->len + strlen(bucket->vals.sval);

	if (data->maxlen <= minlen) 
	{
		if (data->maxlen == 0)
			data->maxlen = minlen * 2;
		else
			data->maxlen *= 2;
		data->data = gsirealloc(data->data, data->maxlen);
		
	}

	switch (bucket->type)
	{	
	case bt_int:
		data->len += (unsigned int)sprintf(data->data + data->len,"\\%s\\%d",bucket->name,bucket->vals.ival);
		break;
	case bt_float:
		data->len += (unsigned int)sprintf(data->data + data->len,"\\%s\\%f",bucket->name,bucket->vals.fval);
		break;
	case bt_string:
		data->len += (unsigned int)sprintf(data->data + data->len,"\\%s\\%s",bucket->name,bucket->vals.sval);
		break;

	}
}


static char *stripchars(char *s)
{
	char *p = s;
	while (*s)
	{
		if (*s == '\\')
			*s = '/';
		s++;
	}
	return p;
}

static void *DoSet(bucket_t *pbucket, void *value)
{
	if (pbucket->type == bt_int)
		pbucket->vals.ival = *(int*)value;
	else if (pbucket->type == bt_float)
		pbucket->vals.fval = *(double *)value;
	else if (pbucket->type == bt_string)
	{
		if (pbucket->vals.sval != NULL)
			gsifree(pbucket->vals.sval);
		pbucket->vals.sval = (value == NULL ? NULL : stripchars(goastrdup((char *)value)));
	}
	return DoGet(pbucket);
}

static void *DoGet(bucket_t *pbucket)
{
	if (!pbucket)
		return NULL;
	if (pbucket->type == bt_string)
		return pbucket->vals.sval;
	else //since it's a union, we can return any member
		return &pbucket->vals.sval;
	
}

static bucket_t *DoFind(bucketset_t set, char *name)
{
	bucket_t tbucket;

	if (set == NULL)
		set = g_buckets;
	assert(set);

	tbucket.name = name;
	return (bucket_t *)TableLookup(set->buckets,&tbucket);
}



/* NonTermHash
 * ----------
 * The hash code is computed using a method called "linear congruence." 
 * This hash function has the additional feature of being case-insensitive,
 */
#define MULTIPLIER -1664117991
static int BucketHash(const void *elem, int numbuckets)
{
    unsigned int i;
    unsigned int len;
    unsigned int hashcode = 0;

	char *s = ((bucket_t *)elem)->name;
    len = strlen(s);
	for (i = 0; i < len ; i++) {
	  hashcode = (unsigned int)((int)hashcode * MULTIPLIER + tolower(s[i]));
	}
    return (int)(hashcode % numbuckets);
}


/* CaseInsensitiveCompare
 * ----------------------
 * Comparison function passed to qsort to sort an array of
 * strings in alphabetical order. It uses strcasecmp which is
 * identical to strcmp, except that it doesn't consider case of the
 * characters when comparing them, thus it sorts case-insensitively.
 */
static int CaseInsensitiveCompare(const void *entry1, const void *entry2)
{
    return strcasecmp(*(char **)entry1,*(char **)entry2);
}

/* keyval
 * Compares two buckets  (case insensative)
 */
static int BucketCompare(const void *entry1, const void *entry2)
{
  	
   	return CaseInsensitiveCompare(&((bucket_t *)entry1)->name, 
									  &((bucket_t *)entry2)->name);
}


/* KeyValFree
 * Frees the memory INSIDE a Bucket structure
 */
static void BucketFree(void *elem)
{
	gsifree(((bucket_t *)elem)->name);
	if (((bucket_t *)elem)->type == bt_string && ((bucket_t *)elem)->vals.sval != NULL)
		gsifree(((bucket_t *)elem)->vals.sval);

}

#ifdef __cplusplus
}
#endif
