local tex_base                = "water\\water_ryaska1"
local tex_env                = "sky\\sky_5_cube"
local tex_dist                = "water\\water_dudv"
local tex_dist2                = "water\\water_dudv"

function normal                (shader, t_base, t_second, t_detail)
  shader:begin                ("water","water")
        : sorting        (2, true)
        : blend                (true,blend.srcalpha,blend.invsrcalpha)
              : aref                (true,0)
              : zb                (true,false)
              : distort        (true)
              : fog                (true)
  shader:sampler        ("s_base")       :texture  (tex_base)
  shader:sampler        ("s_env")        :texture  (tex_env)   : clamp()
end

function l_special        (shader, t_base, t_second, t_detail)
  shader:begin                ("waterd","waterd")
        : sorting        (2, true)
        : blend                (true,blend.srcalpha,blend.invsrcalpha)
        : zb                (true,false)
        : fog                (false)
        : distort        (true)
  shader:sampler        ("s_base")       :texture  (tex_base)
  shader:sampler        ("s_distort0")   :texture  (tex_dist)
  shader:sampler        ("s_distort1")   :texture  (tex_dist2)
end

