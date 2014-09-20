#if !defined(AFX_NATIFY_H__B8FF4369_8789_4674_8569_3D52CE8CA890__INCLUDED_)
#define AFX_NATIFY_H__B8FF4369_8789_4674_8569_3D52CE8CA890__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define NATIFY_COOKIE		777
#define NATIFY_TIMEOUT		10000
#define NATIFY_STATUS_STEPS (NATIFY_TIMEOUT / 1000) + 7

typedef enum { packet_map1a, packet_map2, packet_map3, packet_map1b, NUM_PACKETS } NatifyPacket;
typedef enum { no_nat, firewall_only, full_cone, restricted_cone, port_restricted_cone, symmetric, unknown, NUM_NAT_TYPES } NatType;
typedef enum { promiscuous, not_promiscuous, port_promiscuous, ip_promiscuous, promiscuity_not_applicable, NUM_PROMISCUITY_TYPES } NatPromiscuity;
typedef enum { unrecognized, private_as_public, consistent_port, incremental, mixed, NUM_MAPPING_SCHEMES } NatMappingScheme;

typedef struct _AddressMapping {
	unsigned int privateIp;
	unsigned short privatePort;
	unsigned int publicIp;
	unsigned short publicPort;
} AddressMapping;

typedef struct _NAT {
	char brand[32];
	char model[32];
	char firmware[64];
	gsi_bool ipRestricted;
	gsi_bool portRestricted;
	NatPromiscuity promiscuity;
	NatType natType;
	NatMappingScheme mappingScheme;
	AddressMapping mappings[4];
	gsi_bool qr2Compatible;
} NAT;

int DiscoverReachability(SOCKET sock, unsigned int ip, unsigned short port, int portType);
int DiscoverMapping(SOCKET sock, unsigned int ip, unsigned short port, int portType, int id);
int NatifyThink(SOCKET sock, NAT * nat);
gsi_bool DetermineNatType(NAT * nat);
void OutputMapping(const AddressMapping * theMap);


#endif // !defined(AFX_NATIFY_H__B8FF4369_8789_4674_8569_3D52CE8CA890__INCLUDED_)
