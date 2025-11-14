function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("stub_notransform_t","distort")
			: fog	(false)
			: zb 	(false,false)
--	TODO: DX10: Implement samplers
--	shader:sampler	("s_base")	:texture("$user$generic0")	: clamp() : f_linear ()
--	shader:sampler	("s_distort")  	:texture("$user$generic1")	: clamp() : f_linear ()

	shader:dx10texture	("s_base", "$user$generic0")
	shader:dx10texture	("s_distort", "$user$generic1")

	shader:dx10sampler	("smp_rtlinear");
end
