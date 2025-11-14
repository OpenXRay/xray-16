function normal		(shader, t_base, t_second, t_detail)
	shader:begin		("hud_crosshair","simple_color")
			: fog		(false)
			: zb 		(false,false)
			: blend		(true,blend.srcalpha,blend.invsrcalpha)
end
