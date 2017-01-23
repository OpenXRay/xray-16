#include "stdafx.h"
#pragma hdrstop

/* CRC16.C  -  by Rob Duff, Vancouver, BC, Canada.  Not copyrighted. */
/*
This algorithm is adapted from the classic paper
"Byte-wise CRC Calculations", Aram Perez
IEEE Micro June 1983 pp 40-50

CRC16 is used by the popular ARC archiving program.
CCITT is used by XMODEM, SDLC by IBM corp.

gen16(0) will create table for CRC16
gen16(1) for CCITT and start crc at -1 for SDLC
*/

static BOOL crc16_ready		= FALSE;
static int	crc16_table		[256];					// Lookup table array 

int gen_tab[2][8] = {
	{ 0xC0C1, 0xC181, 0xC301, 0xC601, 0xCC01, 0xD801, 0xF001, 0xA001 },
	{ 0x1189, 0x2312, 0x4624, 0x8C48, 0x1081, 0x2102, 0x4204, 0x8408 }
};

void	crc16_init(unsigned p)
{
	int     i, j, val;

	for (i = 0; i < 256; i++) {
		val = 0;
		for (j = 0; j < 8; j++)
			if (i & (1<<j))
				val ^= gen_tab[p][j];
		crc16_table[i] = val;
	}
}

unsigned crc16_calc(unsigned char *data, unsigned count, unsigned old_crc=0)
{
	if (!crc16_ready)	
	{
		crc16_init	(0);
		crc16_ready	= TRUE;
	}

	union {
		unsigned w;
		struct {
			unsigned char lo;
			unsigned char hi;
		} b;
	} crc;

	crc.w = old_crc;
	while (count-- != 0)
		crc.w = crc16_table[*data++ ^ crc.b.lo] ^ crc.b.hi;
	return crc.w;
}
/*
void usage()
{
	fprintf(stderr, "usage: crc16 [-c][-s] file...\n");
	fprintf(stderr, "       wildcards are permitted\n");
	fprintf(stderr, "       default is CRC16 (ARC)\n");
	fprintf(stderr, "       -c gives CCITT   (XMODEM)\n");
	fprintf(stderr, "       -s gives SDLC    (IBM)\n");
	exit(1);
}

char    fname[64];
unsigned char buffer[20][512];

main(argc, argv)
char *argv[];
{
	int     fd;
	unsigned n, crc;
	int     poly, init;

	argc--; argv++;
	if (argv > 0 && argv[0][0] == '-') {
		if (argv[0][1] == 'c') {
			poly = 1;
			init = 0;
		}
		if (argv[0][1] == 's') {
			poly = 1;
			init = -1;
		}
		argc--; argv++;
	}
	else {
		poly = 0;
		init = 0;
	}
	if (argc < 1)
		usage();
	gen16(poly);
	while (argc) {
		while (findfile(fname, *argv, 0)) {
			fd = open(fname, O_RDONLY | O_BINARY);
			if (fd == -1)
				break;
			crc = init;
			do {
				n = read(fd, buffer[0], sizeof(buffer));
				crc = crc16(buffer[0], n, crc);
			} while (n == sizeof(buffer));
			close(fd);
			printf("crc of %s is %04X\n", fname, crc);
		}
		argc--; argv++;
	}
	return 0;
}
*/