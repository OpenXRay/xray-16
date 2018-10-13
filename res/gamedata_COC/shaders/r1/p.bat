rem @C:\DEV\SDK\dx9se4_jun\Utilities\Bin\x86\fxc  /nologo /Tps_1_1 /Emain_ps_1_1 /Zpr /Fctest\p11_%1 %1
rem @C:\DEV\SDK\dx9se4_jun\Utilities\Bin\x86\fxc  /nologo /Tps_2_0 /Emain_ps_1_1 /Zpr /Fctest\p20_%1 %1
rem @C:\DEV\SDK\dx9se4_jun\Utilities\Bin\x86\fxc  /nologo /Tps_3_0 /Emain_ps_1_1 /Zpr /Fctest\p30_%1 %1
rem @C:\tools\NVSpG70\nvshaderperf -a NV30 test\p11_%1 >  test\pXX_%1.log
rem @C:\tools\NVSpG70\nvshaderperf -a NV35 test\p11_%1 >> test\pXX_%1.log
rem @C:\tools\NVSpG70\nvshaderperf -a NV40 test\p11_%1 >> test\pXX_%1.log 
rem @C:\tools\NVSpG70\nvshaderperf -a G70  test\p11_%1 >> test\pXX_%1.log
rem @rem @C:\tools\NVSpG70\nvshaderperf -a G70  test\p30_%1 >> test\pXX_%1.log 

fxc  /nologo /Tps_1_1 /Emain_ps_1_1 /Zpr /Fctest\p11_%1 %1
fxc  /nologo /Tps_2_0 /Emain_ps_1_1 /Zpr /Fctest\p20_%1 %1
fxc  /nologo /Tps_3_0 /Emain_ps_1_1 /Zpr /Fctest\p30_%1 %1
