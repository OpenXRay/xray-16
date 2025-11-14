function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("hud3d","hud3d")
			: fog		(false)
			: zb 		(true,false)
			: blend		(true,blend.srcalpha,blend.invsrcalpha)
			: dx10color_write_enable( true, true, true, false)
	shader:dx10texture	("s_base", t_base)
	shader:dx10sampler	("smp_base")
end
