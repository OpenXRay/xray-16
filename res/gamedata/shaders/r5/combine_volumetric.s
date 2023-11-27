function normal		(shader, t_base, t_second, t_detail)
	shader:begin	("combine_1", "combine_volumetric")
			: fog		(false)
			: zb 		(false,false)
			: blend		(true,blend.one,blend.one)
--			: aref 		(true,0)	--	enable to save bandwith?
			: sorting	(2, false)
--	TOD0: DX10: Implement samplers
--	shader:sampler	("s_vollight")  :texture	("$user$generic2")
--	shader:sampler 	("s_tonemap")   :texture        ("$user$tonemap")

	shader:dx10texture	("s_vollight", 	"$user$generic2")
	shader:dx10texture 	("s_tonemap", 	"$user$tonemap")

	shader:dx10sampler	("smp_nofilter")
end