#! /bin/bash

OUT=~/OpenXRay
DEF_COPY_PATH=~
OS_RELEASE_FILES=("/etc/os-release" "/usr/lib/os-release")

#=================================== Help function.

helps(){
    whiptail --title  " Help " --msgbox  "  This script will help you easily build the OpenXRay engine and set it up to run. The script was compiled as a result of numerous requests from users who have the same type of minor errors as a result of their little preparation in order to simplify the process of building the engine.
The following functions are implemented in the Script:
1) Updating the source code tree.
2) Building the OpenXRay engine
3) Unpacking the installation package with the game. (system must have innoextract installed)
4) Copying game resources" 14 100
    main
}

#=================================== Source code update feature.

update_src(){
    git pull
    main
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

            if (whiptail --title  "Installing dependencies." --yesno  "Missing dependencies for $DISTRO:
$INSTALL

Do you want the script to install these packages?" 15 60)  then
                PERMISSION=install_deps
            else
                whiptail --title  "Attention!!!" --msgbox  "I continue without installing dependencies." 10 60
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
            *)
                whiptail --title  "Error!!!" --msgbox  "Could not find information about your distribution! Automatic installation of dependencies is not available. Trying to build the OpenXRay engine no matter what. If an error occurs during compilation make sure you have the following packages installed:
gcc cmake make libglvnd libjpeg6-turbo ncurses glew sdl2 openal crypto++ libogg libtheora libvorbis lzo lzop libjpeg-turbo

On some distributions, packages may be split into two and prefixed with -dev or -devel, such as lzo lzo-dev, these packages should also be installed." 16 80
        esac
        done
}

#=================================== Assembly function.

build(){
    dependencies
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
}

#=================================== Unpack function.

unpack(){
    whiptail --title  "Error" --msgbox  "Unpacking has not yet been implemented." 10 60
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

#=================================== Main function.

main(){

OPTION=$(whiptail --title  "Build Menu" --menu  "The folder with the finished engine wakes up $OUT" 15 70 7 \
"1" "Brief reference." \
"2" "Update the source tree. " \
"3" "Build the OpenXRay engine." \
"4" "Unpack the distribution." \
"5" "Copy files S.T.A.L.K.E.R. - Call of Pripyat." \
"6" "Copy files S.T.A.L.K.E.R. - Clear Sky." 3>&1 1>&2 2>&3)

    case $OPTION in
        *1*)
        helps
        ;;
        *2*)
        update_src
        ;;
        *3*)
        build
        ;;
        *4*)
        unpack
        ;;
        *5*)
        res_copy *cop*
        ;;
        *6*)
        res_copy *cs*
        ;;
        *255*)
        echo "The ESC key has been pressed.";;
    esac
}

#=================================== Point of entry.

    if test -f "/usr/bin/whiptail"; then
        main
    else
        echo -e "\033[37;1;41mFor the script to work, you need to install the newt package.\033[0m"
    fi
