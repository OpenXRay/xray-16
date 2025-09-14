function normal(shader, t_base, t_second, t_detail)
  shader:begin("effects_wallmark","stub_default_ma")
		: blend		(true,blend.destcolor,blend.srccolor)
		: zb 		(true,false)
	shader:sampler	("s_base")      :texture	(t_base) : clamp() : f_linear ()
	shader: dx10color_write_enable( true, true, true, false)
end