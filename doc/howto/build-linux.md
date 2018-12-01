## Build and configure X-Ray 1.6 on Linux

Make sure you are using STALKER Call Of Pripyat v1.6.02 and your system contains the following packages:
git, cmake, GLEW, Freeimage, Lockfile, Openal, TBB, Crypto++, pugixml, Theora, Ogg, SDL2, Lzo, Jpeg.

To install software above in **Debian** and **Ubuntu** derivatives just run the command:

```
sudo apt install git cmake libglew-dev libfreeimage-dev liblockfile-dev libopenal-dev libtbb-dev libcrypto ++ - dev libpugixml-dev libogg-dev libtheora-dev libvorbis-dev libsdl2-dev liblzo-dev-libjdg2-dev
```

In case you are **Fedora** user:
```
sudo dnf install git cmake glew-devel freeimage-devel liblockfile-devel openal-devel tbb-devel cryptopp-devel pugixml-devel libogg-devel libtheora-devel libvorbis-devel SDL2-devel lzo-devel libjpeg-turbo-devel
```
Next clone the repo with its submodules:
```
git clone https://github.com/OpenXRay/xray-16.git && git submodule update --init --recursive
```
After the cloning is complete, the xray-16 directory should appear in the directory you invoked command above.
Next it is needed to create directory for binary output and navigate there:
```
cd xray-16 && mkdir bin && cd bin
```
Please pay attention to the errors in next stage. Since they can occur in case of absence of any components.
So, prepare your source to be compiled with:
```
cmake ..
```
You can customize build passing arguments to cmake.
To enable debugging:
```
cmake .. -DCMAKE_BUILD_TYPE = RelWithDebInfo # The string with the debug flag is no longer supported
```
To use clang:
```
CC=clang CXX=clang++ cmake ..
```
To include all optimizations for your machine:
```
CFLAGS="-march=native" CXXFLAGS="-march=native" cmake ..
```
Finally
```
make -jX
```
Where X your processor's number of cores
Also, to output the log and errors to different files:
```
make -jX > out.log 2 > error.log
```
To use linux binaries with the game we need to create directory `bin-linux` there.
So, once compilation is done in the `xray-16/bin` directory execute the command:
```
make DESTDIR=/path/to/your/SCOP/bin-linux install
```
And the `bin-linux` directory will appear where the game is located.
Yes, `/path/to/your/SCOP` it's the path to your legal copy of STALKER CoP.

Backup `res` folder and `fsgame.ltx` in the original game folder in the form you obtained by installing the game from Steam, GOG or license disc
Next copy from `xray-16` `res` folder and `fsgame.ltx` file to the `/path/to/your/SCOP`.

To run the game invoke one of the following commands:
If you want to debug the game, you need to prevent input capture:
```
setxkbmap -option grab: break_actions
```
Now you can run the game being in `bin-linux` directory using:
```
./xr_3da.sh -fsltx ../fsgame.ltx
```
if you want to debug using gdb:
```
DEBUGGER = "gdb --ex=r --args" ./xr_3da.sh -fsltx ../fsgame.ltx
```

You can also create a "Shortcut" to do this, create a file S.T.A.L.K.E.R.desktop with the following content
```
[Desktop Entry]
Type = Application
Terminal = false
Categories = Game
# The icon should be located at ~/.local/share/icons/
Icon = stalker_cop
# Full path to working directory
Path = /path/to/your/SCOP/linux-bin
# Full path to binaries
Exec = /path/to/your/SCOP/linux-bin/xr_3da.sh -fsltx ../fsgame.ltx
Name = S.T.A.L.K.E.R. Openxray engine
Comment=S.T.A.L.K.E.R. (OpenXRay Engine)
Comment[ru_RU]=S.T.A.L.K.E.R. (OpenXRay Engine)
Comment[uk]=S.T.A.L.K.E.R. (OpenXRay Engine)
GenericName=Not the official port of the GSC XRay 1.6.02 engine from the OpenXRay team
GenericName[ru_RU]=Не официальный порт движка GSC XRay 1.6.02 от команды OpenXRay
GenericName[uk]=Не офіційний порт движка GSC XRay 1.6.02 від команди OpenXRay
```

and place to to `~/.local/share/applications`.

Observe a badge with the name S.T.A.L.K.E.R in the system menu Games section

## Notes

If you are installing S.T.A.L.K.E.R Call Pripyat using *wine*. For example if you have a CD you can invoke `wine setup.exe` to start the installation. It is recommended not to use the path with spaces, for example, you can use `c: \ cop` as installation path (this will be the `~ / .wine / drive_c / cop` path on your linux host machine). Btw, do not forget to install patch 1.6.02 (only for the Russian language, should be included worldwide): http://cop.stalker-game.ru/?page=patches#2
In *Steam* you can find the game in folder `{STEAM_DIR}/steamapps/common/STALKER Call of Pripyat` if you did not override default Steam settings
