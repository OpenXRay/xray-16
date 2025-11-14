function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("stub_default","stub_default")
--	shader:begin	("stub_default","test","stub_default")
			: zb 		(true,false)
			: blend		(true,blend.one,blend.one)
			: sorting	(2, true)
  
	shader:dx10texture	("s_base", "water\\water_ryaska1")
	shader:dx10sampler	("smp_base")
end