function l_special	(shader, t_base, t_second, t_detail)
	shader:begin	("particle",	"particle_distort")
			: sorting	(3, false)
			: blend		(true,blend.srcalpha,blend.invsrcalpha)
			: zb 		(true,false)
			: fog		(false)
			: distort 	(true)
--	shader:sampler	("s_base")      :texture	(t_base)
--	shader:sampler	("s_distort")   :texture	(t_base)	-- "pfx\\pfx_distortion"
--	shader:sampler  ("s_position")  :texture  ("$user$position")

	shader:dx10texture	("s_base"		,t_base)
	shader:dx10texture	("s_distort"   ,t_base)	-- "pfx\\pfx_distortion"
	shader:dx10texture  ("s_position"  ,"$user$position")

	shader:dx10sampler	("smp_base")
	shader:dx10sampler	("smp_linear")
	shader:dx10sampler	("smp_nofilter")
end
