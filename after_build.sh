#!/bin/bash

# Simple script to copy our new game files over to a 
# predefined destination

rsync -Puhr ./bin/x64/Debug/* ./CoP/bin/
rsync -Puh res/fsgame.ltx ./CoP/
rsync -Puh  src/xr_3da/xr_3da.sh ./CoP/bin/ 
