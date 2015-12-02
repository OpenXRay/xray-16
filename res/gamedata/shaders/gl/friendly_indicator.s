function normal		(shader, t_base, t_second, t_detail)
	shader	:begin	("stub_default","stub_default")
			:blend		(true,blend.srcalpha,blend.invsrcalpha)
			:zb			(true,false)
	shader:sampler	("s_base")	: texture(t_base)	: clamp() : f_linear ()
end