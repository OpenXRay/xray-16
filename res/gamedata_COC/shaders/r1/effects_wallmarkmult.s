function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("wmark",	"wmarkmult")
			: sorting	(2, false)
			: blend		(true,blend.destcolor,blend.srccolor)
			: aref 		(false,0)
			: zb 		(true,true)
			: fog		(true)
			: wmark		(true)
	shader:sampler	("s_base")      :texture	(t_base)
end


function l_spot    (shader, t_base, t_second, t_detail)
  r1_lspot	(shader, t_base, "wmark_spot")
end

function l_point  (shader, t_base, t_second, t_detail)
  r1_lpoint	(shader, t_base, "wmark_point")
end