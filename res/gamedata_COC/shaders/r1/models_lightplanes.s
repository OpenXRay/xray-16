function normal    	(shader, t_base, t_second, t_detail)
  shader:begin  	("model_def_lplanes","base_lplanes")
      : fog			(false)
      : zb			(true,false)
      : blend   	(true,blend.srcalpha,blend.one)
      : aref    	(true,0)
      : sorting 	(2, false)
  shader:sampler  	("s_base")      :texture  (t_base)
end
