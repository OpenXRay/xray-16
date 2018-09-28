function l_special	(shader, t_base, t_second, t_detail)
	shader:begin	("particle",	"particle_distort")
			: sorting	(3, false)
			: blend		(true,blend.srcalpha,blend.invsrcalpha)
			: zb 		(true,false)
			: fog		(false)
			: distort 	(true)
	shader:sampler	("s_base")      :texture	(t_base)
	shader:sampler	("s_distort")   :texture	(t_base)	-- "pfx\\pfx_distortion"
end
