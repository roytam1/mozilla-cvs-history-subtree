#!c:\perl\bin\perl
# 
# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#  
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#  
# The Original Code is Mozilla Communicator client code, released
# March 31, 1998.
# 
# The Initial Developer of the Original Code is Netscape
# Communications Corporation. Portions created by Netscape are
# Copyright (C) 1998-1999 Netscape Communications Corporation. All
# Rights Reserved.
# 
# Contributor(s): 
# Sean Su <ssu@netscape.com>
# 

#
# This perl script parses the input file for special variables
# in the format of $Variable$ and replace it with the appropriate
# value(s).
#
# Input: .it file
#             - which is a .ini template
#
#        version
#             - version to display on the blue background
#
#   ie: perl makeuninstallini.pl 6.0.0.1999120608
#
#

if (scalar(@ARGV) != 1)
{
  die "usage: $0 <version>

       version       : version to be shown in setup.  Typically the same version
                       as show in mozilla.exe.  This version string will be shown
                       on the title of the main dialog.

                     ie: perl makeuninstallini.pl 6.0.0.1999120608
                      or perl makeuninstallini.pl 6.0b2
       \n";
}

$inVersion        = $ARGV[0];

# get environment vars
$userAgent        = $ENV{XPI_USERAGENT};
$userAgentShort   = $ENV{XPI_USERAGENTSHORT};
$xpinstallVersion = $ENV{XPI_XPINSTALLVERSION};
$nameCompany      = $ENV{XPI_COMPANYNAME};
$nameProduct      = $ENV{XPI_PRODUCTNAME};
$nameProductInternal = $ENV{XPI_PRODUCTNAMEINTERNAL};
$fileMainExe      = $ENV{XPI_MAINEXEFILE};
$fileUninstall    = $ENV{XPI_UNINSTALLFILE};
$greBuildID       = $ENV{XPI_GREBUILDID};
$greFileVersion   = $ENV{XPI_GREFILEVERSION};
$greUniqueID      = $ENV{XPI_GREUNIQUEID};

# While loop to read each line from input file
while($line = <STDIN>)
{
  # For each line read, search and replace $Version$ with the version passed in
  $line =~ s/\$Version\$/$inVersion/gi;
  $line =~ s/\$UserAgent\$/$userAgent/gi;
  $line =~ s/\$UserAgentShort\$/$userAgentShort/gi;
  $line =~ s/\$XPInstallVersion\$/$xpinstallVersion/gi;
  $line =~ s/\$CompanyName\$/$nameCompany/gi;
  $line =~ s/\$ProductName\$/$nameProduct/gi;
  $line =~ s/\$ProductNameInternal\$/$nameProductInternal/gi;
  $line =~ s/\$MainExeFile\$/$fileMainExe/gi if ($fileMainExe);
  $line =~ s/\$UninstallFile\$/$fileUninstall/gi;
  $line =~ s/\$GreBuildID\$/$greBuildID/gi;
  $line =~ s/\$GreFileVersion\$/$greFileVersion/gi;
  $line =~ s/\$GreUniqueID\$/$greUniqueID/gi;
  print STDOUT $line;
}

# end of script
exit(0);

sub ParseUserAgentShort()
{
  my($aUserAgent) = @_;
  my($aUserAgentShort);

  @spaceSplit = split(/ /, $aUserAgent);
  if($#spaceSplit >= 0)
  {
    $aUserAgentShort = $spaceSplit[0];
  }

  return($aUserAgentShort);
}

