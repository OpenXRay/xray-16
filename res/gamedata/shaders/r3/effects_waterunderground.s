local tex_base                = "water\\water_ryaska1"
local tex_nmap                = "fx\\water_normal"
local tex_dist                = "water\\water_dudv"
local tex_sky0                = "$user$sky0"         -- "sky\\sky_8_cube"
local tex_sky1                = "$user$sky1"         -- "sky\\sky_8_cube"

local tex_bluenoise           = "fx\\blue_noise"
local tex_rainsplash          = "fx\\water_sbumpvolume"
local tex_caustics 	     	  = "fx\\water_caustics"

function normal                (shader, t_base, t_second, t_detail)
	shader	:begin		("water_underground","water_underground")
    		:sorting	(2, false)
			:blend		(true,blend.srcalpha,blend.invsrcalpha)
			:zb			(true,false)
			:distort	(true)
			:fog		(true)

	shader:dx10texture	("s_base",		tex_base)
	shader:dx10texture	("s_nmap",		tex_nmap)
	shader:dx10texture	("sky_s0",		tex_sky0)
	shader:dx10texture	("sky_s1",		tex_sky1)
	shader:dx10texture	("s_position", "$user$position")

shader:dx10texture  ("s_rimage", "$user$generic_temp")
shader:dx10texture  ("s_diffuse", "$user$albedo")
shader:dx10texture  ("s_accumulator", "$user$accum")

shader:dx10texture  ("s_bluenoise", tex_bluenoise)
shader:dx10texture  ("s_rainsplash", tex_rainsplash)
shader:dx10texture  ("s_watercaustics", tex_caustics)	

	shader:dx10sampler	("smp_base")
	shader:dx10sampler	("smp_linear")
	shader:dx10sampler	("smp_nofilter")
	shader:dx10sampler	("smp_rtlinear")
end

function l_special        (shader, t_base, t_second, t_detail)
	shader	:begin                ("waterd","waterd")
			:sorting        (2, true)
			:blend                (true,blend.srcalpha,blend.invsrcalpha)
			:zb                (true,false)
			:fog                (false)
			:distort        (true)

	shader: dx10color_write_enable( true, true, true, false)

	shader:dx10texture	("s_base",		tex_base)
	shader:dx10texture	("s_distort",	tex_dist)

	shader:dx10sampler	("smp_base")
	shader:dx10sampler	("smp_nofilter")	
end