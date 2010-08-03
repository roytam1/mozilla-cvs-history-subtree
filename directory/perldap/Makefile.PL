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
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 2001
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Clayton Donley
#   Leif Hedstrom <leif@perldap.org>
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
#    The Makefile "source".

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
$include_nspr = $ENV{"NSPRINCDIR"};       # Full Path to NSPR include dir
$lib_nspr = $ENV{"NSPRLIBDIR"};       # Full Path to NSPR lib dir
$nss_loc = $ENV{"NSSDIR"};       # Full Path to NSS Top-Level
$lib_nss = $ENV{"NSSLIBDIR"};       # Full Path to NSS lib dir
$use_openldap = $ENV{"USE_OPENLDAP"}; # Y to use OpenLDAP instead of MozLDAP

$libexts = "so|sl|a|lib|dll|dylib";
$default_root = "${dir_sep}usr";

# every linux distribution has the OpenLDAP SDK in /usr/lib and /usr/include, so
# we need to use a different default to avoid problems
if (!$use_openldap) {
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
   if (!$nspr_loc && !$include_nspr && !$lib_nspr && -d '../../dist') {
      require File::Find;
      $nspr_loc = findNSPR('../../dist');
      $include_nspr = $nspr_loc . "/include";
      $lib_nspr = $nspr_loc . "/lib";
   }
} else {
   if ($osname eq "linux") {
      $default_root = "/usr";
   }
}

print "\nPerLDAP - Perl 5 Module for LDAP\n";
print     "================================\n";

$silent = 1;
if ($use_openldap)
{
   print "Directory containing 'include' and 'lib' directory of the OpenLDAP\n";
   print "LDAP client libraries (default: $default_root, Enter \"n\" to specify\n" .
      "explicit include and lib directories): ";
} else {
   print "Directory containing 'include' and 'lib' directory of the Mozilla\n";
   print "LDAP Software Developer Kit (default: $default_root, Enter \"n\" to\n" .
      "specify explicit include and lib directories): ";
}

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
   if ($use_openldap) {
      print "Directory containing 'include' files of the OpenLDAP\n";
      print "LDAP client libraries (default: $include_ldap): ";
   } else {
      print "Directory containing 'include' files of the Mozilla\n";
      print "LDAP Software Developer Kit (default: $include_ldap): ";
   }
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
   if ($use_openldap) {
      print "Directory containing 'lib' files of the OpenLDAP\n";
      print "LDAP client libraries (default: $lib_ldap): ";
   } else {
      print "Directory containing 'lib' files of the Mozilla\n";
      print "LDAP Software Developer Kit (default: $lib_ldap): ";
   }
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

if (!$use_openldap) {
   # see if ldap and nspr files are in the same place
   if ( -f "$include_ldap${dir_sep}nspr.h" ) {
      $nspr_loc = $ldapsdk_loc;
   }

   print "Directory containing NSPR API 'include' and 'lib'\n";
   print "directories for NSPR support (type 'n' or 'none' to omit) (default: $nspr_loc): ";
   if (!$ENV{NSPRDIR} && !$ENV{NSPRINCDIR} && !$ENV{NSPRLIBDIR}) {
      $silent = 0;
      chomp ($input = <STDIN>);
      if ($input =~ /^n/i) {
         $nspr_loc = undef;
      } elsif ($input =~ /\S/) {
          $nspr_loc = $input;
      }
   } else {
      print "$nspr_loc\n";
   }

   if ($nspr_loc) {
      if (!$lib_nspr) {
         $lib_nspr = $nspr_loc . ${dir_sep} . 'lib';
      }
      if (!$include_nspr) {
         $include_nspr = $nspr_loc . ${dir_sep} . 'include';
      }
      croak("Missing required include file $include_nspr${dir_sep}nspr.h") unless -f "$include_nspr${dir_sep}nspr.h";
      $pr_def = "-DPRLDAP"
   }

   print "Directory containing NSS API 'lib'\n";
   print "directories for NSS support (type 'n' or 'none' to omit) (default: $nss_loc): ";
   if (!$ENV{NSSDIR} && !$ENV{NSSLIBDIR}) {
      $silent = 0;
      chomp ($input = <STDIN>);
      if ($input =~ /^n/i) {
         $nss_loc = undef;
      } elsif ($input =~ /\S/) {
          $nss_loc = $input;
      }
   } else {
      print "$nss_loc\n";
   }

   if ($nss_loc) {
      if (!$lib_nss) {
         $lib_nss = $nss_loc . ${dir_sep} . 'lib';
      }
   }
}

opendir(DIR,$lib_ldap);
if (!$use_openldap && $ssl_def)
{
   @libfiles = grep{/ldap\d|ssl\d|nss\d|pl[cd]\d|nspr\d|ldappr\d/} readdir(DIR);
} else {
   @libfiles = grep{/ldap|lber/} readdir(DIR);
}
closedir(DIR);

if (!$use_openldap) {
   if ($lib_nspr && -d $lib_nspr) {
      opendir(DIR,$lib_nspr);
      push @libfiles, grep{/pl[cd]\d|nspr\d|ssl\d|nss\d/} readdir(DIR);
      closedir(DIR);
   }

   if ($lib_nss && -d $lib_nss) {
      opendir(DIR,$lib_nss);
      push @libfiles, grep{/ssl\d|nss\d/} readdir(DIR);
      closedir(DIR);
   }
}

@libs = ();
foreach $candidate (@libfiles) {
   next if $candidate !~ /($libexts)$/;
   $candidate =~ s/^lib//;
   if ($osname !~ /mswin/i) {
     $candidate =~ s/\.($libexts)$//;
   }
   push(@libs, $candidate) unless grep(/^$candidate$/, @libs);
}

if (!$use_openldap) {
   if ($osname =~ /mswin/i) {
      @ldaplib = grep{/^nsldap\d.*$/} @libs;
      @prldaplib = grep{/^nsldappr.*$/} @libs;
      @lberlib = grep{/^nslber.*$/} @libs;
      @ldapslib = grep{/^nsldapssl.*$/} @libs if $ssl_def;
   } else {
      @ldaplib = grep{/^ldap\d.*$/} @libs;
      @prldaplib = grep{/^prldap.*$/} @libs;
      @lberlib = grep{/^lber.*$/} @libs;
      @ldapslib = grep{/^ssldap.*$/} @libs if $ssl_def;
   }

   @ssllib = grep{/^ssl\d.*$/} @libs if $ssl_def;
   @nsslib = grep{/^nss\d.*$/} @libs if $ssl_def;
   @plclib = grep{/^plc\d.*$/} @libs if $ssl_def;
   @pldslib = grep{/^plds\d.*$/} @libs if $ssl_def;
   @nsprlib = grep{/^nspr\d.*$/} @libs;
} else {
   # We only need to link with libldap and lilber when using OpenLDAP.
   @ldaplib = grep{/^ldap\.*$/} @libs;
   @lberlib = grep{/^lber\.*$/} @libs;
   # We don't really use PRLDAP when using OpenLDAP, but we want to provide
   # stubs for the PRLDAP functions.
   $openldap_def = "-DUSE_OPENLDAP -DPRLDAP";
}


if ($#ldaplib < 0)
{
   die "No LDAP libraries found.";
}

if ($use_openldap && ($#lberlib < 0))
{
   die "No LBER libraries found.";
}

if ($#ldaplib > 0)
{
   print "Located multiple libraries:\n";
   foreach $alib (@ldaplib)
   {
      print " - $alib\n";
   }
}

# construct linker libraries string
$lline = "";

# add libraries in order of dependencies
$lline .= " -l$ldapslib[0]" if ($#ldapslib >= 0);

#
# Add "main" LDAP library to link line, and the NSPR version
# as well (if available).
#
$lline .= " -l$prldaplib[0]" if ($#prldaplib >= 0);
$lline .= " -l$ldaplib[0]";

#
# Add lber library if using OpenLDAP
#
if ($use_openldap)
{
   $lline .= " -l$lberlib[0]";
}

#
# Add SSL libraries, if needed
#
$lline .= " -L$lib_nss" if ($lib_nss);
$lline .= " -l$ssllib[0]" if ($#ssllib >= 0);
$lline .= " -l$nsslib[0]" if ($#nsslib >= 0);

# Add the NSS/NSPR libraries, if needed
$lline .= " -L$lib_nspr" if ($lib_nspr);
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
	      'DEFINE'		=> "$ssl_def $pr_def $openldap_def",
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
