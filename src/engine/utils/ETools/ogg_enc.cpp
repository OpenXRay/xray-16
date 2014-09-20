#include "stdafx.h"
#include "ETools.h"

#include "encode.h"
#include "platform.h"
#include "audio.h"

#pragma warning(disable:4995)

namespace ETOOLS{
	ETOOLS_API int __stdcall ogg_enc(const char* in_fn, const char* out_fn, float quality, void* comment, int comment_size)
	{
		/* Default values */
		oe_options opt = {NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 
			0, NULL, 0, NULL, 0, NULL, 0, 1, 0, 0,16,44100,2, 0, NULL,
			DEFAULT_NAMEFMT_REMOVE, DEFAULT_NAMEFMT_REPLACE, 
			NULL, 0, -1,-1,-1,-.3f,-1,0, 0,0.f, 0}; 

		oe_enc_opt      enc_opts;
		vorbis_comment  vc;
		FILE *in, *out	= NULL;
//		int foundformat = 0;
//		int closeout	= 0, closein = 0;
		input_format	*format;
		int res			= 1;

		/* Set various encoding defaults */
		enc_opts.serialno			= 0;
		enc_opts.progress_update	= update_statistics_full;
		enc_opts.start_encode		= start_encode_full;
		enc_opts.end_encode			= final_statistics;
		enc_opts.error				= encode_error;
		enc_opts.comments			= &vc;

		in							= fopen(in_fn, "rb");

		if(in == NULL)				return 0;

		format						= open_audio_file(in, &enc_opts);
		if(!format){
			fclose					(in);
			return 0;
		};

		out							= fopen(out_fn, "wb");
		if(out == NULL){
			fclose					(out);
			return 0;
		}	

		/* OK, let's build the vorbis_comments structure */
		memset						(&vc,0,sizeof(vc));

		vc.user_comments			= (char**)_ogg_malloc	(sizeof(*vc.user_comments));
		vc.comment_lengths			= (int*)_ogg_malloc	(sizeof(*vc.comment_lengths));
		vc.comments					= 1;
		vc.user_comments[0]			= (char*)_ogg_malloc	(comment_size);
		memcpy						(vc.user_comments[0],comment,comment_size);
		vc.comment_lengths[0]		= comment_size;

		/* Now, set the rest of the options */
		enc_opts.out				= out;
		enc_opts.comments			= &vc;
		enc_opts.filename			= (char*)out_fn;
		enc_opts.infilename			= (char*)in_fn;
		enc_opts.quality			= quality;
		enc_opts.quality_set		= opt.quality_set;
		enc_opts.managed			= opt.managed;
		enc_opts.bitrate			= opt.nominal_bitrate; 
		enc_opts.min_bitrate		= opt.min_bitrate;
		enc_opts.max_bitrate		= opt.max_bitrate;
		enc_opts.advopt				= opt.advopt;
		enc_opts.advopt_count		= opt.advopt_count;

		// encode
		if(oe_encode(&enc_opts))
			res						= 0;

		// clear comment
		if(vc.user_comments[0])		_ogg_free(vc.user_comments[0]);
		if(vc.user_comments)		_ogg_free(vc.user_comments);
		if(vc.comment_lengths)		_ogg_free(vc.comment_lengths);
		if(vc.vendor)				_ogg_free(vc.vendor);

		// close files
		fclose						(in);
		fclose						(out);

		return res;
	}
}

