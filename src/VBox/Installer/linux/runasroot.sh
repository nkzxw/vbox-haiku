#!/bin/sh
# $Id$
## @file
# VirtualBox privileged execution helper script for Linux and Solaris
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

#include sh-utils.sh

ostype=`uname -s`
if test "$ostype" != "Linux" && test "$ostype" != "SunOS" ; then
  echo "Linux/Solaris not detected."
  exit 1
fi

case "$#" in "2"|"3")
    ;;
    *)
    echo "Usage: `basename $0` DESCRIPTION COMMAND [ADVICE]" >&2
    echo >&2
    echo "Attempt to execute COMMAND with root privileges, displaying DESCRIPTION if" >&2
    echo "possible and displaying ADVICE if possible if no su(1)-like tool is available." >&2
    exit 1
    ;;
esac

DESCRIPTION=$1
COMMAND=$2
ADVICE=$3
PATH=$PATH:/bin:/usr/bin:/usr/local/bin:/sbin:/usr/sbin:/usr/local/sbin:/usr/X11/bin

case "$ostype" in SunOS)
    PATH=$PATH:/usr/sfw/bin:/usr/gnu/bin:/usr/xpg4/bin:/usr/xpg6/bin:/usr/openwin/bin:/usr/ucb
    GKSU_SWITCHES="-au root"
    ;;
    *)
    GKSU_SWITCHES=""
    ;;
esac

case "$DISPLAY" in ?*)
    KDESUDO="`mywhich kdesudo`"
    case "$KDESUDO" in ?*)
        eval "`quotify "$KDESUDO"` --comment `quotify "$DESCRIPTION"` -- $COMMAND"
        exit
        ;;
    esac

    KDESU="`mywhich kdesu`"
    case "$KDESU" in ?*)
        "$KDESU" -c "$COMMAND"
        exit
        ;;
    esac

    GKSU="`mywhich gksu`"
    case "$GKSU" in ?*)
        # Older gksu does not grok --description nor '--' and multiple args.
        # @todo which versions do?
        # "$GKSU" --description "$DESCRIPTION" -- "$@"
        # Note that $GKSU_SWITCHES is NOT quoted in the following
        "$GKSU" $GKSU_SWITCHES "$COMMAND"
        exit
        ;;
    esac
    ;;
esac # $DISPLAY

# pkexec may work for ssh console sessions as well if the right agents
# are installed.  However it is very generic and does not allow for any
# custom messages.  Thus it comes after gksu.
PKEXEC="`mywhich pkexec`"
case "$PKEXEC" in ?*)
    eval "\"$PKEXEC\" $COMMAND"
    exit
    ;;
esac

# The ultimate fallback is running 'su -' within an xterm.  We use the
# title of the xterm to tell what is going on.
case "$DISPLAY" in ?*)
    SU="`mywhich su`"
    case "$SU" in ?*)
        getxterm
        case "$gxtpath" in ?*)
            "$gxtpath" "$gxttitle" "$DESCRIPTION - su" "$gxtexec" su - root -c "$COMMAND"
            exit
            ;;
        esac
    esac
esac # $DISPLAY

# Failure...
case "$DISPLAY" in ?*)
    echo "Unable to locate 'pkexec', 'gksu' or 'su+xterm'. $ADVICE" >&2
    ;;
    *)
    echo "Unable to locate 'pkexec'. $ADVICE" >&2
    ;;
esac

exit 1
