#
# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is Netscape
# Communications Corporation.  Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): 
#
#
# Even though we use AUTOCONF, there are just too many things that need
# fixing up to do it any other way than via an architecture specific file.
#
# If we're not using NSBUILDROOT, then make sure we use multiple object
# directories. We want this name to be relatively short, and to be different
# from what NSPR uses (so that we can wipe out Mozilla objects without
# wiping NSPR objects.

# Tru64 UNIX (dx)ladebug debugger needs -gall to display opaque types in C++
OS_CXXFLAGS	+= -gall

# Needed to locate include files in xptcall assembly and get cpp to expand Stub##n##__ macro
ASFLAGS	= -I$(topsrcdir)/xpcom/reflect/xptcall/public -g -std1 

# Tru64 UNIX assembler requires -c and uses lower case .s file extension
AS_DASH_C_FLAG	= -c
