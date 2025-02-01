-- @ Version: SCREEN SPACE SHADERS - UPDATE 21
-- @ Description: Water SSR - Shader Config
-- @ Author: https://www.moddb.com/members/ascii1457
-- @ Mod: https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders

function normal(shader, t_base, t_second, t_detail)
	shader	:begin		("water_regular","ssfx_water_ssr")
			:sorting	(2, false)
			:zb			(true,false)
			:distort	(true)
			:fog		(true)

	shader:dx10texture  ("ssr_image", "$user$ssfx_water")

	shader:dx10texture	("sky_s0", "$user$sky0")
	shader:dx10texture	("sky_s1", "$user$sky1")
	shader:dx10texture	("s_position", "$user$position")
	shader:dx10texture  ("s_rimage", "$user$generic_temp")
	shader:dx10texture  ("s_bluenoise", "fx\\blue_noise")

	shader:dx10sampler	("smp_linear")
	shader:dx10sampler	("smp_nofilter")
end