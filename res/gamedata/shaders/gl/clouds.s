function normal         (shader, t_base, t_second, t_detail)
        shader:begin    ("clouds","clouds")
                        : fog               (false)
--	TODO: DX10: Check if this is ok.
--                        : zb                (true,false)
						: zb                (false,false)
                        : sorting        	(3, true)
                        : blend             (true, blend.srcalpha,blend.invsrcalpha)
	shader:sampler        ("s_clouds0")   :texture        ("$user$clouds0")        : wrap() : f_anisotropic()
	shader:sampler        ("s_clouds1")   :texture        ("$user$clouds1")        : wrap() : f_anisotropic()
	shader:sampler        ("s_tonemap")   :texture        ("$user$tonemap")
end
