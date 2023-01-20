function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("wmark_blend",	"vert")
			: sorting	(2, false)
			: blend		(true,blend.srcalpha,blend.invsrcalpha)
			: aref 		(true,0)
			: zb 		(true,false)
			: fog		(true)
	shader:sampler	("s_base")      :texture	(t_base)
end

function l_spot    (shader, t_base, t_second, t_detail)
	r1_lspot	(shader, t_base, "wmark_spot")
	shader:sorting(2, false)	
end

function l_point  (shader, t_base, t_second, t_detail)
	r1_lpoint	(shader, t_base, "wmark_point")
	shader:sorting(2, false)
end
