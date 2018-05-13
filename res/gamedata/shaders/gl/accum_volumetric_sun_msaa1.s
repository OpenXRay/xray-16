function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("accum_sun", "accum_volumetric_sun_msaa1")
			: fog		(false)
			: zb 		(false,false)
			: blend		(true,blend.one,blend.one)
			: sorting	(2, false)
--	TODO: DX10: Implement for near and far phase.
	shader:sampler	("s_dmap")      :texture ("$user$smap_depth")
	shader:sampler	("s_smap")      :texture ("$user$smap_depth") : comp_less ()
	shader:sampler	("s_position")	:texture ("$user$position")
	shader:sampler	("jitter0")	:texture ("$user$jitter_0") : f_none ()
end