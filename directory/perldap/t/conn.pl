#!/usr/bin/perl5
#############################################################################
# $Id$
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
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
#
# DESCRIPTION
#    Test most (all?) of the main OO layers, Entry.pm and Conn.pm.
#
#############################################################################

use Getopt::Std;			# To parse command line arguments.
use Mozilla::LDAP::Conn;		# Main "OO" layer for LDAP
use Mozilla::LDAP::Utils;		# LULU, utilities.

use strict;
no strict "vars";


#################################################################################
# Configurations, modify these as needed.
#
$BASE	= "o=Netscape Communications Corp.,c=US";
$PEOPLE	= "ou=people";
$GROUPS	= "ou=groups";
$USR	= "uid=leif";
$GRP	= "cn=test-group1";


#################################################################################
# Constants, shouldn't have to edit these...
#
$APPNAM	= "test1";
$USAGE	= "$APPNAM -b base -h host -D bind -w pswd -P cert";


#################################################################################
# Check arguments, and configure some parameters accordingly..
#
if (!getopts('b:h:D:p:s:w:P:'))
{
   print "usage: $APPNAM $USAGE\n";
   exit;
}
%ld = Mozilla::LDAP::Utils::ldapArgs();


#################################################################################
# Get an LDAP connection
#
sub getConn
{
  my $conn;

  if ($main::reuseConn)
    {
      if (!defined($main::mainConn))
	{
	  $main::mainConn = new Mozilla::LDAP::Conn(\%main::ld);
	  die "Could't connect to LDAP server $main::ld{host}"
	    unless $main::mainConn;
	}
      return $main::mainConn;
    }
  else
    {
      $conn = new Mozilla::LDAP::Conn(\%main::ld);
      die "Could't connect to LDAP server $main::ld{host}" unless $conn;

  return $conn;
}


