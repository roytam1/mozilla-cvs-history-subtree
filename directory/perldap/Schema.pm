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

use strict;
use vars qw($VERSION);

use overload
  '""'	=> \&getLDIFString;

$VERSION = "2.0";


sub new
{

  # do the search for everything
  my ($self) = shift;
  my $conn = shift;

  my $base = "cn=schema";
  
  my $initialReturn = $conn->search($base, "base", "objectclass=*");
  my @attributeArray = $initialReturn->getValues("attributetypes");
  my (%schema, $obj);

  foreach (@attributeArray)  {
    my $attr = new Mozilla::LDAP::Schema::Attribute($_);
    $schema{"attributes"}{$attr->name()} = $attr;
 }
  $obj = bless \%schema;
  #$objclass = new Mozilla::LDAP::Schema::ObjectClass();
return $obj;    
}

sub attributes
{
  my ($self, $attr) = (shift, lc shift);
  return 0 unless (defined($attr) && ($attr ne ""));
  return $self->{"attributes"}{$attr};
}

1;

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
  $self->{"name"} = lc $tempArray[3];
  $self->{"description"} = lc $tempArray[5];
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

sub oid
{
  my $self = shift; 
  if(@_) {
    $self->{'oid'} = shift;
  }
  $self->{'oid'}; 
}

sub name
{
  my $self = shift; 
  if(@_) {
    $self->{'name'} = lc shift;
  }
  $self->{'name'}; 
}

sub description
{
  my $self = shift; 
  if(@_) {
    $self->{'description'} = shift;
  }
  $self->{'description'}; 
}

sub syntax
{
  my $self = shift; 
  if(@_) {
    $self->{'syntax'} = shift;
  }
  $self->{'syntax'}; 
}

1;

package Mozilla::LDAP::Schema::ObjectClass;

sub new
{
#  return self;
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

  Mozilla::LDAP::Entry.pm - Object class to hold one LDAP entry.

=head1 SYNOPSIS

  use Mozilla::LDAP::Conn;
  use Mozilla::LDAP::Entry;

=head1 ABSTRACT

The LDAP::Conn object is used to perform LDAP searches, updates, adds and
deletes. All such functions works on LDAP::Entry objects only. All
modifications and additions you'll do to an LDAP entry, will be done
through this object class.

=head1 DESCRIPTION

The LDAP::Entry object class is built on top of the Tie::Hash standard
object class. This gives us several powerful features, the main one being
to keep track of what is changing in the LDAP entry. This makes it very
easy to write LDAP clients that needs to update/modify entries, since
you'll just do the changes, and this object class will take care of the
rest.

We define local functions for STORE, FETCH, DELETE, EXISTS, FIRSTKEY and
NEXTKEY in this object class, and inherit the rest from the super
class. Overloading these specific functions is how we can keep track of
what is changing in the entry, which turns out to be very convenient. We
can also easily "loop" over the attribute types, ignoring internal data,
or deleted attributes.

Most of the methods here either return the requested LDAP value, or a
status code. The status code (either 0 or 1) indicates the failure or
success of a certain operation. 0 (False) meaning the operation failed,
and a return code of 1 (True) means complete success.

One thing to remember is that in LDAP, attribute names are case
insensitive. All methods in this class are aware of this, and will convert
all attribute name arguments to lower case before performing any
operations. This does not mean that the values are case insensitive. On
the contrary, all values are considered case sensitive by this module,
even if the LDAP server itself treats it as a CIS attribute.

=head1 OBJECT CLASS METHODS

The LDAP::Entry class implements many methods you can use to access and
modify LDAP entries. It is strongly recommended that you use this API as
much as possible, and avoid using the internals of the class
directly. Failing to do so may actually break the functionality.

=head2 Creating a new entry

To create a completely new entry, use the B<new> method, for instance

    $entry = new Mozilla::LDAP::Entry()
    $entry->setDN("uid=leif,ou=people,dc=netscape,dc=com");
    $entry->{objectclass} = [ "top", "person", "inetOrgPerson" ];
    $entry->addValue("cn", "Leif Hedstrom");
    $entry->addValue("sn", "Hedstrom");
    $entry->addValue("givenName", "Leif");
    $entry->addValue("mail", "leif@netscape.com);

    $conn->add($entry);

This is the minimum requirements for an LDAP entry. It must have a DN, and
it must have at least one objectclass. As it turns out, by adding the
I<person> and I<inetOrgPerson> classes, we also must provide some more
attributes, like I<CN> and I<SN>. This is because the object classes have
these attributes marked as "required", and we'd get a schema violation
without those values.

In the example above we use both native API methods to add values, and
setting an attribute entire value set directly. Note that the value set is
a pointer to an array, and not the array itself. In the example above, the
object classes are set using an anonymous array, which the API handles
properly. It's important to be aware that the attribute value list is
indeed a pointer.

Finally, as you can see there's only only one way to add new LDAP entries,
and it's called add(). It normally takes an LDAP::Entry object instance as
argument, but it can also be called with a regular hash array if so
desired.

=head2 Adding and removing attributes and values

This is the main functionality of this module. Use these methods to do any
modifications and updates to your LDAP entries.

=over 13

=item B<addDNValue>

Just like B<addValue>, except this method assume the value is a DN
attribute. For instance

   $dn = "uid=Leif, dc=Netscape, dc=COM";
   $entry->addDNValue("uniqueMember", $dn);


will only add the DN for "uid=leif" if it does not exist as a DN in the
uniqueMember attribute.

=item B<addValue>

Add a value to an attribute. If the attribute value already exists, or we
couldn't add the value for any other reason, we'll return FALSE (0),
otherwise we return TRUE (1). The first two arguments are the attribute
name, and the value to add.

The optional third argument is a flag, indicating that we want to add the
attribute without checking for duplicates. This is useful if you know the
values are unique already, or if you perhaps want to allow duplicates for
a particular attribute. To add a CN to an existing entry/attribute, do:

    $entry->addValue("cn", "Leif Hedstrom");

=item B<attrModified>

This is an internal function, that can be used to force the API to
consider an attribute (value) to have been modified. The only argument is
the name of the attribute. In almost all situation, you never, ever,
should call this. If you do, please contact the developers, and as us to
fix the API. Example

    $entry->attrModified("cn");

=item B<copy>

Copy the value of one attribute to another.  Requires at least two
arguments.  The first argument is the name of the attribute to copy, and
the second argument is the name of the new attribute to copy to.  The new
attribute can not currently exist in the entry, else the copy will fail.
There is an optional third argument (a boolean flag), which, when set to
1, will force an
override and copy to the new attribute even if it already exists.  Returns TRUE if the copy
was successful.

    $entry->copy("cn", "description");

=item B<exists>

Return TRUE if the specified attribute is defined in the LDAP entry. This
is useful to know if an entry has a particular attribute, regardless of
the value. For instance:

    if ($entry->exists("jpegphoto")) { # do something special }

=item B<getDN>

Return the DN for the entry. For instance

    print "The DN is: ", $entry->getDN(), "\n";

Just like B<setDN>, this method also has an optional argument, which
indicates we should normalize the DN before returning it to the caller.

=item B<getValues>

Returns an entire array of values for the attribute specified.  Note that
this returns an array, and not a pointer to an array. This method is
deprecated, use B<values> instead.

=item B<hasValue>

Return TRUE or FALSE if the attribute has the specified value. A typical
usage is to see if an entry is of a certain object class, e.g.

    if ($entry->hasValue("objectclass", "person", 1)) { # do something }

The (optional) third argument indicates if the string comparison should be
case insensitive or not, and the (optional) fourth argument indicats
wheter we should normalize the string as if it was a DN. The first two
arguments are the name and value of the attribute, respectively.

=item B<hasDNValue>

Exactly like B<hasValue>, except we assume the attribute values are DN
attributes.

=item B<isAttr>

This method can be used to decide if an attribute name really is a valid
LDAP attribute in the current entry. Use of this method is fairly limited,
but could potentially be useful. Usage is like previous examples, like

    if ($entry->isAttr("cn")) { # do something }

The code section will only be executed if these criterias are true:

    1. The name of the attribute is a non-empty string.
    2. The name of the attribute does not begin, and end, with an
       underscore character (_).
    2. The attribute has one or more values in the entry.

=item B<isDeleted>

This is almost identical to B<isModified>, except it tests if an attribute
has been deleted. You use it the same way as above, like

    if (! $entry->isDeleted("cn")) { # do something }

=item B<isModified>

This is a somewhat more useful method, which will return the internal
modification status of a particular attribute. The argument is the name of
the attribute, and the return value is True or False. If the attribute has
been modified, in any way, we return True (1), otherwise we return False
(0). For example:

    if ($entry->isModified("cn")) { # do something }

=item B<matchValue>

This is very similar to B<hasValue>, except it does a regular expression
match instead of a full string match. It takes the same arguments,
including the optional third argument to specify case insensitive
matching. The usage is identical to the example for hasValue, e.g.

    if ($entry->matchValue("objectclass", "pers", 1)) { # do something }

=item B<matchDNValue>

Like B<matchValue>, except the attribute values are considered being DNs.

=item B<move>

Identical to the copy method, except the original attribute is
deleted once the move to the new attribute is complete.

    $entry->move("cn", "sn");

=item B<printLDIF>

Print the entry in a format called LDIF (LDAP Data Interchange
Format, RFC xxxx). An example of an LDIF entry is:

    dn: uid=leif,ou=people,dc=netscape,dc=com
    objectclass: top
    objectclass: person
    objectclass: inetOrgPerson
    uid: leif
    cn: Leif Hedstrom
    mail: leif@netscape.com

The above would be the result of

    $entry->printLDIF();

If you need to write to a file, open and then select() it.
For more useful LDIF functionality, check out the
Mozilla::LDAP::LDIF.pm module.

=item B<remove>

This will remove the entire attribute, including all it's values, from the
entry. The only argument is the name of the attribute to remove. Let's say
you want to nuke all I<mailAlternateAddress> values (i.e. the entire
attribute should be removed from the entry):

    $entry->remove("mailAlternateAddress");

=item B<removeValue>

Remove a value from an attribute, if it exists. Of course, if the
attribute has no such value, we won't try to remove it, and instead return
a False (0) status code. The arguments are the name of the attribute, and
the particular value to remove. Note that values are considered case
sensitive, so make sure you preserve case properly. An example is:

    $entry->removeValue("objectclass", "nscpPerson");

=item B<removeDNValue>

This is almost identical to B<removeValue>, except it will normalize the
attribute values before trying to remove them. This is useful if you know
that the attribute is a DN value, but perhaps the values are not cosistent
in all LDAP entries. For example

   $dn = "uid=Leif, dc=Netscape, dc=COM";
   $entry->removeDNValue("owner", $dn);


will remove the owner "uid=leif,dc=netscape,dc=com", no matter how it's
capitalized and formatted in the entry.

=item B<setDN>

Set the DN to the specified value. Only do this on new entries, it will
not work well if you try to do this on an existing entry. If you wish to
rename an entry, use the Mozilla::Conn::modifyRDN method instead.
Eventually we'll provide a complete "rename" method. To set the DN for a
newly created entry, we can do

    $entry->setDN("uid=leif,ou=people,dc=netscape,dc=com");

There is an optional third argument, a boolean flag, indicating that we
should normalize the DN before setting it. This will assure a consistent
format of your DNs.

=item B<setValues>

Set the specified attribute to the new value (or values), overwriting
whatever old values it had before. This is a little dangerous, since you
can lose attribute values you didn't intend to remove. Therefore, it's
usually recommended to use B<removeValue()> and B<setValues()>. This
method is deprecated, use B<values> instead.

=item B<size>

Return the number of values for a particular attribute. For instance

    $entry->{cn} = [ "Leif Hedstrom", "The Swede" ];
    $numVals = $entry->size("cn");

This will set C<$numVals> to two (2). The only argument is the name of the
attribute, and the return value is the size of the value array.

=item B<values>

This is the get/set method for manipulating an entire set of values for a
particular attribute. The first argument is the attribute you wish to set
or get, and the optional second (and so forth) arguments is the list of
attribute values to set.

To get the values from the I<CN> attribute, just do

    @vals = $entry->values("CN");


To set some attributes entire set of values, you can do

    $entry->values("cn", "Leif Hedstrom", "The Swede");
    $entry->values("mail", @mailAddresses);

or if it's a single value attribute,

    $entry->values("uidNumber", "12345");


The only important thing to remember is that the optional value arguments
must be array elements (or an array), not a pointer to an array (or an
anonymous array).

=back

=head2 Deleting entries

To delete an LDAP entry from the LDAP server, you have to use the
B<delete> method from the Mozilla::LDAP::Conn module. It will actually
delete any entry, if you provide an legitimate DN.

=head2 Renaming entries

Again, there's no functionality in this object class to rename the entry
(i.e. changing it's DN). For now, there is a way to modify the RDN
component of a DN through the Mozilla::LDAP::Conn module, with
B<modifyRDN>. Eventually we hope to have a complete B<rename> method,
which should be capable of renaming any entry, in any way, including
moving it to a different part of the DIT (Directory Information Tree).

=head1 EXAMPLES

There are plenty of examples to look at, in the examples directory. We are
adding more examples every day (almost).

=head1 INSTALLATION

Installing this package is part of the Makefile supplied in the
package. See the installation procedures which are part of this package.

=head1 AVAILABILITY

This package can be retrieved from a number of places, including:

    http://www.mozilla.org/directory/
    Your local CPAN server

=head1 CREDITS

Most of this code was developed by Leif Hedstrom, Netscape Communications
Corporation. 

=head1 BUGS

None. :)

=head1 SEE ALSO

L<Mozilla::LDAP::Conn>, L<Mozilla::LDAP::API>, and of course L<Perl>.

=cut
