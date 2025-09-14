function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("accum_sun", "accum_volumetric_sun_msaa6")
			: fog		(false)
			: zb 		(false,false)
			: blend		(true,blend.one,blend.one)
			: sorting	(2, false)
--	TODO: DX10: Implement for near and far phase.
--	TODO: DX10: Setup samplers.
--	shader:sampler	("s_smap")      :texture ("null")
--	shader:sampler	("s_position")	:texture ("$user$position")
--	shader:sampler	("jitter0")	:texture ("$user$jitter_0") : f_none ()

	shader:dx10texture	("s_smap", "null")
	shader:dx10texture	("s_position", "$user$position")
	shader:dx10texture	("jitter0", "$user$jitter_0")

	shader:dx10sampler	("smp_nofilter")
	shader:dx10sampler	("smp_jitter")
	shader:dx10sampler	("smp_smap")
end