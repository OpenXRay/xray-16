function normal		(shader, t_base, t_second, t_detail)
	shader	: begin	("stub_default","stub_default")
			: zb	(true,false)
			: blend	(true,blend.srcalpha,blend.invsrcalpha)
--	TODO: DX10: implement aref for this shader
			: aref 		(true,0)

--	TODO: DX10: Clamp
	shader:sampler	("s_base")      :texture	(t_base)
end