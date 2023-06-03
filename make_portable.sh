#! /bin/bash

DTOOLS="dialog"
OUT_DIR=$HOME/OpenXRay/
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

    mkdir -p $OUT_DIR/{bin,cop,cs}
    cp -v temp/usr/bin/xr_3da $OUT_DIR/bin/
    cp -v ../src/xr_3da/xr_3da.sh $OUT_DIR/bin/xr_3da.sh
    chmod 755 $OUT_DIR/bin/xr_3da.sh
    cp -v temp/usr/lib/*.so $OUT_DIR/bin/
    cp -v -r temp/usr/share/openxray/* $OUT_DIR/cop/
    cp -v -r temp/usr/share/openxray/* $OUT_DIR/cs/

cat >$OUT_DIR/Start_cop.sh <<END
#!/bin/sh

cd bin
./xr_3da.sh -fsltx ../cop/fsgame.ltx
END

cat >$OUT_DIR/Start_cs.sh <<END
#!/bin/sh

cd bin
./xr_3da.sh -cs -fsltx ../cs/fsgame.ltx
END

    chmod 755 $OUT_DIR/Start_cop.sh
    chmod 755 $OUT_DIR/Start_cs.sh

    $DTOOLS --backtitle "OpenXRay Tools" --title "Completed" --msgbox "OpenXRay engine is built and placed in $OUT_DIR In order to run the game you should copy the game resources from the original licensed copy.

You need to copy the following directories:
levels, localization, mp, patches, resources" 12 70
    main
    clear
}

#=================================== Copy function.

copyfile() {
    DESTRDIR=$1
    {
        # Check for compatibility 
        CSPATCHVER=`find $DESTRDIR -name xpatch_10.db`
        if [[ -f $CSPATCHVER ]]; then
            $DTOOLS --backtitle "OpenXRay Tools" --title "Detected" --msgbox "Distribution S.T.A.L.K.E.R.: Clear Sky version 1.5.10 detected." 10 60
            GAMEDIR="cs"
        else
            COPATCHVER=`find $DESTRDIR -name xpatch_02.db`
            if [[ -f $COPATCHVER ]]; then
                $DTOOLS --backtitle "OpenXRay Tools" --title "Detected" --msgbox "Distribution S.T.A.L.K.E.R.: Call of Pripyat version 1.6.02 discovered." 10 60
                GAMEDIR="cop"
            else
                $DTOOLS --backtitle "OpenXRay Tools" --title "ERROR!!!" \
                --msgbox "No supported game version found! Make sure you specify the correct directory with the unpacked distribution of S.T.A.L.K.E.R.: Clear Sky version 1.5.10 or S.T.A.L.K.E.R.: Call of Pripyat version 1.6.02." 10 60
                main
            fi
        fi

        # Preparing the target directory
        rm -rf $OUT_DIR/$GAMEDIR/{levels,localization,mp,patches,resources}
        mkdir -p $OUT_DIR/$GAMEDIR/{levels,localization,mp,patches,resources}

        # Search for available localizations 
        LANGEN="Not available"
        FILEEN=`find $DESTRDIR -name xenglish.db`
        if [[ -f $FILEEN ]]; then
            LANGEN="Available"
        fi

        LANGFR="Not available"
        FILEFR=`find $DESTRDIR -name xfrench.db`
        if [[ -f $FILEFR ]]; then
            LANGFR="Available"
        fi

        LANGDE="Not available"
        FILEDE=`find $DESTRDIR -name xgerman.db`
        if [[ -f $FILEDE ]]; then
            LANGDE="Available"
        fi

        LANGIT="Not available"
        FILEIT=`find $DESTRDIR -name xitalian.db`
        if [[ -f $FILEIT ]]; then
            LANGIT="Available"
        fi

        LANGSP="Not available"
        FILESP=`find $DESTRDIR -name xspanish.db`
        if [[ -f $FILESP ]]; then
            LANGSP="Available"
        fi

        LANGRU="Not available"
        FILERU=`find $DESTRDIR -name xrussian.db`
        if [[ -f $FILERU ]]; then
            LANGRU="Available"
        fi

        LANGPO="Not available"
        FILEPO=`find $DESTRDIR -name xpolish_texts.db`
        if [[ -f $FILEPO ]]; then
            LANGPO="Available"
        else
            FILEPO=`find $DESTRDIR -name xpolish_mpck0.db`
            if [[ -f $FILEPO ]]; then
                LANGPO="Available"
            fi
        fi

        # Localization selection dialog
        # NOTE: I don't know yet how to add elements dynamically.
        LANGUAGE=$($DTOOLS --backtitle "OpenXRay Tools" --title  "Language" --radiolist \
        "Choose an available language pack" 15 60 4 \
        "English" "$LANGEN" OFF \
        "French" "$LANGFR" OFF \
        "Deutsch" "$LANGDE" OFF \
        "Italian" "$LANGIT" OFF \
        "Spanish" "$LANGSP" OFF \
        "Russian" "$LANGRU" OFF \
        "Polish" "$LANGPO" OFF 3>&1 1>&2 2>&3)

        case $LANGUAGE in
            *English*)
                LANGPACK="base_sounds.db xefis_movies.db xenglish.db"
                let numfiles=3+35
            ;;
            *French*)
                LANGPACK="base_sounds.db xefis_movies.db xfrench.db"
                let numfiles=3+35
            ;;
            *Deutsch*)
                LANGPACK="base_sounds.db xefis_movies.db xgerman.db"
                let numfiles=3+35
            ;;
            *Italian*)
                LANGPACK="base_sounds.db xefis_movies.db xitalian.db"
                let numfiles=3+35
            ;;
            *Spanish*)
                LANGPACK="base_sounds.db xefis_movies.db xspanish.db"
                let numfiles=3+35
            ;;
            *Russian*)
                LANGPACK="base_sounds.db xefis_movies.db xrussian.db"
                let numfiles=3+35
            ;;
            *Polish*)
                LANGPACK="base_sounds.db xpolish_texts.db xrus_sounds.db xxpolish_sounds.db xpolish_mpck0.db xpolish_p_patch.db"
                let numfiles=6+35
            ;;
            *)
                # FIXME: When canceling, we return to the main menu as intended, but when you exit the main menu, the search for files begins instead of just exiting 
                main
                clear
            ;;
        esac
    }

    {
        # Search and copy all available patches
        for RESFILE in xpatch_02.db xpatch_03_steam.db xpatch_04.db xpatch_05.db xpatch_07.db xpatch_08.db xpatch_10.db
        do
            let i++
            find $DESTRDIR -name $RESFILE -exec cp {} $OUT_DIR/$GAMEDIR/patches \;
            echo "scale=2;$i/$numfiles*100" | bc -l | cut -d. -f1
        done

        # Search and copy resources
        let i++
        find $DESTRDIR -name configs.db -exec cp {} $OUT_DIR/$GAMEDIR/resources \;
        echo "scale=2;$i/$numfiles*100" | bc -l | cut -d. -f1

        for RESFILE in levels.db0 levels.db1 levels.db2 resources.db0 resources.db1 resources.db2 resources.db3 resources.db4
        do
            let i++
            find $DESTRDIR -name $RESFILE -exec cp {} $OUT_DIR/$GAMEDIR/${RESFILE%.*} \;
            echo "scale=2;$i/$numfiles*100" | bc -l | cut -d. -f1
        done

        # Search and copy multiplayer maps
        # NOTE: At the moment, multiplayer is not supported, maybe it's not worth copying and saving about 800 megabytes of disk space?
        for RESFILE in mp_agroprom.db mp_atp.db mp_autostation.db mp_bath.db mp_darkvalley.db mp_factory.db mp_firestation.db mp_garbage.db mp_limansk.db mp_lost_village.db mp_pool.db mp_railroad.db mp_rembasa.db mp_rostok.db mp_sport_center.db mp_workshop.db mp_pripyat.db mp_close_combat.db mp_military1.db
        do
            let i++
            find $DESTRDIR -name $RESFILE -exec cp {} $OUT_DIR/$GAMEDIR/mp \;
            echo "scale=2;$i/$numfiles*100" | bc -l | cut -d. -f1
        done

        # Finding and copying localization files
        for RESFILE in $LANGPACK
        do
            let i++
            find $DESTRDIR -name $RESFILE -exec cp {} $OUT_DIR/$GAMEDIR/localization \;
            echo "scale=2;$i/$numfiles*100" | bc -l | cut -d. -f1
        done

    } | $DTOOLS --backtitle "OpenXRay Tools" --gauge "Copying files..." 6 60 0

    clear
}

#=================================== Resource manager function.

resmanager() {

    unpack_distribution() {

        DSETUP=$($DTOOLS --backtitle "OpenXRay Tools" --stdout --title "Select setup file (setup.exe) " --fselect $HOME/ 15 80)

        case $? in
            *0*)
                clear
                innoextract $DSETUP $1
                copyfile $OUT_DIR/temp/
                main
                clear
            ;;
            *1*)
                clear
                resmanager
                clear
            ;;
            *)
                clear
                resmanager
                clear
            ;;
        esac
    }

    RES=$($DTOOLS --backtitle "OpenXRay Tools" --title "Resource manager." --menu "Available actions." 15 70 7 \
    "1" "Справка по меню." \
    "2" "Copy files S.T.A.L.K.E.R." \
    "3" "Unpack distribution S.T.A.L.K.E.R." \
    "4" "Unpack GOG distribution S.T.A.L.K.E.R." 3>&1 1>&2 2>&3)


    case $RES in
        *1*)
            helpres
        ;;
        *2*)
            DIRS=$($DTOOLS --backtitle "OpenXRay Tools" --title "Specify a directory" --stdout --dselect $HOME/ 15 80)
            case $? in
                *0*)
                    clear
                    copyfile $DIRS
                    clear
                ;;
                *1*)
                    clear
                    resmanager
                    clear
                ;;
                *)
                    clear
                    resmanager
                    clear
                ;;
            esac
        ;;
        *3*)
            rm -fr $OUT_DIR/temp/
            unpack_distribution "-L -m -d $OUT_DIR/temp/"
            rm -fr $OUT_DIR/temp/
        ;;
        *4*)
            rm -fr $OUT_DIR/temp/
            unpack_distribution "-L -m --gog -d $OUT_DIR/temp/"
            rm -fr $OUT_DIR/temp/
        ;;
        *)
            main
            clear
        ;;
    esac
}

#=================================== Main function.

main(){

    OPTION=$($DTOOLS --cancel-label "Exit" --backtitle "OpenXRay Tools" --title "Build Menu" --menu "The finished engine will be located $OUT_DIR" 15 70 4 \
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
