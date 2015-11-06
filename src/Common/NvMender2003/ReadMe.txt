========================================================================
    STATIC LIBRARY : nv_meshmender 
========================================================================

revision history

2-05-2004
  - added the mapping from new to old vertices

1-26-2004
  - grouped parameters to Mend() differently
  - added default values to Mend() parameters
  - renamed MeshMender::MenderVertex to  MeshMender::Vertex since the extra mender was redundant
  - added some more comments in the options section with advice on usage
  - turned bool parameters into enums so that they aren't easily mixed up.
  
    