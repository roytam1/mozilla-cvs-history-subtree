#!/usr/bin/perl5
#############################################################################
# $Id$
#
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is PerLDAP.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corp.
# Portions created by the Initial Developer are Copyright (C) 2001
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Clayton Donley
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

# DESCRIPTION
#    Search the changelog, and produce an LDIF file suitable for ldapmodify
#    for instance. This should be merged into LDIF.pm eventually.

use Getopt::Std;			# To parse command line arguments.
use Mozilla::LDAP::Conn;		# Main "OO" layer for LDAP
use Mozilla::LDAP::Utils;		# LULU, utilities.

use strict;
no strict "vars";


#################################################################################
# Constants, shouldn't have to edit these...
#
$APPNAM	= "changes2ldif";
$USAGE	= "$APPNAM [-nv] -b base -h host -D bind -w pswd -P cert [min [max]]";


#################################################################################
# Check arguments, and configure some parameters accordingly..
#
if (!getopts('nvb:h:D:p:s:w:P:'))
{
   print "usage: $APPNAM $USAGE\n";
   exit;
}
%ld = Mozilla::LDAP::Utils::ldapArgs();
$ld{root} = "cn=changelog" if (!defined($ld{root}) || $ld{root} eq "");


#################################################################################
# Instantiate an LDAP object, which also binds to the LDAP server.
#
$conn = new Mozilla::LDAP::Conn(\%ld);
die "Could't connect to LDAP server $ld{host}" unless $conn;


#################################################################################
# Create the search filter.
#
$min = $ARGV[$[];
$max = $ARGV[$[ + 1];

if ($min ne "")
{
  if ($max ne "")
    {
      $search = "(&(changenumber>=$min)(changenumber<=$max))";
    }
  else
    {
      $search = "(changenumber>=$min)";
    }
}
else
{
  $search = "(changenumber=*)";
}
  

#################################################################################
# Do the searches, and print the results.
#
$entry = $conn->search($ld{root}, "ONE", "$search");
while ($entry)
{
  print "dn: ", $entry->{targetdn}[0], "\n";
  $type = $entry->{changetype}[0];
  print "changetype: $type\n";
  if ($type =~ /modify/i)
    {
      # Should we filter out modifiersname and modifytimestamp ? We do chop
      # off the trailing \0 though.
      chop($entry->{changes}[0]);
      print $entry->{changes}[0], "\n";
    }
  elsif ($type =~ /add/i)
    {
      print $entry->{changes}[0], "\n";
    }
  else
    {
      print "\n";
    }

  $entry = $conn->nextEntry;
}


#################################################################################
# Close the connection.
#
$ld{conn}->close if $ld{conn};
