local tex_base                = "water\\water_ryaska1"
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
  shader:sampler        ("s_base")       :texture  (tex_base)
  shader:sampler        ("s_nmap")       :texture  (tex_nmap)
  shader:sampler        ("s_env0")       :texture  (tex_env0)   : clamp()
  shader:sampler        ("s_env1")       :texture  (tex_env1)   : clamp()
end

function l_special        (shader, t_base, t_second, t_detail)
  shader:begin                ("waterd","waterd")
        : sorting        (2, true)
        : blend                (true,blend.srcalpha,blend.invsrcalpha)
        : zb                (true,false)
        : fog                (false)
        : distort        (true)
  shader:sampler        ("s_base")       :texture  (tex_base)
  shader:sampler        ("s_distort")    :texture  (tex_dist)
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
