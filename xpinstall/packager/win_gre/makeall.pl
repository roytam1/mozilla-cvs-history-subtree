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
# This perl script builds the xpi and config.ini files.
#

use Cwd;
use File::Copy;
use File::Path;
use File::Basename;
use Getopt::Long;

$DEPTH = "../../..";
$topsrcdir = GetTopSrcDir();

push(@INC, "$topsrcdir/build/package");
require MozPackager;

GetOptions('help|h|?'              => \&PrintUsage,
           'objdir|o=s'            => \$topobjdir,
           'stagepath|s=s'         => \$inStagePath,
           'archive-uri|aurl|a=s'  => \$inXpiURL,
           'verbose|v+'            => \$MozPackager::verbosity);

$inStagePath             = "$topobjdir/stage"           if !defined($inStagePath);
$inXpiURL                = "ftp://not.supplied.invalid" if !defined($inXpiURL);

# because we change directory to stage the XPI packages, make all paths absolute
$topobjdir   = File::Spec->rel2abs($topobjdir);
$inStagePath = File::Spec->rel2abs($inStagePath);

chdir $topobjdir;

MozPackager::_verbosePrint(2,
"win_gre/makeall.pl
topobjdir  : $topobjdir
topsrcdir  : $topsrcdir
inStagePath: $inStagePath");

$gDefaultProductVersion  = MozPackager::GetProductY2KVersion($topobjdir, $topsrcdir, $topsrcdir);

MozPackager::_verbosePrint(1, "
Building GRE
Raw version id: $gDefaultProductVersion");

# $gDefaultProductVersion has the form maj.min.release.bld where maj, min, release
#   and bld are numerics representing version information.
# Other variables need to use parts of the version info also so we'll
#   split out the dot separated values into the array @versionParts
#   such that:
#
#   $versionParts[0] = maj
#   $versionParts[1] = min
#   $versionParts[2] = release
#   $versionParts[3] = bld
@versionParts = split /\./, $gDefaultProductVersion;

# We allow non-numeric characters to be included as the last 
#   characters in fields of $gDefaultProductVersion for display purposes (mostly to
#   show that we have moved past a certain version by adding a '+'
#   character).  Non-numerics must be stripped out of $gDefaultProductVersion,
#   however, since this variable is used to identify the the product 
#   for comparison with other installations, so the values in each field 
#   must be numeric only:
$gDefaultProductVersion =~ s/[^0-9.][^.]*//g;

# set environment vars for use by other .pl scripts called from this script.
if($versionParts[2] eq "0")
{
  $versionMain = "$versionParts[0].$versionParts[1]";
}
else
{
  $versionMain = "$versionParts[0].$versionParts[1].$versionParts[2]";
}

MozPackager::_verbosePrint(2, "
Display version  : $versionMain
Xpinstall version: $gDefaultProductVersion");

$gDirPackager         = "$topsrcdir/xpinstall/packager";
$gDirInstall          = "$topobjdir/dist/install";
$gDirDistGre          = "$topobjdir/dist/install-gre";
$gDirStageProduct     = "$inStagePath/gre";

$seiFileNameGeneric       = "nsinstall.exe";
$seiFileNameSpecific      = "gre-win32-installer.exe";
$seiStubRootName          = "gre-win32-stub-installer";
$seiFileNameSpecificStub  = "$seiStubRootName.exe";
$seuFileNameSpecific      = "GREUninstall.exe";
$seuzFileNameSpecific     = "greuninstall.zip";

$ENV{XPI_VERSION}              = $gDefaultProductVersion;
$ENV{XPI_COMPANYNAME}          = "mozilla.org";
$ENV{XPI_PRODUCTNAME}          = "GRE";
$ENV{XPI_PRODUCTNAMEINTERNAL}  = "GRE"; # product name without the version string
$ENV{XPI_MAINFILEEXE}          = "none.exe";
$ENV{XPI_UNINSTALLFILE}        = $seuFileNameSpecific;
$ENV{XPI_UNINSTALLFILEZIP}     = $seuzFileNameSpecific;

# The following variables are for displaying version info in the 
# the installer.
$ENV{XPI_USERAGENT}            = "$versionMain";
# userAgentShort just means it does not have the language string.
#  ie: '1.3b' as opposed to '1.3b (en)'
$ENV{XPI_USERAGENTSHORT}       = "$versionMain";
$ENV{XPI_XPINSTALLVERSION}     = "$gDefaultProductVersion";

# GetProductBuildID() will return the build id for GRE located here:
#      NS_BUILD_ID in nsBuildID.h: 2003030610
$ENV{XPI_GREBUILDID}       = MozPackager::GetProductBuildID("$topobjdir/dist/include/nsBuildID.h", "NS_BUILD_ID");

# GetGreFileVersion() will return the actual version of xpcom.dll used by GRE.
#  ie:
#      given milestone.txt : 1.4a
#      given nsBuildID.h   : 2003030610
#      gre version would be: 1.4.20030.30610
$ENV{XPI_GREFILEVERSION}       = MozPackager::GetGreFileVersion($topobjdir, $topsrcdir);

# GetProductBuildID() will return the GRE ID to be used in the windows registry.
# This ID is also the same one being querried for by the mozilla glue code.
#  ie:
#      given milestone.txt    : 1.4a
#      given nsBuildID.h      : 2003030610
#      gre special ID would be: 1.4a_2003030610
$ENV{XPI_GREUNIQUEID}          = MozPackager::GetProductBuildID("$topobjdir/dist/include/nsBuildID.h", "GRE_BUILD_ID");

MozPackager::_verbosePrint(1, "
GRE build id       : $ENV{XPI_GREBUILDID}
GRE file version   : $ENV{XPI_GREFILEVERSION}
GRE special version: $ENV{XPI_GREUNIQUEID}

Building $ENV{XPI_PRODUCTNAME} $ENV{XPI_USERAGENT}...");

# Check for existence of staging and install paths
MozPackager::makeDirEmpty($gDirStageProduct);
MozPackager::makeDirEmpty($gDirDistGre);
mkdir "$gDirDistGre/xpi", 0775 || die("Could not make directory '$gDirDistGre/xpi'");

# List of components for to create xpi files from
%gPackages = ('gre-xpi'                => 'gre',
              'xpi-bootstrap'          => 'xpcom');

$dummyFile = "$gDirStageProduct/dummy.touch";
unlink $dummyFile if (-e $dummyFile);
system('touch', $dummyFile);
MozPackages::parsePackageList("$topsrcdir/build/package/packages.list");

foreach $package (keys %gPackages) {
  MozPackager::_verbosePrint(1, "Making $gPackages{$package}.xpi");
  my $packageStageDir = "$gDirStageProduct/$gPackages{$package}";
  my $packageDistPath = "$gDirDistGre/xpi/$gPackages{$package}.xpi";

  my $parser = new MozParser;
  MozParser::XPTMerge::add($parser);
  MozParser::Touch::add($parser, $dummyFile);
  MozParser::Preprocess::add($parser);
  MozParser::Optional::add($parser);
  MozParser::Exec::add($parser);
  $parser->addMapping('dist/bin', 'bin');
  $parser->addMapping('xpiroot/', '');
  $parser->parse("$topobjdir/dist/packages", MozPackages::getPackagesFor($package));
  MozStage::stage($parser, $packageStageDir);
  MozParser::XPTMerge::mergeTo($parser, "$packageStageDir/bin/components/$package.xpt");
  MozPackager::calcDiskSpace($packageStageDir);
  MozParser::Preprocess::preprocessTo($parser, "$topsrcdir/config/preprocessor.pl", $packageStageDir);
  MozParser::Exec::exec($parser, $packageStageDir);
  MozStage::makeZIP($packageStageDir, $packageDistPath);

  # Don't remove the stage: makecfgini needs it to compute disk space
}

mkdir("$gDirDistGre/uninstall", 0775);
mkdir("$gDirDistGre/setup", 0775);

# Make config.ini file
MozPackager::system("perl -w $gDirPackager/makeuninstallini.pl $gDefaultProductVersion < $gDirPackager/win_gre/uninstall.it > $gDirDistGre/uninstall/uninstall.ini");

# Copy the uninstall files to the dist uninstall directory.
MozPackager::doCopy("$gDirPackager/win_gre/defaults_info.ini", "$gDirDistGre/uninstall");

# Get uninstaller from mozilla install
MozPackager::doCopy("$gDirInstall/uninstall.exe", "$gDirDistGre/uninstall");

# build the self-extracting .exe (uninstaller) file.
MozPackager::_verbosePrint(1, "Building self-extracting uninstaller ($seuFileNameSpecific).");
MozPackager::doCopy("$gDirInstall/$seiFileNameGeneric", "$gDirStageProduct/$seuFileNameSpecific");

MozPackager::system("$gDirInstall/nsztool.exe $gDirStageProduct/$seuFileNameSpecific $gDirDistGre/uninstall/*.*");

MakeExeZip($gDirStageProduct, $seuFileNameSpecific, "$gDirDistGre/xpi/$seuzFileNameSpecific");
unlink "$gDirStageProduct/$seuFileNameSpecific";

# Make config.ini file
MozPackager::system("perl -w $gDirPackager/makecfgini.pl $gDefaultProductVersion $gDirStageProduct $gDirDistGre/xpi $inXpiURL < $gDirPackager/win_gre/config.it > $gDirDistGre/setup/config.ini");

# Make install.ini file
MozPackager::system("perl -w $gDirPackager/makecfgini.pl $gDefaultProductVersion $gDirStageProduct $gDirDistGre/xpi $inXpiURL < $gDirPackager/win_gre/install.it > $gDirDistGre/setup/install.ini");

# Get setup binaries
MozPackager::doCopy("$gDirInstall/setup.exe", "$gDirDistGre/setup");
MozPackager::doCopy("$gDirInstall/setuprsc.dll", "$gDirDistGre/setup");

# copy license file for the installer
MozPackager::doCopy("$topsrcdir/LICENSE", "$gDirDistGre/setup/license.txt");

#
# create the self extracting stub installer
#
MozPackager::_verbosePrint(1, "Creating self-extracting stub installer: $gDirDistGre/$seiFileNameSpecificStub");

MozPackager::doCopy("$gDirInstall/$seiFileNameGeneric", "$gDirDistGre/$seiFileNameSpecificStub");
MozPackager::system("$gDirInstall/nsztool.exe $gDirDistGre/$seiFileNameSpecificStub $gDirDistGre/setup/*.*");

#
# create the big self extracting .exe installer
#
MozPackager::_verbosePrint(1, "Creating self-extracting stub installer: $gDirDistGre/$seiFileNameSpecific");

MozPackager::doCopy("$gDirInstall/$seiFileNameGeneric", "$gDirDistGre/$seiFileNameSpecific");
MozPackager::system("$gDirInstall/nsztool.exe $gDirDistGre/$seiFileNameSpecific $gDirDistGre/setup/*.* $gDirDistGre/xpi/*.*");

exit(0);

sub MakeExeZip
{
  my($aSrcDir, $aExeFile, $aZipFile) = @_;
  my($saveCwdir);

  my $qFlag = ($MozPackager::verbosity > 1) ? '' : '-q';

  $saveCwdir = cwd();
  chdir($aSrcDir);
  MozPackager::system("zip -9 $qFlag $aZipFile $aExeFile");
  chdir($saveCwdir);
}

sub PrintUsage
{
  die "usage: $0 [options]

       options include:

           -productVer <ver string>  : Version of the product.  By default it will acquire the
                                       version listed in mozilla/config/milestone.txt file.
                                         ie: 1.4a.0.0

           -stagePath <staging path> : Full path to where the GRE components are staged at
                                       Default path, if one is not set, is:
                                         [mozilla]/stage

           -distPath <dist path>     : Full path to where the GRE dist dir is at.
                                       Default path, if one is not set, is:
                                         [mozilla]/dist

           -aurl <archive url>       : either ftp:// or http:// url to where the
                                       archives (.xpi, .exe, .zip, etc...) reside

       \n";
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

