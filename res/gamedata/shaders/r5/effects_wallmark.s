function normal(shader, t_base, t_second, t_detail)
  shader:begin("effects_wallmark","stub_default_ma")
		: blend		(true,blend.destcolor,blend.srccolor)
		: zb 		(true,false)
	shader:dx10texture	("s_base", t_base)
	shader:dx10sampler	("smp_rtlinear")
	shader: dx10color_write_enable( true, true, true, false)
end