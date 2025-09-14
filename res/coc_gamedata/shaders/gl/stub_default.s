function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("stub_default","stub_default")
	shader:sampler	("s_base")	: texture(t_base)	: clamp() : f_linear ()
end