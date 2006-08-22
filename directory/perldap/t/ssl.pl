#!/usr/bin/env perl
#############################################################################
# $Id$
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.1 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
# License for the specific language governing rights and limitations
# under the License.
#
# The Original Code is PerLDAP. The Initial Developer of the Original
# Code is Netscape Communications Corp. and Clayton Donley. Portions
# created by Netscape are Copyright (C) Netscape Communications
# Corp., portions created by Clayton Donley are Copyright (C) Clayton
# Donley. All Rights Reserved.
#
# Contributor(s):
#    Rich Megginson <richm@stanfordalumni.org>
#
# DESCRIPTION
#    Test most (all?) of the LDAP::Mozilla::Conn methods. This code
#    needs to be rewritten to use the standard test harness in Perl...
#
#############################################################################

use Getopt::Std;			# To parse command line arguments.
use Mozilla::LDAP::Conn;		# Main "OO" layer for LDAP
use Mozilla::LDAP::Utils;		# LULU, utilities.
use Mozilla::LDAP::API;

use strict;
no strict "vars";


# Uncomment for somewhat more verbose messages from core modules
#$LDAP_DEBUG = 1;


#################################################################################
# Configurations, modify these as needed.
#
$BASE	= "dc=example,dc=com";
$PEOPLE	= "ou=people";
$UID	= "scarter";


#################################################################################
# Constants, shouldn't have to edit these...
#
$APPNAM	= "ssl.pl";
$USAGE	= "$APPNAM -b base -h host -D bind -w pswd -P cert -N certname -W keypassword -Z";


#################################################################################
# Check arguments, and configure some parameters accordingly..
#
if (!getopts('b:h:D:p:s:w:P:N:W:Z'))
{
   print "usage: $APPNAM $USAGE\n";
   exit;
}
%ld = Mozilla::LDAP::Utils::ldapArgs(undef, $BASE);
$BASE = $ld{"base"};
Mozilla::LDAP::Utils::userCredentials(\%ld) unless $opt_n;
