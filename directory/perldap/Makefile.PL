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
if ($osname =~ /mswin/i)
{
   $dir_sep = "\\";
} else {
   $dir_sep = "/";
}

$ldapsdk_loc = $ENV{"LDAPSDKDIR"}; # Full Path to C SDK Top-Level
$include_ldap = $ENV{"LDAPSDKINCDIR"}; # Full Path to C SDK Include file directory
$lib_ldap = $ENV{"LDAPSDKLIBDIR"}; # Full Path to C SDK Library directory
$ldapsdk_ssl = $ENV{"LDAPSDKSSL"}; # N to exclude SSL
$ldapsdk_pr = $ENV{"LDAPPR"};      # Y to include prldap_ functions
$nspr_loc = $ENV{"NSPRDIR"};       # Full Path to NSPR Top-Level

$libexts = "so|sl|a|lib|dll|dylib";
$default_root = "${dir_sep}usr";

# every linux distribution has the OpenLDAP SDK in /usr/lib and /usr/include, so
# we need to use a different default to avoid problems
if ($osname eq "linux") {
   $default_root = "/opt/mozldap";
}

# examine our build environment - see if we are being built as part of the overall
# mozilla build process - if so, we can default a lot of things based on that
# for example, if perldap was checked out in mozilla/directory/perldap, and
# nspr in mozilla/nsprpub, and nss in mozilla/security, and the ldap c sdk in
# mozilla/directory/c-sdk, and everything was built according to the usual
# build instructions, then we can just use ../../dist/public/ldap as our
# include file location and ../../dist/lib as our lib location
if ( !$include_ldap && -d '../../dist/public/ldap' ) {
   $include_ldap = "..${dir_sep}..${dir_sep}dist${dir_sep}public${dir_sep}ldap";
}
if ( !$lib_ldap && -d '../../dist/lib' ) {
   $lib_ldap = "..${dir_sep}..${dir_sep}dist${dir_sep}lib";
}

# find the path to NSPR
if (!$nspr_loc && -d '../../dist') {
	require File::Find;
	$nspr_loc = findNSPR('../../dist');
}

print "\nPerLDAP - Perl 5 Module for LDAP\n";
print     "================================\n";

$silent = 1;
print "Directory containing 'include' and 'lib' directory of the Mozilla\n";
print "LDAP Software Developer Kit (default: $default_root): ";
if (!$ldapsdk_loc)
{
   $silent = 0;
   chomp ($ldapsdk_loc = <STDIN>);
   $ldapsdk_loc = $default_root unless $ldapsdk_loc =~ /\S/;
   if (! -d $ldapsdk_loc) {
	   $ldapsdk_loc = undef;
   }
} else {
  print "$ldapsdk_loc\n";
}

if (!$include_ldap && $ldapsdk_loc) {
   $include_ldap = $ldapsdk_loc . $dir_sep . "include";
}
if (!$ldapsdk_loc) {
   print "Directory containing 'include' directory of the Mozilla\n";
   print "LDAP Software Developer Kit (default: $include_ldap): ";
   if (! -f "$include_ldap${dir_sep}ldap.h") {
      $silent = 0;
	  chomp ($input = <STDIN>);
	  $include_ldap = $input if $input =~ /\S/;
	  croak("Missing required include file $include_ldap${dir_sep}ldap.h") unless -f "$include_ldap${dir_sep}ldap.h";
  } else {
	  print "$include_ldap\n";
  }
}

if (!$lib_ldap && $ldapsdk_loc) {
   $lib_ldap = $ldapsdk_loc . $dir_sep . "lib";
}
if (!$ldapsdk_loc) {
   print "Directory containing 'lib' directory of the Mozilla\n";
   print "LDAP Software Developer Kit (default: $lib_ldap): ";
   if (! -d "$lib_ldap") {
      $silent = 0;
	  chomp ($input = <STDIN>);
	  $lib_ldap = $input if $input =~ /\S/;
	  croak("Missing required lib directory $lib_ldap") unless -d "$lib_ldap";
  } else {
	  print "$lib_ldap\n";
  }
}

print "Include SSL Support (default: yes)?  ";
if (!$ldapsdk_ssl)
{
   $silent = 0;
   chomp ($ldapsdk_ssl = <STDIN>);
} else {
  print "$ldapsdk_ssl\n";
}
$ssl_def = "-DUSE_SSL" unless ($ldapsdk_ssl =~ /^n/i);

# see if ldap and nspr files are in the same place
if ( -f "$include_ldap${dir_sep}nspr.h" ) {
	$nspr_loc = $ldapsdk_loc;
}

print "Directory containing NSPR API 'include' and 'lib'\n";
print "directories for NSPR support (type 'n' or 'none' to omit) (default: $nspr_loc): ";
$silent = 0;
chomp ($input = <STDIN>);
if ($input =~ /^n/i) {
	$nspr_loc = undef;
} elsif ($input =~ /\S/) {
	$nspr_loc = $input;
} else {
	print "$nspr_loc\n";
}

if ($nspr_loc) {
   $lib_nspr = $nspr_loc . ${dir_sep} . 'lib';
   $include_nspr = $nspr_loc . ${dir_sep} . 'include';
   croak("Missing required include file $include_nspr${dir_sep}nspr.h") unless -f "$include_nspr${dir_sep}nspr.h";
   $pr_def = "-DPRLDAP"
}

opendir(DIR,$lib_ldap);
if ($ssl_def)
{
   @libfiles = grep{/ldap|lber|ssl|nss|pl[cd]|nspr/} readdir(DIR);
} else {
   @libfiles = grep{/ldap|lber/} readdir(DIR);
}
closedir(DIR);

if ($lib_nspr && -d $lib_nspr) {
opendir(DIR,$lib_nspr);
push @libfiles, grep{/pl[cd]|nspr/} readdir(DIR);
closedir(DIR);
}

@libs = ();
foreach $candidate (@libfiles) {
   $candidate =~ s/^lib//;
   $candidate =~ s/\.($libexts)$//;
   push(@libs, $candidate) unless grep(/^$candidate$/, @libs);
}

if (!((@ldaplib = grep{/ldapssl$/} @libs) && $ssl_def))
{
   @ldaplib = grep{/^(ns)?ldap.*$/} @libs;
   @prldaplib = grep{/^(prldap|nsldappr).*$/} @libs;
   @lberlib = grep{/^(ns)?lber.*$/} @libs;

   @ldapslib = grep{/^(ssldap|nsldapssl).*$/} @libs if $ssl_def;
   @ssllib = grep{/^ssl[0-9].*$/} @libs if $ssl_def;
   @nsslib = grep{/^nss[0-9].*$/} @libs if $ssl_def;
   @plclib = grep{/^plc[0-9].*$/} @libs if $ssl_def;
   @pldslib = grep{/^plds[0-9].*$/} @libs if $ssl_def;
}

@nsprlib = grep{/^nspr[0-9].*$/} @libs;

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
   $my_extlib = "$lib_ldap\\$ldaplib[0].lib";
   $my_extlib .= " $lib_ldap\\$lberlib[0].lib" if $#lberlib >= 0;
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
print "\n######### before WriteMakefile #############\n";
WriteMakefile(
              'ABSTRACT'	=> 'Perl methods for LDAP C API calls',
              'AUTHOR'		=> 'Leif Hedstrom <leif@ogre.com>',
	      'NAME'		=> 'Mozilla::LDAP::API',
	      'DISTNAME'	=> 'PerLDAP',

	      'VERSION_FROM'	=> 'API.pm',
	      'INC'		=> $my_includes,
	      'LIBS'		=> [$lib_line],
	      'MYEXTLIB'	=> $my_extlib,
	      'DEFINE'		=> "$ssl_def $pr_def",
	      'XSOPT'		=> "-nolinenumbers",
	      @extras
);

print "\n######### after WriteMakefile #############\n";

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

sub find_helper {
   if (/nspr\.h/) {
	   $nspr_loc = File::Basename::dirname($File::Find::dir);
   }
}

sub findNSPR {
   my $dir = shift;
   File::Find::find(\&find_helper, $dir);
   return $nspr_loc;
}
