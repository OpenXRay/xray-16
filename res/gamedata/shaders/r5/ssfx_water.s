-- @ Version: SCREEN SPACE SHADERS - UPDATE 21
-- @ Description: Water - Shader Config
-- @ Author: https://www.moddb.com/members/ascii1457
-- @ Mod: https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders

function normal(shader, t_base, t_second, t_detail)
	shader	:begin		("ssfx_water","ssfx_water")
			:sorting	(2, false)
			:blend		(true,blend.srcalpha,blend.invsrcalpha)
			:zb			(true,false)
			:distort	(true)
			:fog		(true)

	shader:dx10texture	("s_position",	"$user$position")
	shader:dx10texture  ("s_diffuse", "$user$albedo")
	shader:dx10texture  ("s_accumulator", "$user$accum")
	shader:dx10texture  ("s_rimage", "$user$generic_temp")


shader:dx10texture	("s_perlin", "water\\water_perlin")
shader:dx10texture  ("s_water_ssr_noblur", "$user$ssfx_water")

	shader:dx10texture  ("s_water_ssr", "$user$ssfx_temp")
	shader:dx10texture  ("s_water_height", "$user$ssfx_water_waves")

	shader:dx10texture	("s_base", "water\\water_water")
	shader:dx10texture	("s_nmap", "fx\\water_normal")

	shader:dx10texture  ("s_rainsplash", "fx\\water_sbumpvolume")
	shader:dx10texture  ("s_watercaustics", "fx\\water_caustics")
	shader:dx10texture  ("s_wind", "fx\\water_wind")

	shader:dx10sampler	("smp_base")
	shader:dx10sampler	("smp_linear")
	shader:dx10sampler	("smp_nofilter")
end