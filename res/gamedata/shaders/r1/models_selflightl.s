function normal		(shader, t_base, t_second, t_detail)
	shader	:begin		("model_def_lqs","simple")
			:fog		(true)
	shader	:sampler	("s_base")      :texture	(t_base)
end

function l_spot		(shader, t_base, t_second, t_detail)
	r1_lspot 	(shader, t_base, "model_def_spot")
end

function l_point	(shader, t_base, t_second, t_detail)
	r1_lpoint 	(shader, t_base, "model_def_point")
end
