local tex_base                = "water\\water_studen"
local tex_nmap                = "water\\water_normal"
local tex_dist                = "water\\water_dudv"
local tex_env0                = "$user$sky0"         -- "sky\\sky_8_cube"
local tex_env1                = "$user$sky1"         -- "sky\\sky_8_cube"

function normal                (shader, t_base, t_second, t_detail)
  shader:begin                ("water","water")
        : sorting        (2, false)
        : blend                (false,blend.srcalpha,blend.invsrcalpha)
              : zb                (true,false)
             : distort        (true)
              : fog                (true)

	shader: dx10color_write_enable( true, true, true, false)
--  shader:sampler        ("s_base")       :texture  (tex_base)
--  shader:sampler        ("s_nmap")       :texture  (tex_nmap)
--  shader:sampler        ("s_env0")       :texture  (tex_env0)   : clamp()
--  shader:sampler        ("s_env1")       :texture  (tex_env1)   : clamp()
	shader:dx10texture	("s_base",		tex_base)
	shader:dx10texture	("s_nmap",		tex_nmap)
	shader:dx10texture	("s_env0",		tex_env0)
	shader:dx10texture	("s_env1",		tex_env1)

	shader:dx10sampler	("smp_base")
	shader:dx10sampler	("smp_rtlinear")
end

function l_special        (shader, t_base, t_second, t_detail)
  shader:begin                ("waterd","waterd")
        : sorting        (2, true)
        : blend                (true,blend.srcalpha,blend.invsrcalpha)
        : zb                (true,false)
        : fog                (false)
        : distort        (true)
--  shader:sampler        ("s_base")       :texture  (tex_base)
--  shader:sampler        ("s_distort")    :texture  (tex_dist)
	shader:dx10texture	("s_base",		tex_base)
	shader:dx10texture	("s_distort",	tex_dist)

	shader:dx10sampler	("smp_base")
end

--[[
function normal                (shader, t_base, t_second, t_detail)
  shader:begin                ("waterd","waterd")
        : sorting        (2, true)
        : blend                (true,blend.srcalpha,blend.invsrcalpha)
        : zb                (true,false)
        : fog                (false)
        : distort        (true)
  shader:sampler        ("s_base")       :texture  (tex_base)
  shader:sampler        ("s_distort")    :texture  (tex_dist)
end
]]
