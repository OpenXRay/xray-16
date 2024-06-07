function normal         (shader, t_base, t_second, t_detail)
        shader:begin    ("clouds","clouds")
                        : fog               (false)
--	TODO: DX10: Check if this is ok.
--                        : zb                (true,false)
						: zb                (false,false)
                        : sorting        	(3, true)
                        : blend             (true, blend.srcalpha,blend.invsrcalpha)

--	TODO: DX10: implement sampler setup
--        shader:sampler        ("s_clouds0")   :texture        ("null")        : wrap() : f_anisotropic()
--        shader:sampler        ("s_clouds1")   :texture        ("null")        : wrap() : f_anisotropic()
--        shader:sampler        ("s_tonemap")   :texture        ("$user$tonemap")

	shader:dx10texture	("s_clouds0", "null")
	shader:dx10texture	("s_clouds1", "null")
	shader:dx10texture	("s_tonemap", "$user$tonemap")

	shader:dx10sampler	("smp_base")
--	shader:dx10sampler	("smp_linear")
end
