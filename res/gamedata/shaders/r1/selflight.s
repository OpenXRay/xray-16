function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("simple","simple")
			: fog	(true)
	shader:sampler	("s_base")      :texture	(t_base)
end

function l_spot		(shader, t_base, t_second, t_detail)
	r1_lspot 	(shader, t_base, "simple_spot")
end

function l_point	(shader, t_base, t_second, t_detail)
	r1_lpoint 	(shader, t_base, "simple_point")
end

function l_special	(shader, t_base, t_second, t_detail)
	shader:begin	("simple","simple")
	  		: fog		(false)
	shader:sampler	("s_base")      :texture	(t_base)
end
