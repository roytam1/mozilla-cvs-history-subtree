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
#    The Makefile "source".
#
#############################################################################

use ExtUtils::MakeMaker;
use Config;

$perlpath = $Config{'perlpath'};
$osname   = $Config{'osname'};

$ldapsdk_loc = $ENV{"LDAPSDKDIR"}; # Full Path to C SDK Top-Level
$ldapsdk_ssl = $ENV{"LDAPSDKSSL"}; # Y to include SSL

$libexts = "so|sl|a|lib";


print "\nPerLDAP - Perl 5 Module for LDAP\n";
print     "================================\n";

if (!$ldapsdk_loc)
{
   print "Directory containing 'include' and 'lib' directory of the Netscape\n";
   print "LDAP Software Developer Kit (default: /usr): ";
   chomp ($ldapsdk_loc = <STDIN>);
   $ldapsdk_loc = "/usr" unless $ldapsdk_loc =~ /\S/;
}

if ($osname =~ /mswin/i)
{
   $dir_sep = "\\";
} else {
   $dir_sep = "/";
}

$include_ldap = $ldapsdk_loc . $dir_sep . "include";
$lib_ldap = $ldapsdk_loc . $dir_sep . "lib";

if (!$ldapsdk_ver)
{
   print "Using LDAPv3 Developer Kit (default: yes)?  ";
   chomp ($ver = <STDIN>);
   if ($ver !~ /^n/i)
   {
      $v3_def = "-DLDAPV3";
   }
}
   
if (!$ldapsdk_ssl)
{
   print "Include SSL Support (default: yes)?  ";
   chomp ($ldapsdk_ssl = <STDIN>);
}

if ($ldapsdk_ssl !~ /^n/i)
{
   $ssl_def = "-DUSE_SSL";
}

opendir(DIR,$lib_ldap);
@files = grep{/ldap|lber/} readdir(DIR);
closedir(DIR);

if (!((@ldaplib = grep{/ldapssl.*\.($libexts)$/} @files) && $ssl_def))
{
   @ldaplib = grep{/ldap.*\.($libexts)$/} @files;
   @lberlib = grep{/lber.*\.($libexts)$/} @files;
}

if ($#ldaplib < 0)
{
   die "No LDAP libraries found.";
}

if ($#ldaplib > 0)
{
   print "Located multiple libraries:\n";
   foreach $alib (@ldaplib)
   {
      print " - $alib\n";
   }
}

$lline_ldap = $ldaplib[0];
$lline_ldap =~ s/^lib//;
$lline_ldap =~ s/\.($libexts)$//;
$lline = "-L$lib_ldap -l$lline_ldap";

if ($#lberlib >= 0 && $lline =~ /ldap$/)
{
   $lline_lber = $lberlib[0];
   $lline_lber =~ s/^lib//;
   $lline_lber =~ s/\.($libexts)$//;
   $lline .= " -l$lline_lber";
}

print "Libraries to link with (default: $lline):  ";
chomp ($lib_line = <STDIN>);
$lib_line = $lline unless $lib_line =~ /\S/;

if ($osname =~ /mswin/i)
{
   $myextlib = "$lib_ldap\\$ldaplib[0]";
   if ($lber_lib)
   {
      $myextlib .= " $lib_ldap\\$lberlib[0]";
   }
} else {
   $myextlib = "";
}

@extras = ();

push(@extras,
     CAPI => 'TRUE')
    if ($] >= 5.005 and $^O eq 'MSWin32'
        and $Config{archname} =~ /-object\b/i);

push(@extras,
     ABSTRACT => 'Perl methods for LDAP C API calls',
     AUTHOR   => 'Netscape Communications Corp., Inc. and Clayton Donley')
    if ($ExtUtils::MakeMaker::Version >= 5.4301);

WriteMakefile(
	'NAME'		=> 	'Mozilla::LDAP::API',
        'DISTNAME'	=>  'PerLDAP',
	'VERSION_FROM'	=> 	'API.pm',
	($include_ldap ne "/usr/include" ? (
	'INC'		=> 	"-I$include_ldap",
	) : (
	'INC'		=> 	"",
	)),
	'LIBS'		=> 	[$lib_line],
	'MYEXTLIB'	=>	$myextlib,
	'DEFINE'	=> 	"$v3_def $ssl_def",
	'XSOPT'		=>	"-nolinenumbers",
    @extras
);
