#ifndef _INC_CPUID
#define _INC_CPUID

#define _CPU_FEATURE_MMX    0x0001
#define _CPU_FEATURE_SSE    0x0002
#define _CPU_FEATURE_SSE2   0x0004
#define _CPU_FEATURE_3DNOW  0x0008

#define _CPU_FEATURE_SSE3	0x0010
#define _CPU_FEATURE_SSSE3	0x0020
#define _CPU_FEATURE_SSE4_1 0x0040
#define _CPU_FEATURE_SSE4_2	0x0080

#define _CPU_FEATURE_MWAIT	0x0100

struct _processor_info {
    char		v_name[13];							// vendor name
    char		model_name[49];						// Name of model eg. Intel_Pentium_Pro
    
	unsigned char family;							// family of the processor, eg. Intel_Pentium_Pro is family 6 processor
    unsigned char model;							// model of processor, eg. Intel_Pentium_Pro is model 1 of family 6 processor
    unsigned char stepping;							// Processor revision number
    
	unsigned int feature;							// processor Feature ( same as return value).
	unsigned int n_threads;							// number of available threads, both physical and logical
};

int _cpuid ( _processor_info * );

#endif
