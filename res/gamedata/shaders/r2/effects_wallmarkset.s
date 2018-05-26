function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("wmark",	"simple")
			: sorting	(1, false)
			: aref 		(false,0)
			: zb 		(true,true)
			: fog		(false)
			: wmark		(true)
	shader:sampler	("s_base")      :texture	(t_base)
end
