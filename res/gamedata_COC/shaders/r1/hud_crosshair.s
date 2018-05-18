function normal		(shader, t_base, t_second, t_detail)
	shader:begin		("null","simple_color")
			: fog		(false)
			: zb 		(false,false)
			: blend		(true,blend.srcalpha,blend.invsrcalpha)
end
