/* NeuQuant Neural-Net Quantization Algorithm
* ------------------------------------------
*
* Copyright (c) 1994 Anthony Dekker
*
* NEUQUANT Neural-Net quantization algorithm by Anthony Dekker, 1994.
* See "Kohonen neural networks for optimal colour quantization"
* in "Network: Computation in Neural Systems" Vol. 5 (1994) pp 351-367.
* for a discussion of the algorithm.
* See also  http://www.acm.org/~dekker/NEUQUANT.HTML
*
* Any party obtaining a copy of these files from the author, directly or
* indirectly, is granted, free of charge, a full and unrestricted irrevocable,
* world-wide, paid up, royalty-free, nonexclusive right and license to deal
* in this software and documentation files (the "Software"), including without
* limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons who receive
* copies from any such party to do so, with the only requirement being
* that this copyright notice remain intact.
*/

//-----------------------------------------------------------------------------
//
// ImageLib Sources
// by Denton Woods
// Last modified: 02/02/2002 <--Y2K Compliant! =]
//
// Filename: src-/src/il_neuquant.c
//
// Description: Heavily modified by Denton Woods.
//
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "compiler.h"

// four primes near 500 - assume no image has a length so large
// that it is divisible by all four primes
#define dimension		4
const u32 prime[dimension + 1] = {499, 491, 487, 503, 509};
#define minpicturebytes	(dimension*prime[dimension])			// minimum size for input image
// Network Definitions
// -------------------
#define netsize			256					// number of colours used
#define maxnetpos		(netsize-1)
#define netbiasshift	4					// bias for colour values
#define ncycles			100					// no. of learning cycles

// defs for freq and bias
#define intbiasshift	16					// bias for fractions
#define intbias			(((int) 1)<<intbiasshift)
#define gammashift		10					// gamma = 1024
#define gamma			(((int) 1)<<gammashift)
#define betashift		10
#define beta			(intbias>>betashift)// beta = 1/1024
#define betagamma		(intbias<<(gammashift-betashift))

// defs for decreasing radius factor
#define initrad			(netsize>>dimension)		// for netsize cols, radius starts
#define radiusbiasshift	6							// at 32.0 biased by 6 bits
#define radiusbias		(((int) 1)<<radiusbiasshift)
#define initradius		(initrad*radiusbias)	// and decreases by a
#define radiusdec		30						// factor of 1/30 each cycle

// defs for decreasing alpha factor
#define alphabiasshift	10						// alpha starts at 1.0
#define initalpha		(((int) 1)<<alphabiasshift)
int	alphadec;								// biased by 10 bits

// radbias and alpharadbias used for radpower calculation
#define radbiasshift	8
#define radbias			(((int) 1)<<radbiasshift)
#define alpharadbshift	(alphabiasshift+radbiasshift)
#define alpharadbias	(((int) 1)<<alpharadbshift)

// Types and Global Variables
// --------------------------

unsigned char	*thepicture;			// the input image itself
int				lengthcount;			// lengthcount = H*W*3
int				samplefac;				// sampling factor 1..30
typedef int		pixel[dimension + 1];	// BGRc
static pixel	network[netsize];		// the network itself
int				netindex[netsize];		// for network lookup - really netsize
int				bias [netsize];			// bias and freq arrays for learning
int				freq [netsize];
int				radpower[initrad];		// radpower for precomputation


// Initialise network in range (0,0,0) to (255,255,255) and set parameters
// -----------------------------------------------------------------------

void initnet(u8 *thepic, int len, int sample)	
{
	int			i;
	int			*p;

	thepicture	= thepic;
	lengthcount = len;
	samplefac	= sample;

	for (i=0; i<netsize; i++) {
		p = network[i];
		for (int ii=0; ii<dimension; ++ii)
			p[ii] = (i << (netbiasshift+8))/netsize;
		freq[i] = intbias/netsize;	// 1/netsize
		bias[i] = 0;
	}
	return;
}


// Unbias network to give byte values 0..255 and record position i to prepare for sort
// -----------------------------------------------------------------------------------

void unbiasnet()
{
	int i,j;

	for (i=0; i<netsize; i++) {
		for (j=0; j<dimension; j++)
			network[i][j] >>= netbiasshift;
		network[i][dimension] = i;			// record colour no
	}
	return;
}


// Insertion sort of network and building of netindex[0..255] (to do after unbias)
// -------------------------------------------------------------------------------

void inxbuild()
{
	int i,j,smallpos,smallval;
	int *p,*q;
	int previouscol,startpos;

	previouscol = 0;
	startpos = 0;
	for (i=0; i<netsize; i++) {
		p = network[i];
		smallpos = i;
		smallval = p[0];			// index on g
		// find smallest in i..netsize-1
		for (j=i+1; j<netsize; j++) {
			q = network[j];
			if (q[0] < smallval) {	// index on g
				smallpos = j;
				smallval = q[0];	// index on g
			}
		}
		q = network[smallpos];
		// swap p (i) and q (smallpos) entries
		if (i != smallpos) {
			for (int ii=0; ii<dimension; ++ii) {
				j = q[ii];   q[ii] = p[ii];   p[ii] = j;
			}
		}
		// smallval entry is now in position i
		if (smallval != previouscol) {
			netindex[previouscol] = (startpos+i)>>1;
			for (j=previouscol+1; j<smallval; j++) netindex[j] = i;
			previouscol = smallval;
			startpos = i;
		}
	}
	netindex[previouscol] = (startpos+maxnetpos)>>1;
	for (j=previouscol+1; j<netsize; j++) netindex[j] = maxnetpos; // really netsize
	return;
}


// Search for BGR values 0..255 (after net is unbiased) and return colour index
// ----------------------------------------------------------------------------

u8 inxsearch(u8 *s)
{
	int i,j,dist,bestd;
	int *p;
	int best;

	bestd = netsize*dimension + 1;		// biggest possible dist is netsize*3
	best = -1;
	i = netindex[s[0]];	// index on g
	j = i-1;			// start at netindex[g] and work outwards

	while ((i<netsize) || (j>=0)) {
		if (i<netsize) {
			p = network[i];
			i++;
			dist = 0;
			for (int ii=0; ii<dimension; ++ii)
				dist += _abs(int(p[ii]) - int(s[ii]));
			if (dist<bestd) {bestd=dist; best=p[dimension];}
		}
		if (j>=0) {
			j--;
			dist = 0;
			for (int ii=0; ii<dimension; ++ii)
				dist += _abs(p[ii] - s[ii]);
			if (dist<bestd) {bestd=dist; best=p[dimension];}
		}
	}
	return (u8)best;
}


// Search for biased BGR values
// ----------------------------

int contest(int *s)
{
	// finds closest neuron (min dist) and updates freq
	// finds best neuron (min dist-bias) and returns position
	// for frequently chosen neurons, freq[i] is high and bias[i] is negative
	// bias[i] = gamma*((1/netsize)-freq[i])

	int i,dist,biasdist,betafreq;
	int bestpos,bestbiaspos,bestd,bestbiasd;
	int *p,*f, *n;

	bestd = ~(((int) 1)<<31);
	bestbiasd = bestd;
	bestpos = -1;
	bestbiaspos = bestpos;
	p = bias;
	f = freq;

	for (i=0; i<netsize; i++) {
		n = network[i];
		dist = 0;
		for (int ii=0; ii<dimension; ++ii)
			dist += _abs(n[ii] - s[ii]);
		if (dist<bestd) {bestd=dist; bestpos=i;}
		biasdist = dist - ((*p)>>(intbiasshift-netbiasshift));
		if (biasdist<bestbiasd) {bestbiasd=biasdist; bestbiaspos=i;}
		betafreq = (*f >> betashift);
		*f++ -= betafreq;
		*p++ += (betafreq<<gammashift);
	}
	freq[bestpos] += beta;
	bias[bestpos] -= betagamma;
	return(bestbiaspos);
}


// Move neuron i towards biased (b,g,r) by factor alpha
// ----------------------------------------------------

void altersingle(int alpha, int i, int *s)
{
	int *n;

	n = network[i];				// alter hit neuron
	for (int ii=0; ii<dimension; ++ii) {
		*n -= (alpha*(*n - s[ii])) / initalpha;
		n++;
	}
	return;
}


// Move adjacent neurons by precomputed alpha*(1-((i-j)^2/[r]^2)) in radpower[|i-j|]
// ---------------------------------------------------------------------------------

void alterneigh(int rad, int i, int *s)
{
	int j,k,lo,hi,a;
	int *p, *q;

	lo = i-rad;   if (lo<-1) lo=-1;
	hi = i+rad;   if (hi>netsize) hi=netsize;

	j = i+1;
	k = i-1;
	q = radpower;
	while ((j<hi) || (k>lo)) {
		a = (*(++q));
		if (j<hi) {
			p = network[j];
			for (int ii=0; ii<dimension; ++ii) {
				*p -= (a*(*p - s[ii])) / alpharadbias;
				p++;
			}
			j++;
		}
		if (k>lo) {
			p = network[k];
			for (int ii=0; ii<dimension; ++ii) {
				*p -= (a*(*p - s[ii])) / alpharadbias;
				p++;
			}
			k--;
		}
	}
	return;
}


// Main Learning Loop
// ------------------

void learn()
{
	int i,j;
	int radius,rad,alpha,step,delta,samplepixels;
	u8 *p;
	u8 *lim;

	alphadec = 30 + ((samplefac-1)/dimension);
	p = thepicture;
	lim = thepicture + lengthcount;
	samplepixels = lengthcount/(dimension*samplefac);
	delta = samplepixels/ncycles;
	alpha = initalpha;
	radius = initradius;

	rad = radius >> radiusbiasshift;
	if (rad <= 1) rad = 0;
	for (i=0; i<rad; i++) 
		radpower[i] = alpha*(((rad*rad - i*i)*radbias)/(rad*rad));

	// beginning 1D learning: initial radius=rad

	bool bOk = false;
	for (int ii=0; ii<dimension; ++ii) {
		if ((lengthcount % prime[ii]) != 0) {
			step = dimension*prime[ii];
			bOk = true;
			break;
		}
	}
	if (!bOk)
		step = dimension*prime[dimension];

	int s[dimension];
	i = 0;
	while (i < samplepixels) {
		for (int ii=0; ii<dimension; ++ii)
			s[ii] = p[ii] << netbiasshift;
		j = contest(s);

		altersingle(alpha,j,s);
		if (rad) alterneigh(rad,j,s);   // alter neighbours

		p += step;
		if (p >= lim) p -= lengthcount;

		i++;
		if (i%delta == 0) {	
			alpha -= alpha / alphadec;
			radius -= radius / radiusdec;
			rad = radius >> radiusbiasshift;
			if (rad <= 1) rad = 0;
			for (j=0; j<rad; j++) 
				radpower[j] = alpha*(((rad*rad - j*j)*radbias)/(rad*rad));
		}
	}
	// finished 1D learning: final alpha=alpha/initalpha;
	return;
}

extern BYTE	compress(float c, int max_value);

void xrPalettizeCovers()
{
	u32				N = g_nodes.size();
	u8				*data = (u8*)xr_malloc(N*dimension);

	// convert cover values and init clusters
	Nodes::const_iterator				I = g_nodes.begin(), B = I;
	Nodes::const_iterator				E = g_nodes.end();
	for ( ; I != E; ++I)
		for (int i=0; i<dimension; ++i)
			data[dimension*(I - B) + i] = compress((*I).cover[i],255);

	initnet		(data,dimension*N,30);
	learn		();
	unbiasnet	();

	for (int i=0; i<(int)N; ++i)
		g_nodes[i].cover_index = inxsearch((u8*)(data + i));

	g_covers_palette.resize(netsize);
	{
//		Msg		("Palette");
		xr_vector<SCover>::iterator					I = g_covers_palette.begin(), B = I;
		xr_vector<SCover>::iterator					E = g_covers_palette.end();
		for ( ; I != E; ++I) {
			for (int i=0; i<dimension; ++i)
				(*I).cover[i]	= network[I - B][i];
//			Msg	(
//				"[%.3f][%.3f][%.3f][%.3f]",
//				float((*I).cover[0])/255.f,
//				float((*I).cover[1])/255.f,
//				float((*I).cover[2])/255.f,
//				float((*I).cover[3])/255.f
//			);
		}
	}

	float		l_sum_sqr = 0.f;
	float		l_sum = 0.f;
	for (int i=0; i<(int)N; ++i) {
		l_sum	+= 
			_abs(g_nodes[i].cover[0] - float(g_covers_palette[g_nodes[i].cover_index].cover[0])/255.f) + 
			_abs(g_nodes[i].cover[1] - float(g_covers_palette[g_nodes[i].cover_index].cover[1])/255.f) + 
			_abs(g_nodes[i].cover[2] - float(g_covers_palette[g_nodes[i].cover_index].cover[2])/255.f) + 
			_abs(g_nodes[i].cover[3] - float(g_covers_palette[g_nodes[i].cover_index].cover[3])/255.f);
		l_sum_sqr += 
			_sqr(g_nodes[i].cover[0] - float(g_covers_palette[g_nodes[i].cover_index].cover[0])/255.f) + 
			_sqr(g_nodes[i].cover[1] - float(g_covers_palette[g_nodes[i].cover_index].cover[1])/255.f) + 
			_sqr(g_nodes[i].cover[2] - float(g_covers_palette[g_nodes[i].cover_index].cover[2])/255.f) + 
			_sqr(g_nodes[i].cover[3] - float(g_covers_palette[g_nodes[i].cover_index].cover[3])/255.f);
//		Msg		(
//			"[%.3f][%.3f][%.3f][%.3f] -> [%.3f][%.3f][%.3f][%.3f] : %7.3f",
//			g_nodes[i].cover[0],
//			g_nodes[i].cover[1],
//			g_nodes[i].cover[2],
//			g_nodes[i].cover[3],
//			float(g_covers_palette[g_nodes[i].cover_index].cover[0])/255.f,
//			float(g_covers_palette[g_nodes[i].cover_index].cover[1])/255.f,
//			float(g_covers_palette[g_nodes[i].cover_index].cover[2])/255.f,
//			float(g_covers_palette[g_nodes[i].cover_index].cover[3])/255.f,
//			_sqrt(
//				_sqr(g_nodes[i].cover[0] - float(g_covers_palette[g_nodes[i].cover_index].cover[0])/255.f) + 
//				_sqr(g_nodes[i].cover[1] - float(g_covers_palette[g_nodes[i].cover_index].cover[1])/255.f) + 
//				_sqr(g_nodes[i].cover[2] - float(g_covers_palette[g_nodes[i].cover_index].cover[2])/255.f) + 
//				_sqr(g_nodes[i].cover[3] - float(g_covers_palette[g_nodes[i].cover_index].cover[3])/255.f)
//			)
//		);
	}
	Msg				("Total absoulte error : %f (%f)",l_sum,l_sum/float(N));
	Msg				("Total squared  error : %f (%f)",l_sum_sqr,l_sum_sqr/float(N));

	xr_free			(data);
}