#! /bin/bash

DTOOLS="dialog"
OUT=~/OpenXRay
DEF_COPY_PATH=~
OS_RELEASE_FILES=("/etc/os-release" "/usr/lib/os-release")

#=================================== Help function.

helps(){
    $DTOOLS --backtitle "OpenXRay Tools" --title "Help." --msgbox "This script will help you easily build the OpenXRay engine and set it up to run. The script was compiled \
as a result of numerous requests from users who have the same type of minor errors as a result of their little preparation in order to simplify the \
process of building the engine.
The following functions are implemented in the Script:
1) Updating the source code tree.
2) Building the OpenXRay engine
3) Unpacking the installation package with the game. (system must have innoextract installed)
4) Copying game resources" 14 100
    main
}

helpres(){
    $DTOOLS --backtitle "OpenXRay Tools" --title "Help Resource manager." --msgbox  "To run the game, you need the game resources of the original licensed copy of \
S.T.A.L.K.E.R. - Call of Pripyat version 1.6.02 and S.T.A.L.K.E.R. - Clear Sky version 1.5.10. If you have the game installed, you can use the \
first two menu options, the script will copy the necessary files to the output directory. If you have a distribution kit for the game, then you \
can use the appropriate menu items to unpack it and get the necessary resources." 14 100
    resmanager
}

#=================================== Source code update feature.

update_src(){
    if test -f "/usr/bin/git"; then
        git pull
        git submodule update --init --recursive
        main
    else
        $DTOOLS --backtitle "OpenXRay Tools" --title  "Error!!!" --msgbox  "git not found, please install git." 10 60
        main
    fi
}

#=================================== Dependency installation function.
#This function is borrowed from the MangoHud project.
#https://github.com/flightlessmango/MangoHud/blob/master/build.sh

# Correctly identify the os-release file.
for os_release in ${OS_RELEASE_FILES[@]} ; do
    if [[ ! -e "${os_release}" ]]; then
        continue
    fi
    DISTRO=$(sed -rn 's/^ID(_LIKE)*=(.+)/\L\2/p' ${os_release} | sed 's/"//g')
done

dependencies() {
        missing_deps() {

            if ($DTOOLS --backtitle "OpenXRay Tools" --title  "Installing dependencies." --yesno  "Missing dependencies for $DISTRO:
$INSTALL

Do you want the script to install these packages?" 15 60)  then
                PERMISSION=install_deps
            else
                $DTOOLS --backtitle "OpenXRay Tools" --title  "Attention!!!" --msgbox  "I continue without installing dependencies." 10 60
            fi
        }
        dep_install() {
            set +e
            for i in $(eval echo $DEPS); do
                $MANAGER_QUERY "$i" &> /dev/null
                if [[ $? == 1 ]]; then
                    INSTALL="$INSTALL""$i "
                fi
            done
            if [[ ! -z "$INSTALL" ]]; then
                missing_deps
                if [[ "$PERMISSION" == "install_deps" ]]; then
                    sudo $MANAGER_INSTALL $INSTALL
                fi
            fi
            set -e
        }

        for i in $DISTRO; do
        case $i in
            *arch*|*manjaro*)
                MANAGER_QUERY="pacman -Q"
                MANAGER_INSTALL="pacman -S"
                DEPS="{gcc,cmake,make,libglvnd,libjpeg6-turbo,ncurses,glew,sdl2,openal,crypto++,libogg,libtheora,libvorbis,lzo,lzop,libjpeg-turbo}"
                dep_install
                break
            ;;
            *fedora*)
                MANAGER_QUERY="dnf list installed"
                MANAGER_INSTALL="dnf install"
                DEPS="{gcc,gcc-c++,cmake,make,glew-devel,openal-devel,cryptopp-devel,libogg-devel,libtheora-devel,libvorbis-devel,SDL2-devel,lzo-devel,libjpeg-turbo-devel}"
                dep_install
                break
            ;;
            *rosa*)
                MANAGER_QUERY="dnf list installed"
                MANAGER_INSTALL="dnf install"
                DEPS="{gcc,gcc-c++,cmake,make,glew,ncurses,openal,lib64ncurses-devel,lib64openal-devel,lib64cryptopp-devel,lib64ogg-devel,lib64theora-devel,lib64vorbis-devel,lib64SDL2-devel,lib64lzo-devel,lib64jpeg-devel}"
                dep_install
                break
            ;;
            *debian*|*ubuntu*|*deepin*)
                MANAGER_QUERY="dpkg-query -s"
                MANAGER_INSTALL="apt install"
                DEPS="{gcc,g++,cmake,make,libglew-dev,libopenal-dev,libcrypto++-dev,libogg-dev,libtheora-dev,libvorbis-dev,libsdl2-dev,liblzo2-dev,libjpeg-dev,libncurses5-dev}"
                dep_install
                break
            ;;
            *suse*)
                MANAGER_QUERY="rpm -q"
                MANAGER_INSTALL="zypper install"
                DEPS="{gcc,gcc-c++,cmake,make,glew-devel,libcryptopp-devel,libogg-devel,libtheora-devel,libvorbis-devel,libSDL2-devel,libjpeg-turbo,openal-soft-devel,lzo-devel,openjpeg2-devel,libjpeg62-devel}"
                dep_install
                break
            ;;
            *void*)
                MANAGER_QUERY="xbps-query -Rs"
                MANAGER_INSTALL="xbps-install -Su"
                DEPS="{gcc,cmake,make,glew-devel,libogg-devel,libtheora-devel,libvorbis-devel,SDL2-devel,libjpeg-turbo-devel,libopenal-devel,lzo-devel,libopenjpeg2-devel,ncurses-devel}"
                dep_install
                break
            ;;
            *)
                $DTOOLS --backtitle "OpenXRay Tools" --title "Error!!!" --msgbox  "Could not find information about your distribution! Automatic installation of dependencies is not available. \
Trying to build the OpenXRay engine no matter what. If an error occurs during compilation make sure you have the following packages installed:
gcc cmake make libglvnd libjpeg6-turbo ncurses glew sdl2 openal crypto++ libogg libtheora libvorbis lzo lzop libjpeg-turbo

On some distributions, packages may be split into two and prefixed with -dev or -devel, such as lzo lzo-dev, these packages should also be installed." 16 80
        esac
        done
}

#=================================== Assembly function.

build(){
    dependencies
   rm -f -R bin
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

    $DTOOLS --backtitle "OpenXRay Tools" --title "Completed" --msgbox "OpenXRay engine is built and placed in $OUT In order to run the game you should copy the game resources from the original licensed copy.

You need to copy the following directories:
levels, localization, mp, patches, resources" 12 70
    main
}

#=================================== Copy function.

res_copy(){
    case $1 in
        *cop*)
        OUT_PATH=$OUT/cop
        TITLES_GAME=Call\ of\ Pripyat
        ;;
        *cs*)
        OUT_PATH=$OUT/cs
        TITLES_GAME=Clear\ Sky
        ;;
        *)
        echo "Error"
        ;;
    esac

    PET=$($DTOOLS --backtitle "OpenXRay Tools" --title "Copying game resources." --inputbox "Specify the directory with the installed game S.T.A.L.K.E.R. - $TITLES_GAME" 10 60 $DEF_COPY_PATH 3>&1 1>&2 2>&3)
    exitstatus=$?
    if [ $exitstatus = 0 ];  then
        echo "Copying in progress, please wait..."
        mkdir -p $OUT_PATH/{levels,localization,mp,patches,resources}
        cp -r -u -v $PET/{levels,mp,patches,resources} $OUT_PATH
        cp -r -u -v $PET/*ocalization/* $OUT_PATH/localization
        $DTOOLS --backtitle "OpenXRay Tools" --title  "Done" --msgbox  "S.T.A.L.K.E.R - $TITLES_GAME resources copied successfully" 10 60
        main
    else
        main
    fi
}

#=================================== Resource manager function.

resmanager(){
    RES=$($DTOOLS --backtitle "OpenXRay Tools" --title "Resource manager." --menu "Доступные операции." 15 70 7 \
"1" "Справка по меню." \
"2" "Copy files S.T.A.L.K.E.R. - Call of Pripyat." \
"3" "Copy files S.T.A.L.K.E.R. - Clear Sky." \
"4" "Unpack S.T.A.L.K.E.R. - Call of Pripyat." \
"5" "Unpack S.T.A.L.K.E.R. - Call of Pripyat GOG." \
"6" "Unpack S.T.A.L.K.E.R. - Clear Sky." \
"7" "Unpack S.T.A.L.K.E.R. - Clear Sky GOG." 3>&1 1>&2 2>&3)
    condactor(){
    UNPACKPET=$($DTOOLS --backtitle "OpenXRay Tools" --title "Путь к дистрибутиву" --inputbox "Укажите путь в папку дистрибутива." 10 60 ~/ 3>&1 1>&2 2>&3)
    exitstatus=$?
    if [ $exitstatus = 0 ];  then
        DSETUP=$($DTOOLS --backtitle "OpenXRay Tools" --title "Путь к дистрибутиву" --inputbox "Укажите имя установочного файла, обычно он называется setup.exe
==================================================================
$(find $UNPACKPET -maxdepth 1 -name "*.exe")" 15 70 setup.exe 3>&1 1>&2 2>&3)
        exitstatus=$?
        if [ $exitstatus = 1 ];  then
            main
        fi
    else
        main
    fi
}
    case $RES in
        *1*)
        helpres
        ;;
        *2*)
        res_copy *cop*
        ;;
        *3*)
        res_copy *cs*
        ;;
        *4*)
        condactor
        innoextract $UNPACKPET/$DSETUP -L -d $UNPACKPET/temp
        ;;
        *5*)
        innoextract $UNPACKPET/$DSETUP -L --gog -d $UNPACKPET/temp
        ;;
        *6*)
        innoextract $UNPACKPET/$DSETUP -L -d $UNPACKPET/temp
        ;;
        *7*)
        innoextract $UNPACKPET/$DSETUP -L --gog -d $UNPACKPET/temp
        ;;
        *)
        main
    esac
}

#=================================== Main function.

main(){

    OPTION=$($DTOOLS --backtitle "OpenXRay Tools" --title "Build Menu" --menu "The finished engine will be located $OUT" 15 70 4 \
    "1" "Brief reference." \
    "2" "Update the source tree. " \
    "3" "Build the OpenXRay engine." \
    "4" "Resource manager." 3>&1 1>&2 2>&3)

    case $OPTION in
        *1*)
            clear
            helps
        ;;
        *2*)
            clear
            update_src
        ;;
        *3*)
            clear
            build
        ;;
        *4*)
            clear
            resmanager 
    esac

    clear
}

#=================================== Point of entry.

    if test -f "/usr/bin/$DTOOLS"; then
        main
        clear
    else
        echo -e "\033[37;1;41mFor the script to work, you need to install the dialog package.\033[0m"
    fi
