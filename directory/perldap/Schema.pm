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


sub new
{

  # do the search for everything
  my ($self) = shift;
  my (%args);

  %args = @_ unless (scalar(@_) % 2);
  
  if (! defined($args{"basedn"}))
    {
      $args{"base"} = "cn=schema";
    }

  
  my $initialReturn = $args{"conn"}->search($args{"base"},
					    "base",
					    "objectclass=*");

my @attributeArray = $initialReturn->getValues("attributetypes");
  foreach (@attributeArray)  {
    $attr = new Mozilla::LDAP::Schema::Attribute($_);
  }
  $objclass = new Mozilla::LDAP::Schema::ObjectClass();
  
  
}

package Mozilla::LDAP::Schema::Attribute;
use Text::ParseWords;

my %fields =  (
	       oid		=>	undef,
	       name		=>	undef,
	       description	=>	undef,
	       syntax		=>	undef,
	       );

sub _initialize
{
  my ($self, $attrString) = @_;
  #take off the first and last parentheses
  #substr ($attrString,0,1)="";
  chop ($attrString);
  my @tempArray = quotewords(" ", 0, $attrString);

  $self->{"oid"} = $tempArray[1];
  $self->{"name"} = $tempArray[3];
  $self->{"description"} = $tempArray[5];
  $self->{"syntax"} = $tempArray[7];
  
}


sub new 
{
  
  my $tmp = shift;
  my $self = {%fields};
  bless $self;
  $self->_initialize(@_);
  #take off the first and last parentheses
  
  return $self;
}  

package Mozilla::LDAP::Schema::ObjectClass;

sub new
{
  return self;
}

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
