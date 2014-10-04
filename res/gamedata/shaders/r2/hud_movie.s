function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("null","yuv2rgb")
			: fog		(false)
			: zb 		(false,false)
			: blend		(true,blend.srcalpha,blend.invsrcalpha)
 	shader:sampler	("s_base")       :texture  (t_base)
end
