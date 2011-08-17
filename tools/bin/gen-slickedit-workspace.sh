# !kmk_ash
# $Id$
## @file
# Script for generating a SlickEdit workspace.
#

#
# Copyright (C) 2009-2011 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

#
# Some constants.
#
MY_CAT="kmk_cat"
MY_MKDIR="kmk_mkdir"
MY_MV="kmk_mv"
MY_SED="kmk_sed"
MY_RM="kmk_rm"
MY_SLEEP="kmk_sleep"
MY_EXPR="kmk_expr"
MY_SVN="svn"

#MY_SORT="kmk_cat"
MY_SORT="sort"

#
# Globals.
#
MY_PROJECT_FILES=""
MY_OUT_DIRS="\
out/${KBUILD_TARGET}.${KBUILD_TARGET_ARCH}/${KBUILD_TYPE} \
out/${BUILD_TARGET}.${BUILD_TARGET_ARCH}/${BUILD_TYPE} \
out/${KBUILD_TARGET}.${KBUILD_TARGET_ARCH}/debug \
out/${BUILD_TARGET}.${BUILD_TARGET_ARCH}/debug \
out/${KBUILD_TARGET}.${KBUILD_TARGET_ARCH}/release \
out/${BUILD_TARGET}.${BUILD_TARGET_ARCH}/release \
out/linux.amd64/debug \
out/linux.x86/debug \
out/win.amd64/debug \
out/win.x86/debug \
out/darwin.amd64/debug \
out/darwin.x86/debug \
out/solaris.amd64/debug \
out/solaris.x86/debug";


#
# Parameters w/ defaults.
#
MY_ROOT_DIR=".."
MY_OUT_DIR="SlickEdit"
MY_PRJ_PRF="VBox-"
MY_WS_NAME="VirtualBox.vpw"
MY_DBG=""
MY_WINDOWS_HOST=""
MY_OPT_MINIMAL=""

#MY_KBUILD_PATH="${KBUILD_PATH}"
#test -z "${MY_KBUILD_PATH}" && MY_KBUILD_PATH="${PATH_KBUILD}"
#MY_KBUILD=""


##
# Gets the absolute path to an existing directory.
#
# @param    $1      The path.
my_abs_dir()
{
    if test -n "${PWD}"; then
        MY_ABS_DIR=`cd ${MY_ROOT_DIR}/${1} && echo ${PWD}`
    else
        # old cygwin shell has no PWD and need adjusting.
        MY_ABS_DIR=`cd ${MY_ROOT_DIR}/${1} && pwd | ${MY_SED} -e 's,^/cygdrive/\(.\)/,\1:/,'`
    fi
    if test -z "${MY_ABS_DIR}"; then
        MY_ABS_DIR="${1}"
    fi
}

##
# Gets the file name part of a path.
#
# @param    $1      The path.
my_get_name()
{
    SAVED_IFS=$IFS
    IFS=":/ "
    set $1
    while test $# -gt 1  -a  -n "${2}";
    do
        shift;
    done

    IFS=$SAVED_IFS
    #echo "$1"
    export MY_GET_NAME=$1
}

##
# Generate file entry for the specified file if it was found to be of interest.
#
# @param    $1      The output file name base.
# @param    $2      The file name.
my_file()
{
    # sort and filter by file type.
    case "$2" in
        # drop these.
        *.kup|*~|*.pyc|*.exe|*.sys|*.dll|*.o|*.obj|*.lib|*.a|*.ko)
            return 0
            ;;

        # by prefix or directory component.
        tst*|*/testcase/*)
            MY_FOLDER="$1-Testcases.lst"
            ;;

        # by extension.
        *.c|*.cpp|*.m|*.mm|*.pl|*.py|*.as|*.c.h|*.cpp.h)
            MY_FOLDER="$1-Sources.lst"
            ;;

        *.h|*.hpp|*.mm)
            MY_FOLDER="$1-Headers.lst"
            ;;

        *.asm|*.s|*.S|*.inc|*.mac)
            MY_FOLDER="$1-Assembly.lst"
            ;;

        *)
            MY_FOLDER="$1-Others.lst"
            ;;
    esac

    ## @todo only files which are in subversion.
#    if ${MY_SVN} info "${2}" > /dev/null 2>&1; then
        my_get_name "${2}"
        echo ' <!-- sortkey: '"${MY_GET_NAME}"' --> <F N="'"${2}"'"/>' >> "${MY_FOLDER}"
#    fi
}

##
# Generate file entries for the specified sub-directory tree.
#
# @param    $1      The output filename.
# @param    $2      The sub-directory.
my_sub_tree()
{
    if test -n "$MY_DBG"; then echo "dbg: my_sub_tree: ${2}"; fi

    # Skip .svn directories.
    case "$2" in
        */.svn|*/.svn)
            return 0
            ;;
    esac

    # Process the files in the directory.
    for f in $2/*;
    do
        if test -d "${f}";
        then
            my_sub_tree "${1}" "${f}"
        else
            my_file "${1}" "${f}"
        fi
    done
    return 0;
}


##
# Generate folders for the specified directories and files.
#
# @param    $1      The output (base) file name.
# @param    $2+     The files and directories to traverse.
my_generate_folder()
{
    MY_FILE="$1"
    shift

    # Zap existing file collections.
    > "${MY_FILE}-Sources.lst"
    > "${MY_FILE}-Headers.lst"
    > "${MY_FILE}-Assembly.lst"
    > "${MY_FILE}-Testcases.lst"
    > "${MY_FILE}-Others.lst"

    # Traverse the directories and files.
    while test $# -ge 1  -a  -n "${1}";
    do
        for f in ${MY_ROOT_DIR}/$1;
        do
            if test -d "${f}";
            then
                my_sub_tree "${MY_FILE}" "${f}"
            else
                my_file "${MY_FILE}" "${f}"
            fi
        done
        shift
    done

    # Generate the folders.
    if test -s "${MY_FILE}-Sources.lst";
    then
        echo '        <Folder Name="Sources"  Filters="*.c;*.cpp;*.cpp.h;*.c.h;*.m;*.mm;*.pl;*.py;*.as">' >> "${MY_FILE}"
        ${MY_SORT} "${MY_FILE}-Sources.lst"   | ${MY_SED} -e 's/<!-- sortkey: [^>]*>/          /' >> "${MY_FILE}"
        echo '        </Folder>' >> "${MY_FILE}"
    fi
    if test -s "${MY_FILE}-Headers.lst";
    then
        echo '        <Folder Name="Headers"  Filters="*.h;*.hpp">' >> "${MY_FILE}"
        ${MY_SORT} "${MY_FILE}-Headers.lst"   | ${MY_SED} -e 's/<!-- sortkey: [^>]*>/          /' >> "${MY_FILE}"
        echo '        </Folder>' >> "${MY_FILE}"
    fi
    if test -s "${MY_FILE}-Assembly.lst";
    then
        echo '        <Folder Name="Assembly" Filters="*.asm;*.s;*.S;*.inc;*.mac">' >> "${MY_FILE}"
        ${MY_SORT} "${MY_FILE}-Assembly.lst"  | ${MY_SED} -e 's/<!-- sortkey: [^>]*>/          /' >> "${MY_FILE}"
        echo '        </Folder>' >> "${MY_FILE}"
    fi
    if test -s "${MY_FILE}-Testcases.lst";
    then
        echo '        <Folder Name="Testcases" Filters="tst*;">' >> "${MY_FILE}"
        ${MY_SORT} "${MY_FILE}-Testcases.lst" | ${MY_SED} -e 's/<!-- sortkey: [^>]*>/          /' >> "${MY_FILE}"
        echo '        </Folder>' >> "${MY_FILE}"
    fi
    if test -s "${MY_FILE}-Others.lst";
    then
        echo '        <Folder Name="Others"   Filters="">' >> "${MY_FILE}"
        ${MY_SORT} "${MY_FILE}-Others.lst"    | ${MY_SED} -e 's/<!-- sortkey: [^>]*>/          /' >> "${MY_FILE}"
        echo '        </Folder>' >> "${MY_FILE}"
    fi

    # Cleanup
    ${MY_RM}  "${MY_FILE}-Sources.lst" "${MY_FILE}-Headers.lst" "${MY_FILE}-Assembly.lst" "${MY_FILE}-Testcases.lst" "${MY_FILE}-Others.lst"
}


##
# Function generating a project.
#
# @param    $1      The project file name.
# @param    $2      The project working directory.
# @param    $3      Dummy separator.
# @param    $4+     Include directories.
# @param    $N      --end-includes
# @param    $N+1    Directory sub-trees and files to include in the project.
#
my_generate_project()
{
    MY_FILE="${MY_PRJ_PRF}${1}.vpj";
    echo "Generating ${MY_FILE}..."
    MY_WRK_DIR="${2}"
    shift
    shift
    shift

    # Add it to the project list for workspace construction later on.
    MY_PROJECT_FILES="${MY_PROJECT_FILES} ${MY_FILE}"


    #
    # Generate the head bits.
    #
    echo '<!DOCTYPE Project SYSTEM "http://www.slickedit.com/dtd/vse/10.0/vpj.dtd">'                                    >  "${MY_FILE}"
    echo '<Project'                                                                                                     >> "${MY_FILE}"
    echo '        Version="10.0"'                                                                                       >> "${MY_FILE}"
    echo '        VendorName="SlickEdit"'                                                                               >> "${MY_FILE}"
    echo '        VCSProject="Subversion:"'                                                                             >> "${MY_FILE}"
#    echo '        Customized="1"'                                                                                       >> "${MY_FILE}"
#    echo '        WorkingDir="."'                                                                                       >> "${MY_FILE}"
    my_abs_dir "${MY_WRK_DIR}"                                                                                          >> "${MY_FILE}"
    echo '        WorkingDir="'"${MY_ABS_DIR}"'"'                                                                       >> "${MY_FILE}"
    echo '        >'                                                                                                    >> "${MY_FILE}"
    echo '    <Config Name="Release" OutputFile="" CompilerConfigName="Latest Version">'                                >> "${MY_FILE}"
    echo '        <Menu>'                                                                                               >> "${MY_FILE}"
    echo '            <Target Name="Compile" MenuCaption="&amp;Compile" CaptureOutputWith="ProcessBuffer"'              >> "${MY_FILE}"
    echo '                    SaveOption="SaveCurrent" RunFromDir="%p" ClearProcessBuffer="1">'                         >> "${MY_FILE}"
    echo '                <Exec CmdLine="'"${MY_KMK_INVOCATION}"' -C %p %n.o"/>'                                        >> "${MY_FILE}"
    echo '            </Target>'                                                                                        >> "${MY_FILE}"
    echo '            <Target Name="Build" MenuCaption="&amp;Build"CaptureOutputWith="ProcessBuffer"'                   >> "${MY_FILE}"
    echo '                    SaveOption="SaveWorkspaceFiles" RunFromDir="%rw" ClearProcessBuffer="1">'                 >> "${MY_FILE}"
    echo '                <Exec CmdLine="'"${MY_KMK_INVOCATION}"'"/>'                                                   >> "${MY_FILE}"
    echo '            </Target>'                                                                                        >> "${MY_FILE}"
    echo '            <Target Name="Rebuild" MenuCaption="&amp;Rebuild" CaptureOutputWith="ProcessBuffer"'              >> "${MY_FILE}"
    echo '                    SaveOption="SaveWorkspaceFiles" RunFromDir="%rw" ClearProcessBuffer="1">'                 >> "${MY_FILE}"
    echo '                <Exec CmdLine="'"${MY_KMK_INVOCATION}"' rebuild"/>'                                           >> "${MY_FILE}"
    echo '            </Target>'                                                                                        >> "${MY_FILE}"
    echo '            <Target Name="Debug" MenuCaption="&amp;Debug" SaveOption="SaveNone" RunFromDir="%rw">'            >> "${MY_FILE}"
    echo '                <Exec/>'                                                                                      >> "${MY_FILE}"
    echo '            </Target>'                                                                                        >> "${MY_FILE}"
    echo '            <Target Name="Execute" MenuCaption="E&amp;xecute" SaveOption="SaveNone" RunFromDir="%rw">'        >> "${MY_FILE}"
    echo '                <Exec/>'                                                                                      >> "${MY_FILE}"
    echo '            </Target>'                                                                                        >> "${MY_FILE}"
    echo '        </Menu>'                                                                                              >> "${MY_FILE}"

    #
    # Include directories.
    #
    echo '        <Includes>'                                                                                           >> "${MY_FILE}"
    while test $# -ge 1  -a  "${1}" != "--end-includes";
    do
        for f in $1;
        do
            my_abs_dir ${f}
            echo '            <Include Dir="'"${MY_ABS_DIR}/"'"/>'                                                      >> "${MY_FILE}"
        done
        shift
    done
    shift
    echo '        </Includes>'                                                                                          >> "${MY_FILE}"
    echo '    </Config>'                                                                                                >> "${MY_FILE}"


    #
    # Process directories+files and create folders.
    #
    echo '    <Files>'                                                                                                  >> "${MY_FILE}"
    my_generate_folder "${MY_FILE}" $*
    echo '    </Files>'                                                                                                 >> "${MY_FILE}"

    #
    # The tail.
    #
    echo '</Project>'                                                                                                   >> "${MY_FILE}"

    return 0
}


##
# Generate the workspace
#
my_generate_workspace()
{
    MY_FILE="${MY_WS_NAME}"
    echo "Generating ${MY_FILE}..."
    echo '<!DOCTYPE Workspace SYSTEM "http://www.slickedit.com/dtd/vse/10.0/vpw.dtd">'   > "${MY_FILE}"
    echo '<Workspace Version="10.0" VendorName="SlickEdit">'                            >> "${MY_FILE}"
    echo '    <Projects>'                                                               >> "${MY_FILE}"
    for i in ${MY_PROJECT_FILES};
    do
        echo '        <Project File="'"${i}"'" />'                                      >> "${MY_FILE}"
    done
    echo '    </Projects>'                                                              >> "${MY_FILE}"
    echo '</Workspace>'                                                                 >> "${MY_FILE}"
    return 0;
}


###### end of functions ####


#
# Parse arguments.
#
while test $# -ge 1;
do
    ARG=$1
    shift
    case "$ARG" in

        --rootdir)
            if test $# -eq 0; then
                echo "error: missing --rootdir argument." 1>&2
                exit 1;
            fi
            MY_ROOT_DIR="$1"
            shift
            ;;

        --outdir)
            if test $# -eq 0; then
                echo "error: missing --outdir argument." 1>&2
                exit 1;
            fi
            MY_OUT_DIR="$1"
            shift
            ;;

        --project-base)
            if test $# -eq 0; then
                echo "error: missing --project-base argument." 1>&2
                exit 1;
            fi
            MY_PRJ_PRF="$1"
            shift
            ;;

        --workspace)
            if test $# -eq 0; then
                echo "error: missing --workspace argument." 1>&2
                exit 1;
            fi
            MY_WS_NAME="$1"
            shift
            ;;

        --windows-host)
            MY_WINDOWS_HOST=1
            ;;

        --minimal)
            MY_OPT_MINIMAL=1
            ;;

        # usage
        --h*|-h*|-?|--?)
            echo "usage: $0 [--rootdir <rootdir>] [--outdir <outdir>] [--project-base <prefix>] [--workspace <name>] [--minimal]"
            echo ""
            echo "If --outdir is specified, you must specify a --rootdir relative to it as well."
            exit 1;
            ;;

        # default
        *)
            echo "error: Invalid parameter '$ARG'" 1>&2
            exit 1;

    esac
done


#
# From now on everything *MUST* succeed.
#
set -e


#
# Make sure the output directory exists, is valid and clean.
#
${MY_RM} -f \
    "${MY_OUT_DIR}/${MY_PRJ_PRF}"*.vpj \
    "${MY_OUT_DIR}/${MY_WS_NAME}" \
    "${MY_OUT_DIR}/`echo ${MY_WS_NAME} | ${MY_SED} -e 's/\.vpw$/.vtg/'`"
${MY_MKDIR} -p "${MY_OUT_DIR}"
cd "${MY_OUT_DIR}"


#
# Determine the invocation to conjure up kmk.
#
my_abs_dir "tools"
if test -n "${MY_WINDOWS_HOST}"; then
    MY_KMK_INVOCATION="${MY_ABS_DIR}/win.x86/bin/rexx.exe ${MY_ABS_DIR}/envSub.cmd kmk.exe"
else
    MY_KMK_INVOCATION="${MY_ABS_DIR}/env.sh --quiet kmk"
fi


#
# Generate the projects and workspace.
#
# Note! The configs aren't optimal yet, lots of adjustment wrt headers left to be done.
#

# src/VBox/Runtime
my_generate_project "IPRT"          "src/VBox/Runtime"                      --begin-incs "include" "src/VBox/Runtime/include"               --end-includes "include/iprt" "src/VBox/Runtime"

# src/VBox/VMM
my_generate_project "VMM"           "src/VBox/VMM"                          --begin-incs "include" "src/VBox/VMM"                           --end-includes "src/VBox/VMM" \
    "include/VBox/vmm/cfgm.h" \
    "include/VBox/vmm/cpum.*" \
    "include/VBox/vmm/dbgf.h" \
    "include/VBox/vmm/em.h" \
    "include/VBox/vmm/gmm.*" \
    "include/VBox/vmm/gvm.*" \
    "include/VBox/vmm/hw*.*" \
    "include/VBox/vmm/iom.h" \
    "include/VBox/vmm/mm.h" \
    "include/VBox/vmm/patm.*" \
    "include/VBox/vmm/pdm*.h" \
    "include/VBox/vmm/pgm.*" \
    "include/VBox/vmm/rem.h" \
    "include/VBox/vmm/selm.*" \
    "include/VBox/vmm/ssm.h" \
    "include/VBox/vmm/stam.*" \
    "include/VBox/vmm/tm.h" \
    "include/VBox/vmm/trpm.*" \
    "include/VBox/vmm/vm.*" \
    "include/VBox/vmm/vmm.*"

# src/recompiler
my_generate_project "REM"           "src/recompiler"                        --begin-incs \
    "include" \
    "src/recompiler" \
    "src/recompiler/target-i386" \
    "src/recompiler/tcg/i386" \
    "src/recompiler/Sun/crt" \
    --end-includes \
    "src/recompiler" \
    "src/VBox/VMM/REMInternal.h" \
    "src/VBox/VMM/VMMAll/REMAll.cpp"

# src/VBox/Additions
my_generate_project "Add-freebsd"   "src/VBox/Additions/freebsd"            --begin-incs "include" "src/VBox/Additions/freebsd"             --end-includes "src/VBox/Additions/freebsd"
my_generate_project "Add-linux"     "src/VBox/Additions/linux"              --begin-incs "include" "src/VBox/Additions/linux"               --end-includes "src/VBox/Additions/linux"
my_generate_project "Add-os2"       "src/VBox/Additions/os2"                --begin-incs "include" "src/VBox/Additions/os2"                 --end-includes "src/VBox/Additions/os2"
my_generate_project "Add-solaris"   "src/VBox/Additions/solaris"            --begin-incs "include" "src/VBox/Additions/solaris"             --end-includes "src/VBox/Additions/solaris"
my_generate_project "Add-win"       "src/VBox/Additions/WINNT"              --begin-incs "include" "src/VBox/Additions/WINNT"               --end-includes "src/VBox/Additions/WINNT"
test -z "$MY_OPT_MINIMAL" && \
my_generate_project "Add-x11"       "src/VBox/Additions/x11"                --begin-incs "include" "src/VBox/Additions/x11"                 --end-includes "src/VBox/Additions/x11"
my_generate_project "Add-Control"   "src/VBox/Additions/common/VBoxControl" --begin-incs "include" "src/VBox/Additions/common/VBoxControl"  --end-includes "src/VBox/Additions/common/VBoxControl"
my_generate_project "Add-GuestDrv"  "src/VBox/Additions/common/VBoxGuest"   --begin-incs "include" "src/VBox/Additions/common/VBoxGuest"    --end-includes "src/VBox/Additions/common/VBoxGuest"    "include/VBox/VBoxGuest*.*"
my_generate_project "Add-Lib"      "src/VBox/Additions/common/VBoxGuestLib" --begin-incs "include" "src/VBox/Additions/common/VBoxGuestLib" --end-includes "src/VBox/Additions/common/VBoxGuestLib" "include/VBox/VBoxGuest*.*"
my_generate_project "Add-Service"   "src/VBox/Additions/common/VBoxService" --begin-incs "include" "src/VBox/Additions/common/VBoxService"  --end-includes "src/VBox/Additions/common/VBoxService"
test -z "$MY_OPT_MINIMAL" && \
my_generate_project "Add-crOpenGL"  "src/VBox/Additions/common/crOpenGL"    --begin-incs "include" "src/VBox/Additions/common/crOpenGL"     --end-includes "src/VBox/Additions/common/crOpenGL"

# src/VBox/Debugger
my_generate_project "Debugger"      "src/VBox/Debugger"                     --begin-incs "include" "src/VBox/Debugger"                      --end-includes "src/VBox/Debugger" "include/VBox/dbggui.h" "include/VBox/dbg.h"

# src/VBox/Devices
my_generate_project "Devices"       "src/VBox/Devices"                      --begin-incs "include" "src/VBox/Devices"                       --end-includes "src/VBox/Devices" "include/VBox/pci.h" "include/VBox/pdm*.h"
## @todo split this up.

# src/VBox/Disassembler
my_generate_project "DIS"           "src/VBox/Disassembler"                 --begin-incs "include" "src/VBox/Disassembler"                  --end-includes "src/VBox/Disassembler" "include/VBox/dis*.h"

# src/VBox/Frontends
my_generate_project "FE-VBoxManage" "src/VBox/Frontends/VBoxManage"         --begin-incs "include" "src/VBox/Frontends/VBoxManage"          --end-includes "src/VBox/Frontends/VBoxManage"
my_generate_project "FE-VBoxHeadless" "src/VBox/Frontends/VBoxHeadless"     --begin-incs "include" "src/VBox/Frontends/VBoxHeadless"        --end-includes "src/VBox/Frontends/VBoxHeadless"
my_generate_project "FE-VBoxSDL"    "src/VBox/Frontends/VBoxSDL"            --begin-incs "include" "src/VBox/Frontends/VBoxSDL"             --end-includes "src/VBox/Frontends/VBoxSDL"
my_generate_project "FE-VBoxShell"  "src/VBox/Frontends/VBoxShell"          --begin-incs "include" "src/VBox/Frontends/VBoxShell"           --end-includes "src/VBox/Frontends/VBoxShell"
# noise - my_generate_project "FE-VBoxBFE"      "src/VBox/Frontends/VBoxBFE"      --begin-incs "include" "src/VBox/Frontends/VBoxBFE"            --end-includes "src/VBox/Frontends/VBoxBFE"
FE_VBOX_WRAPPERS=""
for d in ${MY_OUT_DIRS};
do
    if test -d "${MY_ROOT_DIR}/${d}/obj/VirtualBox/include"; then
        FE_VBOX_WRAPPERS="${d}/obj/VirtualBox/include"
        break
    fi
done
if test -n "${FE_VBOX_WRAPPERS}"; then
    my_generate_project "FE-VirtualBox" "src/VBox/Frontends/VirtualBox"     --begin-incs "include" "${FE_VBOX_WRAPPERS}"                    --end-includes "src/VBox/Frontends/VirtualBox" "${FE_VBOX_WRAPPERS}/COMWrappers.cpp" "${FE_VBOX_WRAPPERS}/COMWrappers.h"
else
    my_generate_project "FE-VirtualBox" "src/VBox/Frontends/VirtualBox"     --begin-incs "include"                                          --end-includes "src/VBox/Frontends/VirtualBox"
fi

# src/VBox/GuestHost
my_generate_project "HGSMI-GH"      "src/VBox/GuestHost/HGSMI"              --begin-incs "include"                                          --end-includes "src/VBox/GuestHost/HGSMI"
test -z "$MY_OPT_MINIMAL" && \
my_generate_project "OpenGL-GH"     "src/VBox/GuestHost/OpenGL"             --begin-incs "include" "src/VBox/GuestHost/OpenGL"              --end-includes "src/VBox/GuestHost/OpenGL"
my_generate_project "ShClip-GH"     "src/VBox/GuestHost/SharedClipboard"    --begin-incs "include"                                          --end-includes "src/VBox/GuestHost/SharedClipboard"

# src/VBox/HostDrivers
my_generate_project "SUP"           "src/VBox/HostDrivers/Support"          --begin-incs "include" "src/VBox/HostDrivers/Support"           --end-includes "src/VBox/HostDrivers/Support" "include/VBox/sup.h" "include/VBox/sup.mac"
my_generate_project "VBoxNetAdp"    "src/VBox/HostDrivers/VBoxNetAdp"       --begin-incs "include" "src/VBox/HostDrivers/VBoxNetAdp"        --end-includes "src/VBox/HostDrivers/VBoxNetAdp" "include/VBox/intnet.h"
my_generate_project "VBoxNetFlt"    "src/VBox/HostDrivers/VBoxNetFlt"       --begin-incs "include" "src/VBox/HostDrivers/VBoxNetFlt"        --end-includes "src/VBox/HostDrivers/VBoxNetFlt" "include/VBox/intnet.h"
my_generate_project "VBoxUSB"       "src/VBox/HostDrivers/VBoxUSB"          --begin-incs "include" "src/VBox/HostDrivers/VBoxUSB"           --end-includes "src/VBox/HostDrivers/VBoxUSB" "include/VBox/usblib*.h" "include/VBox/usbfilter.h"

# src/VBox/HostServices
my_generate_project "GuestProps"    "src/VBox/HostServices/GuestProperties" --begin-incs "include" "src/VBox/HostServices/GuestProperties"  --end-includes "src/VBox/HostServices/GuestProperties"
my_generate_project "ShClip-HS"     "src/VBox/HostServices/SharedClipboard" --begin-incs "include" "src/VBox/HostServices/SharedClipboard"  --end-includes "src/VBox/HostServices/SharedClipboard"
my_generate_project "SharedFolders" "src/VBox/HostServices/SharedFolders"   --begin-incs "include" "src/VBox/HostServices/SharedFolders"    --end-includes "src/VBox/HostServices/SharedFolders" "include/VBox/shflsvc.h"
my_generate_project "OpenGL-HS"     "src/VBox/HostServices/SharedOpenGL"    --begin-incs "include" "src/VBox/HostServices/SharedOpenGL"     --end-includes "src/VBox/HostServices/SharedOpenGL"

# src/VBox/ImageMounter
my_generate_project "ImageMounter"  "src/VBox/ImageMounter"                 --begin-incs "include" "src/VBox/ImageMounter"                  --end-includes "src/VBox/ImageMounter"

# src/VBox/Installer
my_generate_project "Installers"    "src/VBox/Installer"                    --begin-incs "include"                                          --end-includes "src/VBox/Installer"

# src/VBox/Main
my_generate_project "Main"          "src/VBox/Main"                         --begin-incs "include" "src/VBox/Main/include"                  --end-includes "src/VBox/Main" "include/VBox/com" "include/VBox/settings.h"
## @todo seperate webservices and Main. pick the right headers. added generated headers.

# src/VBox/Network
my_generate_project "Net-DHCP"      "src/VBox/NetworkServices/DHCP"         --begin-incs "include" "src/VBox/NetworkServices/NetLib"        --end-includes "src/VBox/NetworkServices/DHCP"
my_generate_project "Net-NAT"       "src/VBox/NetworkServices/NAT"          --begin-incs "include" "src/VBox/NetworkServices/NAT"           --end-includes "src/VBox/NetworkServices/NAT" "src/VBox/Devices/Network/slirp"
my_generate_project "Net-NetLib"    "src/VBox/NetworkServices/NetLib"       --begin-incs "include" "src/VBox/NetworkServices/NetLib"        --end-includes "src/VBox/NetworkServices/NetLib"

# src/VBox/RDP
my_generate_project "RDP-Client"    "src/VBox/RDP/client"                   --begin-incs "include" "src/VBox/RDP/client"                    --end-includes "src/VBox/RDP/client"
my_generate_project "RDP-Server"    "src/VBox/RDP/server"                   --begin-incs "include" "src/VBox/RDP/server"                    --end-includes "src/VBox/RDP/server"
my_generate_project "RDP-WebClient" "src/VBox/RDP/webclient"                --begin-incs "include" "src/VBox/RDP/webclient"                 --end-includes "src/VBox/RDP/webclient"
my_generate_project "RDP-Misc"      "src/VBox/RDP"                          --begin-incs "include"                                          --end-includes "src/VBox/RDP/auth" "src/VBox/RDP/tscpasswd" "src/VBox/RDP/x11server"

# src/VBox/Testsuite
my_generate_project "Testsuite"     "src/VBox/Testsuite"                    --begin-incs "include"                                          --end-includes "src/VBox/Testsuite"

# src/apps/adpctl - misplaced.
my_generate_project "adpctl"        "src/apps/adpctl"                       --begin-incs "include"                                          --end-includes "src/apps/adpctl"

# src/bldprogs
my_generate_project "bldprogs"      "src/bldprogs"                          --begin-incs "include"                                          --end-includes "src/bldprogs"

# A few things from src/lib
my_generate_project "zlib"          "src/libs/zlib-1.2.1"                   --begin-incs "include"                                          --end-includes "src/libs/zlib-1.2.1/*.c" "src/libs/zlib-1.2.1/*.h"
my_generate_project "liblzf"        "src/libs/liblzf-3.4"                   --begin-incs "include"                                          --end-includes "src/libs/liblzf-3.4"
my_generate_project "libpng"        "src/libs/libpng-1.2.8"                 --begin-incs "include"                                          --end-includes "src/libs/libpng-1.2.8/*.c" "src/libs/libpng-1.2.8/*.h"
my_generate_project "openssl"       "src/libs/openssl-0.9.8p"               --begin-incs "include" "src/libs/openssl-0.9.8p/crypto"         --end-includes "src/libs/openssl-0.9.8p"


# include/VBox
my_generate_project "VBoxHeaders"   "include"                               --begin-incs "include"                                          --end-includes "include/VBox"

# Misc
my_generate_project "misc"          "src/testcase"                          --begin-incs "include"                                          --end-includes \
    "src/testcase" \
    "configure" \
    "configure.vbs" \
    "Config.kmk" \
    "Makefile.kmk" \
    "src/Makefile.kmk" \
    "src/VBox/Makefile.kmk"


# out/x.y/z/bin/sdk/bindings/xpcom
XPCOM_INCS="src/libs/xpcom18a4"
for d in \
    "out/${KBUILD_TARGET}.${KBUILD_TARGET_ARCH}/${KBUILD_TYPE}/dist/sdk/bindings/xpcom" \
    "out/${BUILD_TARGET}.${BUILD_TARGET_ARCH}/${BUILD_TYPE}/dist/sdk/bindings/xpcom" \
    "out/${KBUILD_TARGET}.${KBUILD_TARGET_ARCH}/${KBUILD_TYPE}/bin/sdk/bindings/xpcom" \
    "out/${BUILD_TARGET}.${BUILD_TARGET_ARCH}/${BUILD_TYPE}/bin/sdk/bindings/xpcom" \
    "out/linux.amd64/debug/bin/sdk/bindings/xpcom" \
    "out/linux.x86/debug/bin/sdk/bindings/xpcom" \
    "out/darwin.amd64/debug/dist/sdk/bindings/xpcom" \
    "out/darwin.x86/debug/bin/dist/bindings/xpcom" \
    "out/solaris.amd64/debug/bin/sdk/bindings/xpcom" \
    "out/solaris.x86/debug/bin/sdk/bindings/xpcom";
do
    if test -d "${MY_ROOT_DIR}/${d}"; then
        my_generate_project "SDK-xpcom" "${d}"  --begin-incs "include" "${d}/include" --end-includes "${d}"
        XPCOM_INCS="${d}/include"
        break
    fi
done

# lib/xpcom
my_generate_project "xpcom"         "src/libs/xpcom18a4"                    --begin-incs "include" "${XPCOM_INCS}"                          --end-includes "src/libs/xpcom18a4"

my_generate_workspace


#
# Update the history file if present.
#
MY_FILE="${MY_WS_NAME}histu"
if test -f "${MY_FILE}"; then
    echo "Updating ${MY_FILE}..."
    ${MY_MV} -f "${MY_FILE}" "${MY_FILE}.old"
    ${MY_SED} -n \
        -e '/PROJECT_CACHE/d' \
        -e '/\[TreeExpansion2\]/d' \
        -e '/^\[/p' \
        -e '/: /p' \
        -e '/^CurrentProject/p' \
        "${MY_FILE}.old" > "${MY_FILE}"
fi

echo "done"
