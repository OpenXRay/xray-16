#! /bin/bash

OUT=~/OpenXRay
DEF_COPY_PATH=~

#=================================== Функция справки

helps(){
    whiptail --title  " Help " --msgbox  "  This script will help you easily build the OpenXRay engine and set it up to run. The script was compiled as a result of numerous requests from users who have the same type of minor errors as a result of their little preparation in order to simplify the process of building the engine.
The following functions are implemented in the Script:
1) Updating the source code tree.
2) Building the OpenXRay engine
3) Unpacking the installation package with the game. (system must have innoextract installed)
4) Copying game resources" 14 100
    main
}

#=================================== Функция обновления

update_src(){
    git pull
    main
}

#=================================== Функция сборки

build(){
    if (whiptail --title  " Dependencies " --yes-button  "Build" --no-button  "Cancel" --yesno  "   Before building the OpenXRay engine, you should make sure you have the following packages installed:

gcc cmake make libglvnd libjpeg6-turbo ncurses glew sdl2 openal crypto++ libogg libtheora libvorbis lzo lzop libjpeg-turbo

Attention!!! On some distributions, packages may be prefixed with -dev e.g. libglvnd-dev libjpeg6-turbo-dev " 14 130)  then

#   rm -f -R bin
   mkdir -p bin
   cd bin
   cmake .. -DCMAKE_BUILD_TYPE=Release \
   -DCMAKE_INSTALL_PREFIX=/usr \
   -DCMAKE_INSTALL_LIBDIR=lib
   make -j$(nproc)
   make DESTDIR=`pwd`/temp install

   mkdir -p $OUT/{bin,cop,cs}
   cp -v temp/usr/bin/xr_3da $OUT/bin/
   cp -v ../src/xr_3da/xr_3da.sh $OUT/bin/xr_3da.sh
   chmod 755 $OUT/bin/xr_3da.sh
   cp -v temp/usr/lib/*.so $OUT/bin/
   cp -v -r temp/usr/share/openxray/* $OUT/cop/
   cp -v -r temp/usr/share/openxray/* $OUT/cs/

cat >$OUT/Start_cop.sh <<END
#!/bin/sh

cd bin
./xr_3da.sh -fsltx ../cop/fsgame.ltx
END

cat >$OUT/Start_cs.sh <<END
#!/bin/sh

cd bin
./xr_3da.sh -cs -fsltx ../cs/fsgame.ltx
END

    chmod 755 $OUT/Start_cop.sh
    chmod 755 $OUT/Start_cs.sh

    whiptail --title  "Completed" --msgbox  "   OpenXRay engine is built and placed in $OUT In order to run the game you should copy the game resources from the original licensed copy.

You need to copy the following directories:
levels, localization, mp, patches, resources" 12 70
    main
    else
    main
    fi
}

#=================================== Функция распаковки

unpack(){
    whiptail --title  "Error" --msgbox  "Unpacking has not yet been implemented." 10 60
    main
}

#=================================== Функция копирования

res_copy(){
    case $1 in
        cop)
        OUT_PATH=$OUT/cop
        TITLES_GAME=Call\ of\ Pripyat
        ;;
        cs)
        OUT_PATH=$OUT/cs
        TITLES_GAME=Clear\ Sky
        ;;
    *)
        echo "Error"
        ;;
    esac

    PET=$(whiptail --title  "Copying game resources." --inputbox  "   Specify the directory with the installed game S.T.A.L.K.E.R. - $TITLES_GAME" 10 60 $DEF_COPY_PATH 3>&1 1>&2 2>&3)
    exitstatus=$?
    if [ $exitstatus = 0 ];  then
        echo "Copying in progress, please wait..."
        mkdir -p $OUT_PATH/{levels,localization,mp,patches,resources}
        cp -r -u -v $PET/{levels,mp,patches,resources} $OUT_PATH
        cp -r -u -v $PET/*ocalization/* $OUT_PATH/localization
        whiptail --title  "Done" --msgbox  "S.T.A.L.K.E.R - $TITLES_GAME resources copied successfully" 10 60
        main
    else
        main
    fi
}

#=================================== Главная функция

main(){

OPTION=$(whiptail --title  "Build Menu" --menu  "The folder with the finished engine wakes up $OUT" 15 70 7 \
"1" "Brief reference." \
"2" "Update the source tree. " \
"3" "Build the OpenXRay engine." \
"4" "Unpack the distribution." \
"5" "Copy files S.T.A.L.K.E.R. - Call of Pripyat." \
"6" "Copy files S.T.A.L.K.E.R. - Clear Sky." 3>&1 1>&2 2>&3)
 
case $OPTION in
                "1")
                helps
             ;;
                "2")
                update_src
             ;;
                "3")
                build
             ;;
                "4")
                unpack
             ;;
                "5")
                res_copy "cop"

             ;;
                "6")
                res_copy "cs"

             ;;
            255)
            echo "The ESC key has been pressed.";;
esac
}

#=================================== Точка входа
main
