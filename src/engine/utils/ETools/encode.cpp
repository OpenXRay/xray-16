/* OggEnc
 **
 ** This program is distributed under the GNU General Public License, version 2.
 ** A copy of this license is included with this source.
 **
 ** Copyright 2000-2002, Michael Smith <msmith@xiph.org>
 **
 ** Portions from Vorbize, (c) Kenneth Arnold <kcarnold-xiph@arnoldnet.net>
 ** and libvorbis examples, (c) Monty <monty@xiph.org>
 **/
#include "stdafx.h"


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "platform.h"
#include <vorbis/vorbisenc.h>
#include "encode.h"
//#include "i18n.h"

#define READSIZE 1024
#define _(a) a


int oe_write_page(ogg_page *page, FILE *fp);

#define SETD(toset) \
    do {\
        if(sscanf(opts[i].val, "%lf", &dval) != 1)\
            fprintf(stderr, "For option %s, couldn't read value %s as double\n",\
                    opts[i].arg, opts[i].val);\
        else\
            toset = dval;\
    } while(0)

#define SETL(toset) \
    do {\
        if(sscanf(opts[i].val, "%ld", &lval) != 1)\
            fprintf(stderr, "For option %s, couldn't read value %s as integer\n",\
                    opts[i].arg, opts[i].val);\
        else\
            toset = lval;\
    } while(0)

static void set_advanced_encoder_options(adv_opt *opts, int count, 
        vorbis_info *vi)
{
    int manage = 0;
    struct ovectl_ratemanage2_arg ai;
    int i;
    double dval;
    long lval;

    vorbis_encode_ctl(vi, OV_ECTL_RATEMANAGE2_GET, &ai);

    for(i=0; i < count; i++) {
        fprintf(stderr, _("Setting advanced encoder option \"%s\" to %s\n"),
                opts[i].arg, opts[i].val);

        if(!xr_strcmp(opts[i].arg, "bitrate_average_damping")) {
            SETD(ai.bitrate_average_damping);
            manage = 1;
        }
        else if(!xr_strcmp(opts[i].arg, "bitrate_average")) {
            SETL(ai.bitrate_average_kbps);
            manage = 1;
        }
        else if(!xr_strcmp(opts[i].arg, "bit_reservoir_bias")) {
            SETD(ai.bitrate_limit_reservoir_bias);
            manage = 1;
        }
        else if(!xr_strcmp(opts[i].arg, "bit_reservoir_bits")) {
            SETL(ai.bitrate_limit_reservoir_bits);
            manage = 1;
        }
        else if(!xr_strcmp(opts[i].arg, "bitrate_hard_min")) {
            SETL(ai.bitrate_limit_min_kbps);
            manage = 1;
        }
        else if(!xr_strcmp(opts[i].arg, "bitrate_hard_max")) {
            SETL(ai.bitrate_limit_max_kbps);
            manage = 1;
        }
        else if(!xr_strcmp(opts[i].arg, "impulse_noisetune")) {
            double val;
            SETD(val);
            vorbis_encode_ctl(vi, OV_ECTL_IBLOCK_SET, &val);
        }
        else if(!xr_strcmp(opts[i].arg, "lowpass_frequency")) {
            double _prev, _new;
            SETD(_new);
            vorbis_encode_ctl(vi, OV_ECTL_LOWPASS_GET, &_prev);
            vorbis_encode_ctl(vi, OV_ECTL_LOWPASS_SET, &_new);
            fprintf(stderr, _("Changed lowpass frequency from %f kHz to %f kHz\n"), _prev, _new);
        }
        else {
            fprintf(stderr, _("Unrecognised advanced option \"%s\"\n"), 
                    opts[i].arg);
        }
    }

    if(manage) {
        if(vorbis_encode_ctl(vi, OV_ECTL_RATEMANAGE2_SET, &ai)) {
            fprintf(stderr, "Failed to set advanced rate management parameters\n");
        }
    }
}

int oe_encode(oe_enc_opt *opt)
{

	ogg_stream_state os;
	ogg_page 		 og;
	ogg_packet 		 op;

	vorbis_dsp_state vd;
	vorbis_block     vb;
	vorbis_info      vi;
	long bitrate;

	long samplesdone=0;
    int eos;
	long bytes_written = 0, packetsdone=0;
	double time_elapsed;
	int ret=0;
	TIMER *timer;

    if(opt->channels > 255) {
        fprintf(stderr, _("255 channels should be enough for anyone. (Sorry, vorbis doesn't support more)\n"));
        return 1;
    }

    /* get start time. */
    timer = timer_start();

    if(!opt->managed && (opt->min_bitrate>=0 || opt->max_bitrate>=0)){
      fprintf(stderr, _("Requesting a minimum or maximum bitrate requires --managed\n"));
      return 1;
    }

    /* if we had no quality or bitrate spec at all from the user, use
       the default quality with no management --Monty 20020711 */
    if(opt->bitrate < 0 && opt->min_bitrate < 0 && opt->max_bitrate < 0){
      opt->quality_set=1;
    }

    opt->start_encode(opt->infilename, opt->filename, opt->bitrate, opt->quality, 
		      opt->quality_set, opt->managed, opt->min_bitrate, opt->max_bitrate);
    
    /* Have vorbisenc choose a mode for us */
    vorbis_info_init(&vi);
    
    if(opt->quality_set > 0){
        if(vorbis_encode_setup_vbr(&vi, opt->channels, opt->rate, opt->quality)){
	        fprintf(stderr, _("Mode initialisation failed: invalid parameters for quality\n"));
	        vorbis_info_clear(&vi);
	        return 1;
        }

        /* do we have optional hard bitrate restrictions? */
        if(opt->max_bitrate > 0 || opt->min_bitrate > 0){
            struct ovectl_ratemanage2_arg ai;
	        vorbis_encode_ctl(&vi, OV_ECTL_RATEMANAGE2_GET, &ai);

            /* libvorbis 1.1 (and current svn) doesn't actually fill this in,
               which looks like a bug. It'll then reject it when we call the
               SET version below. So, fill it in with the values that libvorbis
               would have used to fill in this structure if we were using the
               bitrate-oriented setup functions. Unfortunately, some of those
               values are dependent on the bitrate, and libvorbis has no way to
               get a nominal bitrate from a quality value. Well, except by doing
               a full setup... So, we do that. 
               Also, note that this won't work correctly unless you have a very
               recent (2005/03/04 or later) version of libvorbis from svn).
             */

            {
                vorbis_info vi2;
                vorbis_info_init(&vi2);
                vorbis_encode_setup_vbr(&vi2, opt->channels, opt->rate, opt->quality);
                vorbis_encode_setup_init(&vi2);
                bitrate = vi2.bitrate_nominal;
                vorbis_info_clear(&vi2);
            }

            ai.bitrate_average_kbps = bitrate/1000;
            ai.bitrate_average_damping = 1.5;
            ai.bitrate_limit_reservoir_bits = bitrate * 2;
            ai.bitrate_limit_reservoir_bias = .1;
	
            /* And now the ones we actually wanted to set */
	        ai.bitrate_limit_min_kbps=opt->min_bitrate;
	        ai.bitrate_limit_max_kbps=opt->max_bitrate;
	        ai.management_active=1;

	        if(vorbis_encode_ctl(&vi, OV_ECTL_RATEMANAGE2_SET, &ai) == 0)
                fprintf(stderr, _("Set optional hard quality restrictions\n"));
            else {
                fprintf(stderr, _("Failed to set bitrate min/max in quality mode\n"));
                vorbis_info_clear(&vi);
                return 1;
            }
        }


    }else {
        if(vorbis_encode_setup_managed(&vi, opt->channels, opt->rate, 
				     opt->max_bitrate>0?opt->max_bitrate*1000:-1,
				     opt->bitrate*1000, 
				     opt->min_bitrate>0?opt->min_bitrate*1000:-1)){
	        fprintf(stderr, _("Mode initialisation failed: invalid parameters for bitrate\n"));
	        vorbis_info_clear(&vi);
	        return 1;
        }
    }
    
    if(opt->managed && opt->bitrate < 0)
    {
      struct ovectl_ratemanage2_arg ai;
      vorbis_encode_ctl(&vi, OV_ECTL_RATEMANAGE2_GET, &ai);
      ai.bitrate_average_kbps=-1;
      vorbis_encode_ctl(&vi, OV_ECTL_RATEMANAGE2_SET, &ai);
    }
    else if(!opt->managed)
    {
        /* Turn off management entirely (if it was turned on). */
        vorbis_encode_ctl(&vi, OV_ECTL_RATEMANAGE2_SET, NULL);
    }
    
    set_advanced_encoder_options(opt->advopt, opt->advopt_count, &vi);
    
    vorbis_encode_setup_init(&vi);


	/* Now, set up the analysis engine, stream encoder, and other
	   preparation before the encoding begins.
	 */

	vorbis_analysis_init(&vd,&vi);
	vorbis_block_init(&vd,&vb);

	ogg_stream_init(&os, opt->serialno);

	/* Now, build the three header packets and send through to the stream 
	   output stage (but defer actual file output until the main encode loop) */

	{
		ogg_packet header_main;
		ogg_packet header_comments;
		ogg_packet header_codebooks;
		int result;

		/* Build the packets */
		vorbis_analysis_headerout(&vd,opt->comments,
				&header_main,&header_comments,&header_codebooks);

		/* And stream them out */
		ogg_stream_packetin(&os,&header_main);
		ogg_stream_packetin(&os,&header_comments);
		ogg_stream_packetin(&os,&header_codebooks);

		while((result = ogg_stream_flush(&os, &og)) != 0)
		{
			if(!result) break;
			ret = oe_write_page(&og, opt->out);
			if(ret != og.header_len + og.body_len)
			{
				opt->error(_("Failed writing header to output stream\n"));
				ret = 1;
				goto cleanup; /* Bail and try to clean up stuff */
			}
		}
	}

	eos = 0;

	/* Main encode loop - continue until end of file */
	while(!eos)
	{
		float **buffer = vorbis_analysis_buffer(&vd, READSIZE);
		long samples_read = opt->read_samples(opt->readdata, 
				buffer, READSIZE);

		if(samples_read ==0)
			/* Tell the library that we wrote 0 bytes - signalling the end */
			vorbis_analysis_wrote(&vd,0);
		else
		{
			samplesdone += samples_read;

			/* Call progress update every 40 pages */
			if(packetsdone>=40)
			{
				double time;

				packetsdone = 0;
				time = timer_time(timer);

				opt->progress_update(opt->filename, opt->total_samples_per_channel, 
						samplesdone, time);
			}

			/* Tell the library how many samples (per channel) we wrote 
			   into the supplied buffer */
			vorbis_analysis_wrote(&vd, samples_read);
		}

		/* While we can get enough data from the library to analyse, one
		   block at a time... */
		while(vorbis_analysis_blockout(&vd,&vb)==1)
		{

			/* Do the main analysis, creating a packet */
			vorbis_analysis(&vb, NULL);
			vorbis_bitrate_addblock(&vb);

			while(vorbis_bitrate_flushpacket(&vd, &op)) 
			{
				/* Add packet to bitstream */
				ogg_stream_packetin(&os,&op);
				packetsdone++;

				/* If we've gone over a page boundary, we can do actual output,
				   so do so (for however many pages are available) */

				while(!eos)
				{
					int result = ogg_stream_pageout(&os,&og);
					if(!result) break;

					ret = oe_write_page(&og, opt->out);
					if(ret != og.header_len + og.body_len)
					{
						opt->error(_("Failed writing data to output stream\n"));
						ret = 1;
						goto cleanup; /* Bail */
					}
					else
						bytes_written += ret; 
	
					if(ogg_page_eos(&og))
						eos = 1;
				}
			}
		}
	}

	ret = 0; /* Success, set return value to 0 since other things reuse it
			  * for nefarious purposes. */

	/* Cleanup time */
cleanup:

	ogg_stream_clear(&os);

	vorbis_block_clear(&vb);
	vorbis_dsp_clear(&vd);
	vorbis_info_clear(&vi);

	time_elapsed = timer_time(timer);
	opt->end_encode(opt->filename, time_elapsed, opt->rate, samplesdone, bytes_written);

	timer_clear(timer);

	return ret;
}

void update_statistics_full(char *fn, long total, long done, double time)
{
	static char *spinner="|/-\\";
	static int spinpoint = 0;
	double remain_time;
	int minutes=0,seconds=0;
	
	remain_time = time/((double)done/(double)total) - time;
	minutes = ((int)remain_time)/60;
	seconds = (int)(remain_time - (double)((int)remain_time/60)*60);

	fprintf(stderr, "\r");
	fprintf(stderr, _("\t[%5.1f%%] [%2dm%.2ds remaining] %c "), 
			done*100.0/total, minutes, seconds, spinner[spinpoint++%4]);
}

void update_statistics_notime(char *fn, long total, long done, double time)
{
	static char *spinner="|/-\\";
	static int spinpoint =0;
	
	fprintf(stderr, "\r");
	fprintf(stderr, _("\tEncoding [%2dm%.2ds so far] %c "), 
            ((int)time)/60, (int)(time - (double)((int)time/60)*60),
			spinner[spinpoint++%4]);
}

int oe_write_page(ogg_page *page, FILE *fp)
{
	int written;
	written = (int)fwrite(page->header,1,page->header_len, fp);
	written += (int)fwrite(page->body,1,page->body_len, fp);

	return written;
}

void final_statistics(char *fn, double time, int rate, long samples, long bytes)
{
	double speed_ratio;
	if(fn)
		fprintf(stderr, _("\n\nDone encoding file \"%s\"\n"), fn);
	else
		fprintf(stderr, _("\n\nDone encoding.\n"));

	speed_ratio = (double)samples / (double)rate / time;
	
	fprintf(stderr, _("\n\tFile length:  %dm %04.1fs\n"),
			(int)(samples/rate/60),
			samples/rate - 
			samples/rate/60*60);
	fprintf(stderr, _("\tElapsed time: %dm %04.1fs\n"),
			(int)(time/60),
			time - iFloor((float)time/60)*60);
	fprintf(stderr, _("\tRate:         %.4f\n"), speed_ratio);
	fprintf(stderr, _("\tAverage bitrate: %.1f kb/s\n\n"), 
		8./1000.*((double)bytes/((double)samples/(double)rate)));
}

void final_statistics_null(char *fn, double time, int rate, long samples, 
		long bytes)
{
	/* Don't do anything, this is just a placeholder function for quiet mode */
}

void update_statistics_null(char *fn, long total, long done, double time)
{
	/* So is this */
}

void encode_error(char *errmsg)
{
	fprintf(stderr, "\n%s\n", errmsg);
}

static void print_brconstraints(int min, int max)
{
    if(min > 0 && max > 0)
        fprintf(stderr, "(min %d kbps, max %d kbps)", min,max);
    else if(min > 0)
        fprintf(stderr, "(min %d kbps, no max)", min);
    else if(max > 0)
        fprintf(stderr, "(no min, max %d kbps)", max);
    else
        fprintf(stderr, "(no min or max)");
}

void start_encode_full(char *fn, char *outfn, int bitrate, float quality, int qset,
        int managed, int min, int max)
{
  if(bitrate>0){
    if(managed>0){
      fprintf(stderr, _("Encoding %s%s%s to \n         "
			"%s%s%s \nat average bitrate %d kbps "),
	      fn?"\"":"", fn?fn:_("standard input"), fn?"\"":"",
	      outfn?"\"":"", outfn?outfn:_("standard output"), outfn?"\"":"",
	      bitrate);
      print_brconstraints(min,max);
      fprintf(stderr, ", \nusing full bitrate management engine\n");
    } else {
      fprintf(stderr, _("Encoding %s%s%s to \n         %s%s%s \nat approximate bitrate %d kbps (VBR encoding enabled)\n"),
	      fn?"\"":"", fn?fn:_("standard input"), fn?"\"":"",
	    outfn?"\"":"", outfn?outfn:_("standard output"), outfn?"\"":"",
	      bitrate);
    }
  }else{
    if(qset>0){
      if(managed>0){
	fprintf(stderr, _("Encoding %s%s%s to \n         %s%s%s \nat quality level %2.2f using constrained VBR "),
		fn?"\"":"", fn?fn:_("standard input"), fn?"\"":"",
		outfn?"\"":"", outfn?outfn:_("standard output"), outfn?"\"":"",
		quality * 10);
	print_brconstraints(min,max);
	fprintf(stderr, "\n");
      }else{
        fprintf(stderr, _("Encoding %s%s%s to \n         %s%s%s \nat quality %2.2f\n"),
			    fn?"\"":"", fn?fn:_("standard input"), fn?"\"":"",
                outfn?"\"":"", outfn?outfn:_("standard output"), outfn?"\"":"",
                quality * 10);
      }
    }else{
      fprintf(stderr, _("Encoding %s%s%s to \n         %s%s%s \nusing bitrate management "),
	      fn?"\"":"", fn?fn:_("standard input"), fn?"\"":"",
	      outfn?"\"":"", outfn?outfn:_("standard output"), outfn?"\"":"");
      print_brconstraints(min,max);
      fprintf(stderr, "\n");
    }
  }
}

void start_encode_null(char *fn, char *outfn, int bitrate, float quality, int qset,
        int managed, int min, int max)
{
}


