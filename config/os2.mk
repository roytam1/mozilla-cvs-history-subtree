#
# The contents of this file are subject to the Netscape Public License
# Version 1.0 (the "NPL"); you may not use this file except in
# compliance with the NPL.  You may obtain a copy of the NPL at
# http://www.mozilla.org/NPL/
#
# Software distributed under the NPL is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
# for the specific language governing rights and limitations under the
# NPL.
#
# The Initial Developer of this code under the NPL is Netscape
# Communications Corporation.  Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation.  All Rights
# Reserved.
#

######################################################################
# Config stuff for OS/2
######################################################################
SHELL	= GBASH.EXE

# Specify toolset.  Default to EMX.
ifeq ($(MOZ_OS2_TOOLS),VACPP)
XP_OS2_VACPP = 1
else
XP_OS2_EMX   = 1
endif

ifdef XP_OS2_EMX
######################################################################
# These are for emx/gcc
######################################################################

PLATFORM_FLAGS	= -ansi -Wall -Zmtd -DXP_OS2 -DXP_OS2_FIX -DXP_OS2_EMX
MOVEMAIL_FLAGS	=
PORT_FLAGS		= -DNEED_GETOPT_H -DHAVE_SIGNED_CHAR

OS_CFLAGS		= $(PLATFORM_FLAGS) $(PORT_FLAGS) $(MOVEMAIL_FLAGS)

######################################################################
# end XP_OS2_EMX
######################################################################
endif

ifdef XP_OS2_VACPP
######################################################################
# These are for VisualAge C++
######################################################################
CC				= icc
LINK			= ilink

ifdef BUILD_OPT
OPTIMIZER	= -O+ -Oi
else
OPTIMIZER   = -Ti+
endif

PLATFORM_FLAGS	= -q -W3 -DOS2::4 -DXP_OS2 -DXP_OS2_FIX -DXP_OS2_VACPP -N10 -D_X86_
PLATFORM_FLAGS += -Gm -Gd+ -Su4 -Ss -I. -I$(DEPTH)/config/os2
MOVEMAIL_FLAGS	=
PORT_FLAGS		= -DHAVE_SIGNED_CHAR

OS_CFLAGS		= $(PLATFORM_FLAGS) $(PORT_FLAGS) $(MOVEMAIL_FLAGS)

OS_LFLAGS       = /PM:VIO /NOLOGO

######################################################################
# End XP_OS2_VACPP
######################################################################
endif
