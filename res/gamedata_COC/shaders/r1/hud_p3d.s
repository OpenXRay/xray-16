function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("hud3d","hud3d")
			: fog		(false)
			: zb 		(true,true)
			: blend		(true,blend.srcalpha,blend.invsrcalpha)
 	shader:sampler	("s_base")       :texture  (t_base)
end
