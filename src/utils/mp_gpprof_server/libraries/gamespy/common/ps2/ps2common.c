#include "../gsCommon.h"

// One of the following network devices must be defined
#if 0
#define T10K_ETHERNET
#endif
#if 0
#define USB_ETHERNET
#endif
#if 0
#define USB_ETHERNET_WITH_PPPOE
#endif
#if 1
#define HDD_ETHERNET
#endif
#if 0
#define HDD_ETHERNET_WITH_PPPOE
#endif
#if 0
#define	MODEM
#endif

// attempts to load a module
static int load_module(const char * filename, int args, const char * argp)
{
	int result;

	result = sceSifLoadModule(filename, args, argp);
	if(result < 0)
	{
		const char * errorString;
		if(result == -100)
			errorString = "Called from exception handler / interrupt handler";
		else if(result == -200)
			errorString = "Resident library required by loaded module does not exist";
		else if(result == -201)
			errorString = "Object file format is invalid";
		else if(result == -203)
			errorString = "Specified file was not found";
		else if(result == -204)
			errorString = "Error occurred when reading file";
		else if(result == -400)
			errorString = "Insufficient memory";
		else if(result == -0x10000)
			errorString = "Binding to the IOP module failed.";
		else if(result == -0x10001)
			errorString = "RPC to the IOP failed.";
		else if(result == -0x10004)
			errorString = "The IOP module version does not match.";
		else
			errorString = "Unknown error";
		scePrintf("FAILED TO LOAD MODULE: %s\n", filename);
		scePrintf("\t(%d:%s)\n", result, errorString);
		while(1)
			msleep(1);
	}

	return result;
}

#define SCEROOT           "host0:/usr/local/sce/"
#define MODROOT           SCEROOT "iop/modules/"
#define APPROOT           SCEROOT "conf/neticon/english/"

// SN Systems stack
#ifdef SN_SYSTEMS

// these only need to be set if using DHCP
// up to four DNS servers can be specified, and the list must be terminated with an empty string
#define SNPS2_IP_ADDR   "0.0.0.0"
#define SNPS2_SUB_MSK   "0.0.0.0"
#define SNPS2_GATEWAY   "0.0.0.0"
static const sn_char* dns_servers[] = { "" };

static void load_network_modules()
{
	// load the TCP/IP stack module
#ifndef _DEBUG
    load_module(MODROOT "snstkrel.irx", 0, NULL);
#else
	const char iop_params[] 
		= SNPS2_IP_ADDR "\x00" SNPS2_SUB_MSK "\x00" SNPS2_GATEWAY;

	load_module(MODROOT "snstkdbg.irx", sizeof(iop_params), (char*) iop_params);
#endif

	// load device specific module(s)
#if defined(T10K_ETHERNET)
    load_module(MODROOT "sndrv000.irx", 0, NULL);

#elif defined(USB_ETHERNET)
	load_module(MODROOT "usbd.irx", 0, NULL);
	#if defined(USB_LUCENT)
		load_module(MODROOT "sndrv002.irx", 0, NULL);
	#elif defined(USB_CONEXANT)
    	load_module(MODROOT "sndrv003.irx", 0, NULL);
	#else
    	load_module(MODROOT "sndrv001.irx", 0, NULL);
	#endif

#elif defined(USB_ETHERNET_WITH_PPPOE)
    load_module(MODROOT "usbd.irx",     0, NULL);
	load_module(MODROOT "sndrv200.irx", 0, NULL);
	load_module(MODROOT "sndrv201.irx", 0, NULL);

#elif defined(HDD_ETHERNET)
	load_module(MODROOT "dev9.irx",     0, NULL);
	load_module(MODROOT "sndrv100.irx", 0, NULL);
	load_module(MODROOT "smap.irx",     0, NULL);

#elif defined(HDD_ETHERNET_WITH_PPPOE)
	load_module(MODROOT "dev9.irx",     0, NULL);
	load_module(MODROOT "sndrv200.irx", 0, NULL);
	load_module(MODROOT "sndrv202.irx", 0, NULL);
	load_module(MODROOT "smap.irx",     0, NULL);

#elif defined(MODEM)
	load_module(MODROOT "dev9.irx",     0, NULL);
	load_module(MODROOT "sndrv101.irx", 0, NULL);
	load_module(MODROOT "spduart.irx",  0, NULL);
#endif
}

static sn_int32 do_initialisation(void)
{
    sn_int32 result;
    sn_int32 device_attached;
    sn_int16 idVendor;
    sn_int16 idProduct;
    sn_int32 stack_state;
    sndev_set_ether_ip_type params;
    struct hostent * host;
    sn_bool got_ip;
    struct in_addr addr;

	// load network modules
	load_network_modules();

	// init the socket API
	scePrintf("Initializing the sockets API\n");
    result = sockAPIinit(1);
    if(result != 0)
    {
        scePrintf("sockAPIinit() failed %d\n", result);
        return result;
    }

	// register this thread with the socket API
    result = sockAPIregthr();
    if(result != 0)
    {
        scePrintf("sockAPIregthr() failed %d\n", result);
        return result;
    }

	// wait for the device adaptor to be attached
    scePrintf("Waiting for network device to be initialized\n");
    device_attached = SN_DEV_TYPE_NONE;
    while(device_attached == SN_DEV_TYPE_NONE)
    {
    	// check if attached
        result = sndev_get_attached(0, &device_attached, &idVendor, &idProduct);
        if(result != 0)
        {
            scePrintf("sndev_get_attached() failed %d\n", result);
            return result;
        }

		// if nothing attached, give it a rest before trying again
        if(device_attached == SN_DEV_TYPE_NONE)
            sn_delay(10);
    }

    scePrintf("Device ready (idVendor=0x%04X idProduct=0x%04X)\n",	((int)idVendor) & 0xFFFF, ((int)idProduct) & 0xFFFF);

	// set the DNS servers
    result = sntc_set_dns_server_list((const sn_char**)dns_servers);
    if(result != 0)
    {
        scePrintf("sntc_set_dns_server_list() failed %d\n", result);
        return result;
    }

    // set the IP, subnet mask, and gateway (all 0's for DHCP)
    inet_aton(SNPS2_IP_ADDR, (struct in_addr*)&params.ip_addr);
    inet_aton(SNPS2_SUB_MSK, (struct in_addr*)&params.sub_mask);
    inet_aton(SNPS2_GATEWAY, (struct in_addr*)&params.gateway);
    result = sndev_set_options(0, SN_DEV_SET_ETHER_IP, &params, sizeof(params));
    if(result != 0)
    {
        scePrintf("sndev_set_options() failed %d\n", result);
        return result;
    }
    
	// start the stack
    scePrintf("Starting the TCP/IP stack\n");
    result = sn_stack_state(SN_STACK_STATE_START, &stack_state);
    if(result != 0)
    {
        scePrintf("sn_stack_sate() failed %d\n", result);
        return result;
    }

	// wait for the stack to come up
	while(sn_socket_api_ready() == SN_FALSE)
    	sn_delay(100);

    // wait for a non-zero IP (for DHCP)
    scePrintf("Waiting for DHCP-supplied IP\n");
    got_ip = SN_FALSE;
    do
    {
    	// get the local host info
    	host = gethostbyname(LOCAL_NAME);
    	if(host && host->h_addr_list[0])
    	{
    		// copy the IP
    		memcpy(&addr, host->h_addr_list[0], sizeof(addr));
    		if(addr.s_addr)
    		{
    			got_ip = SN_TRUE;
				scePrintf("DHCP allocated IP addr %s\n", inet_ntoa(addr));
    		}
    	}

    	// don't hog the CPU
    	if(got_ip == SN_FALSE)
    		sn_delay(100);
    }
    while(got_ip == SN_FALSE);

    return 0;
}

static void do_shutdown(void)
{
	int result;
	int stack_state;

	// stopping the stack
	result = sn_stack_state(SN_STACK_STATE_STOP, &stack_state);
	if(result != 0)
	{
		scePrintf("sn_stack_sate() failed %d\n", result);
		return;
	}

	// deregister this thread with the socket API
	sockAPIderegthr();
}
#endif // SN_SYSTEMS

#ifdef EENET

#ifdef __MWERKS__
#include "/ee/sample/libeenet/ent_cnf/ent_cnf.h"
#else
#include "/usr/local/sce/ee/sample/libeenet/ent_cnf/ent_cnf.h"
#endif

#if defined(USB_ETHERNET)
#define USR_CONF_NAME     "Combination4"
#elif defined(USB_ETHERNET_WITH_PPPOE)
#define USR_CONF_NAME     "Combination5"
#elif defined(HDD_ETHERNET)
#define USR_CONF_NAME     "Combination6"
#elif defined(HDD_ETHERNET_WITH_PPPOE)
#define USR_CONF_NAME     "Combination7"
#elif defined(MODEM)
#define USR_CONF_NAME     ""
#endif

#define	MOD_NETCNF        MODROOT "netcnf.irx"
#define MOD_EENETCTL      MODROOT "eenetctl.irx"
#define MOD_DEV9          MODROOT "dev9.irx"
#define MOD_USBD          MODROOT "usbd.irx"
#define MOD_ENT_DEVM      MODROOT "ent_devm.irx"
#define MOD_ENT_SMAP      MODROOT "ent_smap.irx"
#define MOD_ENT_ETH       MODROOT "ent_eth.irx"
#define MOD_ENT_PPP       MODROOT "ent_ppp.irx"
#define	MOD_ENT_CNF       SCEROOT "iop/sample/libeenet/ent_cnf/ent_cnf.irx"
#define MOD_MODEMDRV      ""

#define	INETCTL_ARG       "-no_auto" "\0" "-no_decode"
#define	NETCNF_ICON       APPROOT "SYS_NET.ICO"
#define	NETCNF_ICONSYS    APPROOT "icon.sys"
#define	NETCNF_ARG        "icon=" NETCNF_ICON "\0" "iconsys=" NETCNF_ICONSYS
#define MODEMDRV_ARG      ""

#define NET_DB            SCEROOT "conf/net/net.db"

#define EENET_MEMSIZE     (512 * 1024)
#define EENET_TPL         32
#define EENET_APP_PRIO    48

static int eenetctl_mid;
static int ent_cnf_mid;
static int ent_devm_mid;
#if defined(HDD_ETHERNET) || defined(HDD_ETHERNET_WITH_PPPOE)
#define EENET_IFNAME "smap0"
static int ent_smap_mid;
#endif
#if defined(USB_ETHERNET) || defined(USB_ETHERNET_WITH_PPPOE)
#define EENET_IFNAME "eth0"
static int ent_eth_mid;
#endif
#if defined(MODEM)
#define EENET_IFNAME "ppp0"
static int ent_ppp_mid;
static int modem_mid;
#endif

static void * eenet_pool;

static int sema_id;
static int event_flag = 0;
#define Ev_Attach          0x01
#define Ev_UpCompleted     0x02
#define Ev_DownCompleted   0x04
#define Ev_DetachCompleted 0x08

#define WaitEvent(event) \
	while(1){ \
		WaitSema(sema_id); \
		if(event_flag & (event)) \
			break; \
	}

static void event_handler(const char *ifname, int af, int type)
{
	scePrintf("event_handler: event happened. af = %d, type = %d\n", af, type);

	switch(type)
	{
	case sceEENETCTL_IEV_Attach:
		if(strcmp(ifname, EENET_IFNAME))
			break;
		event_flag |= Ev_Attach;
		SignalSema(sema_id);
		break;
	case sceEENETCTL_IEV_UpCompleted:
		event_flag |= Ev_UpCompleted;
		SignalSema(sema_id);
		break;
	case sceEENETCTL_IEV_DownCompleted:
		event_flag |= Ev_DownCompleted;
		SignalSema(sema_id);
		break;
	case sceEENETCTL_IEV_DetachCompleted:
		if(strcmp(ifname, EENET_IFNAME))
			break;
		event_flag |= Ev_DetachCompleted;
		SignalSema(sema_id);
		break;
	}

	return;
}

static int create_sema(int init_count, int max_count){
	struct SemaParam sema_param;
	int sema_id;

	memset(&sema_param, 0, sizeof(struct SemaParam));
	sema_param.initCount = init_count;
	sema_param.maxCount = max_count;
	sema_id = CreateSema(&sema_param);

	return sema_id;
}

// ent_cnf sifrpc num
#define ENT_CNF_SIFRPC_NUM 0x0a31108e

// code
#define ENT_CNF_SIFRPC_LOAD_CONFIG      1
#define ENT_CNF_SIFRPC_SET_CONFIG       2
#define ENT_CNF_SIFRPC_SET_SIFCMD       3

// utility macro
#define ee_rpc_size(size) ((size + 15) & ~15)
#define iop_rpc_size(size) ((size + 3) & ~3)

#define NETBUFSIZE 512
#define RPCSIZE NETBUFSIZE * 4

static sceSifClientData cd;
static u_int rpc_buffer[NETBUFSIZE]  __attribute__((aligned(64)));

int ent_cnf_init(void)
{
	int i;

	/* bind rpc */
	while(1){
		if (sceSifBindRpc(&cd, ENT_CNF_SIFRPC_NUM, 0) < 0) {
			scePrintf("ent_cnf: sceSifBindRpc failed.\n");
			while(1) {};
		}
		if (cd.serve != 0) break;
		i = 0x10000;
		while(i --) {};
	}

	return 0;
}

int ent_cnf_load_config(const char *fname, const char *usr_name)
{
	int ret, len;

	strcpy((char *)rpc_buffer, fname);
	len = (int)strlen(fname) + 1;
	strcpy((char *)rpc_buffer + len, usr_name);
	len += (int)strlen(usr_name) + 1;

	ret = sceSifCallRpc(&cd, ENT_CNF_SIFRPC_LOAD_CONFIG, 0,
		(void *)rpc_buffer, ee_rpc_size(len),
		(void *)rpc_buffer, ee_rpc_size(sizeof(u_int)), 0, 0);
	if(ret < 0){
		scePrintf("ent_cnf: RPC call in ent_cnf_load_config() failed.\n");
		return -1;
	}

	return (int)rpc_buffer[0];
}

int ent_cnf_set_config(void)
{
	int ret;

	ret = sceSifCallRpc(&cd, ENT_CNF_SIFRPC_SET_CONFIG, 0,
		NULL, 0, (void *)rpc_buffer, ee_rpc_size(sizeof(u_int)), 0, 0);
	if(ret < 0){
		scePrintf("ent_cnf: RPC call in ent_cnf_set_config() failed.\n");
		return -1;
	}

	return (int)rpc_buffer[0];
}

static void load_network_modules()
{
	ent_devm_mid = load_module(MOD_ENT_DEVM, 0, NULL);
	load_module(MOD_NETCNF, sizeof(NETCNF_ARG), NETCNF_ARG);
	eenetctl_mid = load_module(MOD_EENETCTL, 0, NULL);
	ent_cnf_mid = load_module(MOD_ENT_CNF, 0, NULL);
#if defined( HDD_ETHERNET ) || defined( HDD_ETHERNET_WITH_PPPOE )
	load_module(MOD_DEV9, 0, NULL);
	ent_smap_mid = load_module(MOD_ENT_SMAP, 0, NULL);
	//PreparePowerOff();
#endif
#if defined( USB_ETHERNET ) || defined( USB_ETHERNET_WITH_PPPOE )
	load_module(MOD_USBD, 0, NULL);
	ent_eth_mid = load_module(MOD_ENT_ETH, 0, NULL);
#endif
#if defined( MODEM )
	ent_ppp_mid = load_module(MOD_ENT_PPP, 0, NULL);
	modem_mid = load_module(MOD_MODEMDRV, sizeof(MODEMDRV_ARG), MODEMDRV_ARG);
#endif
}

static int do_initialisation(void)
{
	int rcode;

	// create a semaphore
	sema_id = create_sema(0, 1);
	if(sema_id < 0)
	{
		scePrintf("create_sema() failed.\n");
		return -1;
	}
	
	// load network modules
	load_network_modules();

	// allocate memory for EENet
	//eenet_pool = gsimemalign(64, EENET_MEMSIZE);
	eenet_pool = memalign(64, EENET_MEMSIZE);
	if(eenet_pool == NULL)
		return -1;

	// initialize eenet
	rcode = sceEENetInit(eenet_pool, EENET_MEMSIZE, EENET_TPL, 8192, EENET_APP_PRIO);


	if(rcode < 0)
	{
		scePrintf("sceEENetInit failed (%d)\n", rcode);
		return -1;
	}

#ifdef _DEBUG
	sceEENetSetLogLevel(EENET_LOG_DEBUG);
#endif

	// init eenetctl.a
	rcode = sceEENetCtlInit(8192, 32, 8192, 32, 8192, 32, 1, 0);
	if(rcode < 0)
	{
		scePrintf("sceEENetCtlInit failed (%d)\n", rcode);
		return -1;
	}

	// add event handler
	rcode = sceEENetCtlRegisterEventHandler(event_handler);
	if(rcode < 0)
	{
		scePrintf("sceEENetCtlRegisterEventHandler failed (%d)\n", rcode);
		return -1;
	}

	// init ent_cnf
	rcode = ent_cnf_init();
	if(rcode < 0)
	{
		scePrintf("ent_cnf_init failed (%d)\n", rcode);
		return -1;
	}

	// register network interface driver
#if defined( HDD_ETHERNET ) || defined( HDD_ETHERNET_WITH_PPPOE )
	rcode = sceEENetDeviceSMAPReg(8192, 8192);
	if(rcode < 0)
	{
		scePrintf("sceEENetDeviceSMAPReg failed (%d)\n", rcode);
		return -1;
	}
#endif
#if defined( USB_ETHERNET ) || defined( USB_ETHERNET_WITH_PPPOE )
	rcode = sceEENetDeviceETHReg(8192, 8192);
	if(rcode < 0)
	{
		scePrintf("sceEENetDeviceETHReg failed (%d)\n", rcode);
		return -1;
	}
#endif
#if defined( MODEM )
	rcode = sceEENetDevicePPPReg(8192, 8192);
	if(rcode < 0)
	{
		scePrintf("sceEENetDevicePPPReg failed (%d)\n", rcode);
		return -1;
	}
#endif

	// wait until target interface is attached
	WaitEvent(Ev_Attach);

	// load network configuration
	rcode = ent_cnf_load_config(NET_DB, USR_CONF_NAME);
	if(rcode < 0)
	{
		scePrintf("ent_cnf_load_config failed (%d)\n", rcode);
		return -1;
	}

	// set network configuration
	rcode = ent_cnf_set_config();
	if(rcode < 0)
	{
		scePrintf("ent_cnf_set_config failed (%d)\n", rcode);
		return -1;
	}

	// wait for interface initialization to complete
	WaitEvent(Ev_UpCompleted);

	return 0;
}

static void do_shutdown(void)
{
	int rcode;
	
	// bring down the interface
	rcode = sceEENetCtlDownInterface(EENET_IFNAME);
	if(rcode < 0)
	{
		scePrintf("sceEENetCtlDownInterface failed (%d)\n", rcode);
		return;
	}

	// wait for the termination to complete
	WaitEvent(Ev_DownCompleted);

	// unload the driver module
#if defined( HDD_ETHERNET ) || defined( HDD_ETHERNET_WITH_PPPOE )
	rcode = sceEENetDeviceSMAPUnreg();
	if(rcode < 0)
	{
		scePrintf("sceEENetDeviceSMAPUnreg failed (%d)\n", rcode);
		return;
	}
#endif
#if defined( USB_ETHERNET ) || defined( USB_ETHERNET_WITH_PPPOE )
	rcode = sceEENetDeviceETHUnreg();
	if(rcode < 0)
	{
		scePrintf("sceEENetDeviceETHUnreg failed (%d)\n", rcode);
		return;
	}
#endif
#if defined( MODEM )
	rcode = sceEENetDevicePPPUnreg();
	if(rcode < 0)
	{
		scePrintf("sceEENetDevicePPPUnreg failed (%d)\n", rcode);
		return;
	}
#endif

	// wait for the detach to complete
	WaitEvent(Ev_DetachCompleted);

	// cancel the event handler
	rcode = sceEENetCtlUnregisterEventHandler(event_handler);
	if(rcode < 0)
	{
		scePrintf("sceEENetCtlUnRegisterEventHandler failed (%d)\n", rcode);
		return;
	}

	// eenetctl.a termination processing
	rcode = sceEENetCtlTerm();
	if(rcode < 0)
	{
		scePrintf("sceEENetCtlTerm failed (%d)\n", rcode);
		return;
	}

	// terminate eenet
	rcode = sceEENetTerm();
	if(rcode < 0)
	{
		scePrintf("sceEENetTerm failed (%d)\n", rcode);
		return;
	}

	// stop and unload modules
#if defined( HDD_ETHERNET ) || defined( HDD_ETHERNET_WITH_PPPOE )
	sceSifStopModule(ent_smap_mid, 0, NULL, &rcode);
	sceSifUnloadModule(ent_smap_mid);
#endif
#if defined( USB_ETHERNET ) || defined( USB_ETHERNET_WITH_PPPOE )
	sceSifStopModule(ent_eth_mid, 0, NULL, &rcode);
	sceSifUnloadModule(ent_eth_mid);
#endif
#if defined( MODEM )
	sceSifStopModule(modem_mid, 0, NULL, &rcode);
	sceSifUnloadModule(modem_mid);

	sceSifStopModule(ent_ppp_mid, 0, NULL, &rcode);
	sceSifUnloadModule(ent_ppp_mid);
#endif

	sceSifStopModule(ent_cnf_mid, 0, NULL, &rcode);
	sceSifUnloadModule(ent_cnf_mid);

	sceSifStopModule(eenetctl_mid, 0, NULL, &rcode);
	sceSifUnloadModule(eenetctl_mid);

	sceSifStopModule(ent_devm_mid, 0, NULL, &rcode);
	sceSifUnloadModule(ent_devm_mid);

	// free the EENet pool
	if(eenet_pool != NULL)
		free(eenet_pool);
}

#endif

#ifdef INSOCK

// These are also used in nonport.c to obtain the MAC address
sceSifMClientData gGSIInsockClientData;
u_int             gGSIInsockSocketBuffer[NETBUFSIZE] __attribute__((aligned(64)));


// IRX paths
#define	IOP_MOD_INET      MODROOT "inet.irx"
#define	IOP_MOD_AN986     MODROOT "an986.irx"
#define	IOP_MOD_USBD      MODROOT "usbd.irx"
#define	IOP_MOD_NETCNF    MODROOT "netcnf.irx"
#define	IOP_MOD_INETCTL   MODROOT "inetctl.irx"
#define	IOP_MOD_MSIFRPC   MODROOT "msifrpc.irx"
#define	IOP_MOD_DEV9      MODROOT "dev9.irx"
#define	IOP_MOD_SMAP      MODROOT "smap.irx"
#define	IOP_MOD_PPP       MODROOT "ppp.irx"
#define	IOP_MOD_PPPOE     MODROOT "pppoe.irx"
#define	IOP_MOD_LIBNET    MODROOT "libnet.irx"
#define IOP_MOD_NETCNFIF  MODROOT "netcnfif.irx"
#define	IOP_MOD_INETLOG   SCEROOT "iop/util/inet/inetlog.irx"
#define	IOP_MOD_MODEMDRV  ""
#define	NET_DB            SCEROOT "conf/net/net.db"
#define	INET_ARG          "debug=18"
#define	INETCTL_ARG       "-no_auto" "\0" "-no_decode"
#define	LIBNET_ARG        "-verbose"
#define	NETCNF_ICON       APPROOT "SYS_NET.ICO"
#define	NETCNF_ICONSYS    APPROOT "icon.sys"
#define	NETCNF_ARG        "icon=" NETCNF_ICON "\0" "iconsys=" NETCNF_ICONSYS
#define	MODEMDRV_ARG      ""

#if	defined( USB_ETHERNET )
#define	USR_CONF_NAME     "Combination4"
#elif	defined( USB_ETHERNET_WITH_PPPOE )
#define	USR_CONF_NAME     "Combination5"
#elif	defined( HDD_ETHERNET )
#define	USR_CONF_NAME     "Combination6"
#elif	defined( HDD_ETHERNET_WITH_PPPOE )
#define	USR_CONF_NAME     "Combination7"
#elif	defined( MODEM )
#define	USR_CONF_NAME     ""
#endif

static int do_initialisation(void)
{
	int result;
	int i;
	int if_id[sceLIBNET_MAX_INTERFACE];
	sceInetAddress_t myaddr;

	sceSifInitRpc( 0 );
	
	load_module( IOP_MOD_INET, 0, NULL );
	load_module( IOP_MOD_NETCNF, sizeof( NETCNF_ARG ), NETCNF_ARG );
	load_module( IOP_MOD_INETCTL, sizeof( INETCTL_ARG ), INETCTL_ARG );

#if defined( USB_ETHERNET ) || defined( USB_ETHERNET_WITH_PPPOE )
	load_module( IOP_MOD_USBD, 0, NULL );
	load_module( IOP_MOD_AN986, 0, NULL );
#endif

#if defined( HDD_ETHERNET ) || defined( HDD_ETHERNET_WITH_PPPOE )
	load_module( IOP_MOD_DEV9, 0, NULL );
	load_module( IOP_MOD_SMAP, 0, NULL );
#endif

#if defined( USB_ETHERNET_WITH_PPPOE ) || defined( HDD_ETHERNET_WITH_PPPOE )
	load_module( IOP_MOD_PPP, 0, NULL );
	load_module( IOP_MOD_PPPOE, 0, NULL );
#endif

#if defined( MODEM )
	load_module( IOP_MOD_PPP, 0, NULL );
	load_module( IOP_MOD_USBD, 0, NULL );
	load_module( IOP_MOD_MODEMDRV, sizeof( MODEMDRV_ARG ), MODEMDRV_ARG );
#endif

	load_module( IOP_MOD_MSIFRPC, 0, NULL );
	load_module( IOP_MOD_LIBNET, sizeof( LIBNET_ARG ), LIBNET_ARG );
	load_module( IOP_MOD_NETCNFIF, 0, NULL );

#if defined( HDD_ETHERNET ) || defined( HDD_ETHERNET_WITH_PPPOE )
//	PreparePowerOff();
#endif

	// Initialize Libnet
	sceSifMInitRpc(0);

	result = sceInsockSetSifMBindRpcValue(NETBUFSIZE, sceLIBNET_STACKSIZE, sceLIBNET_PRIORITY);
	if (result < 0)
		return result;

	result = sceLibnetInitialize(&gGSIInsockClientData, NETBUFSIZE, sceLIBNET_STACKSIZE, sceLIBNET_PRIORITY);
	if (result < 0)
		return result;

	result = sceLibnetRegisterHandler(&gGSIInsockClientData, gGSIInsockSocketBuffer);
	if (result < 0)
		return result;

	result = load_set_conf_extra(&gGSIInsockClientData, gGSIInsockSocketBuffer, NET_DB, USR_CONF_NAME, sceLIBNETF_AUTO_UPIF);
	if (result < 0)
		return result;

	result = sceLibnetWaitGetAddress( &gGSIInsockClientData, gGSIInsockSocketBuffer, if_id, sceLIBNET_MAX_INTERFACE, &myaddr, sceLIBNETF_AUTO_UPIF );
	if (result < 0)
		return result;

	for ( i = 0; i < sceLIBNET_MAX_INTERFACE; i++ ) {
		if ( if_id[ i ] == 0 ) {
			break;
		}
		scePrintf( "interface: %d\n", if_id[ i ] );
	}

	return 0;
}

static void do_shutdown(void)
{
	// Release the network interface
	down_interface(&gGSIInsockClientData, gGSIInsockSocketBuffer, 0);

	// Shutdown libnet
	libnet_term(&gGSIInsockClientData);

	sceSifMExitRpc();
}

#endif // INSOCK

#ifdef GSI_VOICE
void load_voice_modules(void); // prototype so codewarrior will be happy
void load_voice_modules(void)
{
	// this is the maximum size of a raw frame, in bytes
	char lgaudArgs[] = "maxstream=512\n";

	// load the usb module
	load_module("host0:/usr/local/sce/iop/modules/usbd.irx", 0, NULL);

	// load the lgAud module
	// see the lgAud documentation for info on optional loading parameters
	load_module("host0:/usr/local/sce/iop/modules/lgaud.irx", sizeof(lgaudArgs), lgaudArgs);

	// load the lgVid module
	// see the lgVid documentation for info on optional loading parameters
	load_module("host0:/usr/local/sce/iop/modules/lgvid.irx", 0, NULL);

	// load the SPU2 modules
	load_module("host0:/usr/local/sce/iop/modules/libsd.irx", 0, NULL);
	load_module("host0:/usr/local/sce/iop/modules/sdrdrv.irx", 0, NULL);
}
#endif

// New hooks required by crt0.s
#if !defined(__MWERKS__) 
int _init(){ return 0; }
int _fini(){ return 0; }

  #if (__GNUC__ >= 3)
    int __main(int argc, char ** argp){ GSI_UNUSED(argp); GSI_UNUSED(argc); return 0; }
  #endif
#endif

extern int test_main(int argc, char ** argp);
int main(int argc, char ** argp)
{
    int result = 0;
    
	GSI_UNUSED(argc);
	GSI_UNUSED(argp);

    printf("\nGameSpy Test App Initializing\n" 
	       "----------------------------------\n");

	// init RPC
    sceSifInitRpc(0);

    // initialize the stack
	result = (int)do_initialisation();
    if(result)
    {
        printf("Initialization failed\n");
        return result;
    }

#ifdef GSI_VOICE
	load_voice_modules();
#endif

	// start the actual program
    printf("\nGameSpy Test App Starting\n" 
	       "----------------------------------\n");
	test_main(argc, argp);

	// do any needed cleanup
	do_shutdown();
    printf("\nGameSpy Test App Exiting\n" 
	       "----------------------------------\n");

    return 0;
}
