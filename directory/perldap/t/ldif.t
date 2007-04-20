#!/usr/bin/perl
#############################################################################
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
# The Original Code is PerlDAP. The Initial Developer of the Original
# Code is Netscape Communications Corp. and Clayton Donley. Portions
# created by Netscape are Copyright (C) Netscape Communications
# Corp., portions created by Clayton Donley are Copyright (C) Clayton
# Donley. All Rights Reserved.
#
# Contributor(s):
#
# DESCRIPTION
#    Test the LDIF module
#
#############################################################################

use Mozilla::LDAP::Conn;
use Mozilla::LDAP::Utils qw(normalizeDN);
use Mozilla::LDAP::API qw(ldap_explode_dn);
use Mozilla::LDAP::LDIF;
use strict;


my $testldif = "t/test.ldif";

print "1..1\n";
open( LDIF, "$testldif" ) || die "Can't open $testldif: $!";
my $in = new Mozilla::LDAP::LDIF(*LDIF) ;
my $ent;
while ($ent = readOneEntry $in) {
    print "The Entry is:\n";
    Mozilla::LDAP::LDIF::put_LDIF(select(), 78, $ent);
    my $desc = $ent->getValue('description');
    # this tests the base64 decoding
    die "Error: the description is not hello world" if ($desc ne 'hello world');
}
close LDIF;
print "ok 1\n";
