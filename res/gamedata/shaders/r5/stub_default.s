function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("stub_default","stub_default")

--	Decouple sampler and texture
--	shader:sampler	("s_base")	: texture(t_base)	: clamp() : f_linear ()
--	TODO: DX10: move stub_default to smp_rtlinear
	shader:dx10texture	("s_base", t_base)
	shader:dx10sampler	("smp_base")
end