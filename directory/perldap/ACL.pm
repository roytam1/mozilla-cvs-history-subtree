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
# Code is Leif Hedstrom and Netscape Communications. Portions created
# by Leif are Copyright (C) Leif Hedstrom, portions created by Netscape
# are Copyright (C) Netscape Communications Corp. All Rights Reserved.
#
# Contributor(s):	Michelle Wyner <mwyner@perldap.org>
#
# DESCRIPTION
#    This is an object class to parse, manage and generate ACL
#    definitions for an LDAP server. Currently we only support Netscape
#    and Sun/Alliance LDAP servers.
#
#############################################################################

package Mozilla::LDAP::ACL;

use Mozilla::LDAP::Utils 2.0 qw(normalizeDN);
use Tie::Hash;

use strict;
use vars qw($VERSION @ISA);

use overload
  '""'	=> \&getLDIFString;

@ISA = ('Tie::StdHash');
$VERSION = "1.0";


#############################################################################
# Constructor.
#
sub new
{
  my ($class) = shift;
  my (%entry, $obj);

  tie %entry, $class;
  $obj = bless \%entry, $class;

  return $obj;
}


#############################################################################
# Creator, make a new tie hash instance, which will keep track of all
# changes made to the hash array. This is needed so we only update modified
# attributes.
#
sub TIEHASH
{
  my ($class, $self) = (shift, {});

  return bless $self, $class;
}
