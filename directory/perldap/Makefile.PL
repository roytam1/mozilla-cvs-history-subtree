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
# Donley, portions created by Leif Hedstrom are Copyright (C) Leif
# Hedstrom. All Rights Reserved.
#
# Contributor(s):
#    * Leif Hedstrom <leif@perldap.org>
#
# DESCRIPTION
#    The Makefile "source".
#
#############################################################################

require 5.005;

use ExtUtils::MakeMaker;
use ExtUtils::Liblist;
use Config;
use Carp;

$osname   = $Config{'osname'};

$ldapsdk_loc = $ENV{"LDAPSDKDIR"}; # Full Path to C SDK Top-Level
$ldapsdk_ssl = $ENV{"LDAPSDKSSL"}; # N to exclude SSL
$ldapsdk_ver = $ENV{"LDAPV3ON"};   # N to exclude LDAP v3 features
$ldapsdk_pr = $ENV{"LDAPPR"};      # Y to include prldap_ functions

$libexts = "so|sl|a|lib";


print "\nPerLDAP - Perl 5 Module for LDAP\n";
print     "================================\n";

$silent = 1;
print "Directory containing 'include' and 'lib' directory of the Netscape\n";
print "LDAP Software Developer Kit (default: /usr): ";
if (!$ldapsdk_loc)
{
   $silent = 0;
   chomp ($ldapsdk_loc = <STDIN>);
   $ldapsdk_loc = "/usr" unless $ldapsdk_loc =~ /\S/;
} else {
  print "$ldapsdk_loc\n";
}
croak("Directory $ldapsdk_loc does not exist!") unless -d $ldapsdk_loc;

if ($osname =~ /mswin/i)
{
   $dir_sep = "\\";
} else {
   $dir_sep = "/";
}

$include_ldap = $ldapsdk_loc . $dir_sep . "include";
$include_nspr = $ldapsdk_loc . $dir_sep . "include-nspr";
$lib_ldap = $ldapsdk_loc . $dir_sep . "lib";

print "Using LDAPv3 Developer Kit (default: yes)?  ";
if (!$ldapsdk_ver)
{
   $silent = 0;
   chomp ($ldapsdk_ver = <STDIN>);
} else {
  print "$ldapsdk_ver\n";
}
$v3_def = "-DLDAPV3" unless ($ldapsdk_ver =~ /^n/i);

   
print "Include SSL Support (default: yes)?  ";
if (!$ldapsdk_ssl)
{
   $silent = 0;
   chomp ($ldapsdk_ssl = <STDIN>);
} else {
  print "$ldapsdk_ssl\n";
}
$ssl_def = "-DUSE_SSL" unless ($ldapsdk_ssl =~ /^n/i);

opendir(DIR,$lib_ldap);
if ($ssl_def)
{
   @libfiles = grep{/ldap|lber|ssl|nss|pl[cd]|nspr/} readdir(DIR);
} else {
   @libfiles = grep{/ldap|lber/} readdir(DIR);
 }
closedir(DIR);

@libs = ();
foreach $candidate (@libfiles) {
   $candidate =~ s/^lib//;
   $candidate =~ s/\.($libexts)$//;
   push(@libs, $candidate) unless grep(/^$candidate$/, @libs);
}

if (!((@ldaplib = grep{/ldapssl$/} @libs) && $ssl_def))
{
   @ldaplib = grep{/^ldap.*$/} @libs;
   @prldaplib = grep{/^prldap.*$/} @libs;
   @lberlib = grep{/^lber.*$/} @libs;

   @ldapslib = grep{/^ssldap.*$/} @libs if $ssl_def;
   @ssllib = grep{/^ssl[0-9].*$/} @libs if $ssl_def;
   @nsslib = grep{/^nss[0-9].*$/} @libs if $ssl_def;
   @plclib = grep{/^plc[0-9].*$/} @libs if $ssl_def;
   @pldslib = grep{/^plds[0-9].*$/} @libs if $ssl_def;
}

@nsprlib = grep{/^nspr[0-9].*$/} @libs;
if ($#nsprlib >= 0)
{
  print "Support NSPR API versions (default: no)?  ";
  if (!$ldapsdk_pr)
    {
      $silent = 0;
      chomp ($ldapsdk_pr = <STDIN>);
    } else {
      print "$ldapsdk_pr\n";
    }
}
$pr_def = "-DPRLDAP" if ($ldapsdk_pr =~ /^y/i);
@nsprlib = () unless ($ldapsdk_pr =~ /^y/i);

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

#
# Add "main" LDAP library to link line, and the NSPR version
# as well (if available).
#
$lline = "-l$ldaplib[0]";
$lline .= " -l$prldaplib[0]" if ($#prldaplib >= 0);

#
# I think this is obsolete, and will never trigger, but I'll keep it.
#
if ($#lberlib >= 0 && $lline =~ /ldap$/)
{
   $lline .= " -l$lberlib[0]";
}

#
# Add SSL libraries, if needed
#
$lline .= " -l$ldapslib[0]" if ($#ldapslib >= 0);
$lline .= " -l$ssllib[0]" if ($#ssllib >= 0);

# Add the NSS/NSPR libraries, if needed
$lline .= " -l$nsslib[0]" if ($#nsslib >= 0);
$lline .= " -l$plclib[0]" if ($#plclib >= 0);
$lline .= " -l$pldslib[0]" if ($#pldslib >= 0);
$lline .= " -l$nsprlib[0]" if ($#nsprlib >= 0);

print "Libraries to link with (default: $lline):  ";
if (!$silent)
{
   chomp ($lib_line = <STDIN>);
   $lib_line = $lline unless $lib_line =~ /\S/;
} else {
  $lib_line = $lline;
  print "\n";
}

$lib_line = "-L$lib_ldap $lib_line";

# Include directories etc.
$my_includes = "";
$my_includes .= " -I$include_ldap" unless ($include_ldap eq "/usr/include");
$my_includes .= " -I$include_nspr" if $pr_def;

# Add system dependant stuff here...
@extras = ();
if ($osname =~ /mswin/i)
{
   $my_extlib = "$lib_ldap\\$ldaplib[0]";
   $my_extlib .= " $lib_ldap\\$lberlib[0]" if $#lberlib >= 0;
   push(@extras, 'dynamic_lib' => {
     'OTHERLDFLAGS' => "kernel32.lib oldnames.lib" });
} else {
   $my_extlib = "";
}

if ($^O eq 'MSWin32' and $Config{archname} =~ /-object\b/i) {
   push(@extras, CAPI => 'TRUE');
}


#
# See if we need to add an LD_RUN_PATH. This seems to have changed
# somewhere between Perl 5.8.0 and 5.8.5. So, on some systems this
# will add an extra LD_RUN_PATH ...
#
@liblist = ExtUtils::Liblist->ext($lib_line);
if ($liblist[3] ne "")
{
  use Config;
  push(@extras,
       'LD'	=> 'LD_RUN_PATH=$(LD_RUN_PATH)' . " $Config{ld}",
       );
}


#
# Ok, let's do it!
#
WriteMakefile(
              'ABSTRACT'	=> 'Perl methods for LDAP C API calls',
              'AUTHOR'		=> 'Leif Hedstrom <leif@ogre.com>',
	      'NAME'		=> 'Mozilla::LDAP::API',
	      'DISTNAME'	=> 'PerLDAP',

	      'VERSION_FROM'	=> 'API.pm',
	      'INC'		=> $my_includes,
	      'LIBS'		=> [$lib_line],
	      'MYEXTLIB'	=> $my_extlib,
	      'DEFINE'		=> "$v3_def $ssl_def $pr_def",
	      'XSOPT'		=> "-nolinenumbers",
	      @extras
);


#
# Generate a "make HTML" target
#
sub MY::postamble
{
  '
.SUFFIXES: .pm .html
.PHONY: html

.pm.html:
	pod2html --netscape $< > $@

html:	Entry.html Conn.html Utils.html API.html LDIF.html $(FIRST_MAKEFILE)
	@rm -f pod2html-itemcache pod2html-dircache
'
}
