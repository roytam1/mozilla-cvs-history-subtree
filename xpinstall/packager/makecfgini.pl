#! /usr/bin/perl
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
#        Path to staging area
#             - path on where the seamonkey built bits are staged to
#
#        xpi path
#             - path on where xpi files will be located at
#
#        xpi url
#             - url to where the .xpi files will be staged at.
#               Either ftp:// or http:// can be used
#               ie: ftp://ftp.netscape.com/pub/seamonkey/xpi
#
#   ie: perl makecfgini.pl 5.0.0.1999120608 k:\windows\32bit\5.0 d:\builds\mozilla\dist\win32_o.obj\install\xpi ftp://ftp.netscape.com/pub/seamonkey/windows/32bit/x86/1999-09-13-10-M10/xpi
#
#

use Cwd;
use File::Spec;
use File::Basename;

# Make sure there are four arguments
if(scalar(@ARGV) != 4)
{
  die "usage: $0 <version> <staging path> <.xpi path> <redirect file url> <xpi url>

       version       : version to be shown in setup.  Typically the same version
                       as show in mozilla.exe.

       staging path  : path to where the components are staged at

       .xpi path     : path to where the .xpi files have been built to
                       ie: d:/builds/mozilla/dist/win32_o.obj/install/xpi

       xpi url       : url to where the .xpi files will be staged at.
                       Either ftp:// or http:// can be used
                       ie: ftp://ftp.netscape.com/pub/seamonkey/xpi

       This script should be run from the topobjdir.
       \n";
}

$inVersion        = $ARGV[0];
$inStagePath      = $ARGV[1];
$inXpiPath        = $ARGV[2];
$inUrl            = $ARGV[3];

$DEPTH = "../..";
$topsrcdir = GetTopSrcDir();
push(@INC, "$topsrcdir/build/package");
require MozPackager;

# get environment vars
$userAgent        = $ENV{XPI_USERAGENT};
$userAgentShort   = $ENV{XPI_USERAGENTSHORT};
$xpinstallVersion = $ENV{XPI_XPINSTALLVERSION};
$nameCompany      = $ENV{XPI_COMPANYNAME};
$nameProduct      = $ENV{XPI_PRODUCTNAME};
$nameProductInternal = $ENV{XPI_PRODUCTNAMEINTERNAL};
$fileMainExe      = $ENV{XPI_MAINEXEFILE};
$fileUninstall    = $ENV{XPI_UNINSTALLFILE};
$fileUninstallZip = $ENV{XPI_UNINSTALLFILEZIP};
$greBuildID       = $ENV{XPI_GREBUILDID};
$greFileVersion   = $ENV{XPI_GREFILEVERSION};
$greUniqueID      = $ENV{XPI_GREUNIQUEID};

# While loop to read each line from input file
while(defined($line = <STDIN>))
{
  # For each line read, search and replace $InstallSize$ with the calculated size
  if($line =~ /\$InstallSize\$/i)
  {
    my $installSize          = 0;
    my $installSizeSystem    = 0;

    # split read line by ":" deliminator
    @colonSplit = split(/:/, $line);
    if($#colonSplit >= 0)
    {
      $componentName    = $colonSplit[1];
      chop($componentName);

      if($componentName =~ /\$UninstallFileZip\$/i)
      {
        $installSize = OutputInstallSizeArchive("$inXpiPath/$fileUninstallZip") * 2;
      }
      else
      {
        $installSize = OutputInstallSize("$inStagePath/$componentName");

        # special oji consideration here.  Since it's an installer that 
        # seamonkey installer will be calling, the disk space allocation
        # needs to be adjusted by an expansion factor of 3.62.
        if($componentName =~ /oji/i)
        {
          $installSize = int($installSize * 3.62);
        }

        if($componentName =~ /gre/i)
        {
          $installSize = int($installSize * 4.48);
        }
      }
    }

    # Read the next line to calculate for the "Install Size System="
    if(defined($line = <STDIN>))
    {
      if($line =~ /\$InstallSizeSystem\$/i)
      {
        $installSizeSystem = OutputInstallSizeSystem($line, "$inStagePath/$componentName");
      }
      else
      {
          # if the line didn't match, go back and try the other matches
          seek STDIN, 0-length($line), 1;
      }
    }

    $installSize -= $installSizeSystem;
    print STDOUT "Install Size=$installSize\n";
    print STDOUT "Install Size System=$installSizeSystem\n"
        if ($installSizeSystem);
  }
  elsif($line =~ /\$InstallSizeArchive\$/i)
  {
    $installSizeArchive = 0;

    # split read line by ":" deliminator
    @colonSplit = split(/:/, $line);
    if($#colonSplit >= 0)
    {
      $componentName = $colonSplit[1];
      chop($componentName);
      $componentName      =~ s/\$UninstallFileZip\$/$fileUninstallZip/gi
          if ($fileUninstallZip);
      $installSizeArchive = OutputInstallSizeArchive("$inXpiPath/$componentName");
    }

    print STDOUT "Install Size Archive=$installSizeArchive\n";
  }
  else
  {
    # For each line read, search and replace $Version$ with the version passed in
    $line =~ s/\$Version\$/$inVersion/gi;
    $line =~ s/\$ArchiveUrl\$/$inUrl/gi;
    $line =~ s/\$UserAgent\$/$userAgent/gi;
    $line =~ s/\$UserAgentShort\$/$userAgentShort/gi;
    $line =~ s/\$XPInstallVersion\$/$xpinstallVersion/gi;
    $line =~ s/\$CompanyName\$/$nameCompany/gi;
    $line =~ s/\$ProductName\$/$nameProduct/gi;
    $line =~ s/\$ProductNameInternal\$/$nameProductInternal/gi;
    $line =~ s/\$MainExeFile\$/$fileMainExe/gi if ($fileMainExe);
    $line =~ s/\$UninstallFile\$/$fileUninstall/gi if ($fileUninstall);
    $line =~ s/\$UninstallFileZip\$/$fileUninstallZip/gi if ($fileUninstallZip);
    $line =~ s/\$GreBuildID\$/$greBuildID/gi if ($greBuildID);
    $line =~ s/\$GreFileVersion\$/$greFileVersion/gi if ($greFileVersion);
    $line =~ s/\$GreUniqueID\$/$greUniqueID/gi if ($greUniqueID);
    print STDOUT $line;
  }
}

# end of script
exit(0);

sub OutputInstallSize
{
  my($inPath) = @_;
  my($installSize);

  $installSize    = MozPackager::realDiskSpace($inPath);
  $installSize    = int($installSize / 1024);
  $installSize   += 1;
  return($installSize);
}

sub OutputInstallSizeArchive
{
  my($inPath) = @_;
  my($installSizeArchive);
  my($dev, $ino, $mode, $nlink, $uid, $gui, $rdev, $size, $atime, $mtime, $ctime, $blksize, $blocks);

  ($dev, $ino, $mode, $nlink, $uid, $gui, $rdev, $size, $atime, $mtime, $ctime, $blksize, $blocks) = stat $inPath;
  $installSizeArchive    = int($size / 1024);
  $installSizeArchive   += 1;
  return($installSizeArchive);
}

sub OutputInstallSizeSystem
{
  my($inLine, $inPath) = @_;
  my($installSizeSystem) = 0;

  # split read line by ":" deliminator
  @colonSplit = split(/:/, $inLine);
  if($#colonSplit >= 2)
  {
    # split line by "," deliminator
    @commaSplit = split(/\,/, $colonSplit[1]);
    if($#commaSplit >= 0)
    {
      foreach(@commaSplit)
      {
        # calculate the size of component installed using ds32.exe in Kbytes
        $installSizeSystem += MozPackager::realDiskSpace("$inPath/$_");
      }
    }
  }

  $installSizeSystem  = int($installSizeSystem / 1024);
  $installSizeSystem += 1;
  return($installSizeSystem);
}

sub GetTopSrcDir
{
  my($rootDir) = dirname($0) . "/$DEPTH";
  my($savedCwdDir) = cwd();

  chdir($rootDir);
  $rootDir = cwd();
  chdir($savedCwdDir);
  return($rootDir);
}
