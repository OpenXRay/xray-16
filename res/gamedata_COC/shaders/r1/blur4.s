function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("null","avg4")
			: fog	(false)
			: zb 	(false,false)
	shader:sampler	("s_base0")	:texture(t_base): clamp() : f_linear ()
	shader:sampler	("s_base1")    	:texture(t_base): clamp() : f_linear ()
	shader:sampler	("s_base2")	:texture(t_base): clamp() : f_linear ()
	shader:sampler	("s_base3")    	:texture(t_base): clamp() : f_linear ()
end
