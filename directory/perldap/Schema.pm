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
#
#############################################################################

package Mozilla::LDAP::Schema;

use Mozilla::LDAP::Utils 2.0 qw(normalizeDN);
use Tie::Hash;

use strict;
use vars qw($VERSION @ISA);

use overload
  '""'	=> \&getLDIFString;

@ISA = ('Tie::StdHash');
$VERSION = "2.0";

package Mozilla::LDAP::Schema::Attribute;

package Mozilla::LDAP::Schema::ObjectClass;

#############################################################################
# Mandatory TRUE return value.
#
1;


#############################################################################
# POD documentation...
#
__END__

=head1 NAME

  Mozilla::LDAP::Schema.pm

=head1 SYNOPSIS


=head1 ABSTRACT

=head1 DESCRIPTION


=head1 OBJECT CLASS METHODS


=cut
