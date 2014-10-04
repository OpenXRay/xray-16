function normal    (shader, t_base, t_second, t_detail)
  shader:begin  ("model_distort","particle")    -- particle_alphaonly
      : sorting  (2,true)
      : blend    (true,blend.srcalpha,blend.invsrcalpha)
      : aref     (true,0)
      : zb     (true,false)
      : fog    (false)
      : distort   (true)
  shader:sampler  ("s_base")      :texture  (t_base)
end

function l_special  (shader, t_base, t_second, t_detail)
  shader:begin  ("model_distort","particle_distort")
      : sorting  (3, false)
      : blend    (true,blend.srcalpha,blend.invsrcalpha)
      : zb     (true,false)
      : fog    (false)
      : distort   (true)
  shader:sampler  ("s_base")      :texture  (t_base)
  shader:sampler  ("s_distort")   :texture  (t_base)  -- "pfx\\pfx_distortion"
end
