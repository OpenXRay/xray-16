@C:\DEV\SDK\dx9se4_jun\Utilities\Bin\x86\fxc /DUSE_SHADER3=1 	/nologo /Gfp /Tps_3_0 /Emain /Zpr /Fctest\p30_%1 %1
@C:\tools\NVSpNV40\nvshaderperf -v 1 -o test\p30_%1.log -a NV40 test\p30_%1
@C:\tools\NVSpG70\nvshaderperf 			-a NV40 test\p30_%1 >> test\p30_%1.log 
@C:\tools\NVSpG70\nvshaderperf -minbranch 	-a NV40 test\p30_%1 >> test\p30_%1.log 
@C:\tools\NVSpG70\nvshaderperf 			-a G70 	test\p30_%1 >> test\p30_%1.log 
@C:\tools\NVSpG70\nvshaderperf -minbranch 	-a G70 	test\p30_%1 >> test\p30_%1.log 
