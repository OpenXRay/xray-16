function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("accum_volumetric", "accum_volumetric")
			: fog		(false)
			: zb 		(true,false)
			: blend		(true,blend.one,blend.one)
			: sorting	(2, false)

	shader:dx10texture	("s_lmap", t_base)	
	shader:dx10texture	("s_smap", "null")
	shader:dx10texture	("s_position",	"$user$position")

	shader:dx10sampler	("smp_rtlinear")
	shader:dx10sampler	("smp_smap")
	shader:dx10sampler	("smp_jitter")
end