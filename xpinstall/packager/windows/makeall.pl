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
# Benjamin Smedberg <bsmedberg@covad.net>
# 

#
# This perl script builds the xpi and config.ini files.
#

use Cwd;
use File::Copy;
use File::Path;
use File::Basename;
use Getopt::Long;

Getopt::Long::Configure ("bundling");

$DEPTH = "../../..";
$topsrcdir = GetTopSrcDir();

# ensure that Packager.pm is in @INC, since we might not be called from
# mozilla/xpinstall/packager
push(@INC, "$topsrcdir/build/package");
require MozPackager;

GetOptions('help|h|?'              => \&PrintUsage,
           'objdir|o=s'            => \$topobjdir,
           'stagepath|s=s'         => \$inStagePath,
           'archive-uri|aurl|a=s'  => \$inXpiURL,
           'verbose|v+'            => \$MozPackager::verbosity);

$verboseFlags = '';
if ($MozPackager::verbosity) {
    for(my $i = 0; $i < $MozPackager::verbosity; ++$i) {
        $verboseFlags .= ' -v ';
    }
}

$seiFileNameGeneric       = "nsinstall.exe";
$seiFileNameSpecific      = "mozilla-win32-installer.exe";
$seiStubRootName          = "mozilla-win32-stub-installer";
$seiFileNameSpecificStub  = "$seiStubRootName.exe";
$seuFileNameSpecific      = "MozillaUninstall.exe";
$seuzFileNameSpecific     = "mozillauninstall.zip";
$seiGreFileNameSpecific   = "gre-win32-installer.exe";
$seizGreFileNameSpecific  = "gre-win32-installer.zip";

$topobjdir                = "$topsrcdir"                 if !defined($topobjdir);
$inStagePath              = "$topobjdir/stage"           if !defined($inStagePath);
$inXpiURL                 = "ftp://not.supplied.invalid" if !defined($inXpiURL);

# because we change directory to stage the XPI packages, make all paths absolute
$topobjdir   = File::Spec->rel2abs($topobjdir);
$inStagePath = File::Spec->rel2abs($inStagePath);
  
chdir $topobjdir;

MozPackager::_verbosePrint(2,
"windows/makeall.pl
topobjdir  : $topobjdir
topsrcdir  : $topsrcdir
inStagePath: $inStagePath");

$gDefaultProductVersion   = MozPackager::GetProductY2KVersion($topobjdir, $topsrcdir, $topsrcdir);

MozPackager::_verbosePrint(1, "Raw version id: $gDefaultProductVersion");

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

# set environment vars for use by the preprocessor
if($versionParts[2] eq "0")
{
  $versionMain = "$versionParts[0].$versionParts[1]";
}
else
{
  $versionMain = "$versionParts[0].$versionParts[1].$versionParts[2]";
}

$versionLanguage               = "en";

MozPackager::_verbosePrint(2, "
Display version  : $versionMain
Xpinstall version: $gDefaultProductVersion");

$gDirPackager         = "$topsrcdir/xpinstall/packager";
$gDirInstall          = "$topobjdir/dist/install";
$gDirDistProduct      = "$topobjdir/dist/install-seamonkey";
$gDirDistGre          = "$topobjdir/dist/install-gre";
$gDirStageProduct     = "$inStagePath/seamonkey";

MozPackager::makeDirEmpty($gDirStageProduct);
MozPackager::makeDirEmpty($gDirDistProduct);

# Build GRE installer package first before building Mozilla!  GRE installer is required by the mozilla installer.
MozPackager::system("perl -w \"$gDirPackager/win_gre/makeall.pl\" -objDir \"$topobjdir\" -aurl $inXpiURL $verboseFlags");

MozPackager::_verbosePrint(2, "Done in win_gre/makeall.pl");

$ENV{XPI_VERSION}              = $gDefaultProductVersion;
$ENV{XPI_COMPANYNAME}          = "mozilla.org";
$ENV{XPI_PRODUCTNAME}          = "Mozilla";
# product name without the version string
$ENV{XPI_PRODUCTNAMEINTERNAL}  = "Mozilla";
$ENV{XPI_MAINEXEFILE}          = "Mozilla.exe";
$ENV{XPI_UNINSTALLFILE}        = $seuFileNameSpecific;
$ENV{XPI_UNINSTALLFILEZIP}     = $seuzFileNameSpecific;
$ENV{XPI_XPINSTALLVERSION}     = $gDefaultProductVersion;

# The following variables are for displaying version info in the 
# the installer.
$ENV{XPI_USERAGENT}            = "$versionMain ($versionLanguage)";
$ENV{XPI_USERAGENTSHORT}       = "$versionMain";

# GetProductBuildID() will return the build id for GRE located here:
#      NS_BUILD_ID in nsBuildID.h: 2003030610
$ENV{XPI_GREBUILDID}           = MozPackager::GetProductBuildID("$topobjdir/dist/include/nsBuildID.h", "NS_BUILD_ID");

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

MozPackager::_verbosePrint(2, "
GRE build id       : $ENV{XPI_GREBUILDID}
GRE file version   : $ENV{XPI_GREFILEVERSION}
GRE special version: $ENV{XPI_GREUNIQUEID}
Building $ENV{XPI_PRODUCTNAME} $ENV{XPI_USERAGENT}");

# Stage our XPI packages.
%gPackages = ('xpfe-browser-xpi'       => 'browser',
              'xpfe-lang-enUS-xpi'     => 'langenus',
              'xpfe-locale-US-xpi'     => 'regus',
              'xpfe-default-US-xpi'    => 'deflenus',
              'xpfe-mailnews-xpi'      => 'mail',
              'venkman-xpi'            => 'venkman',
              'inspector-xpi'          => 'inspector',
              'chatzilla-xpi'          => 'chatzilla',
              'spellcheck-enUS-xpi'    => 'spellcheck',
              'talkback-xpi'           => 'talkback',
              'xpi-bootstrap'          => 'xpcom');

$dummyFile = "$gDirStageProduct/dummy.touch";
unlink $dummyFile if (-e $dummyFile);
system('touch', $dummyFile);
MozPackages::parsePackageList("$topsrcdir/build/package/packages.list");

foreach $package (keys %gPackages) {
  MozPackager::_verbosePrint(2, "Making $gPackages{$package}.xpi");
  my $packageStageDir = "$gDirStageProduct/$gPackages{$package}";
  my $packageDistPath = "$gDirDistProduct/xpi/$gPackages{$package}.xpi";

  my $parser = new MozParser;
  MozParser::XPTMerge::add($parser);
  MozParser::Touch::add($parser, $dummyFile);
  MozParser::Preprocess::add($parser);
  MozParser::Optional::add($parser);
  MozParser::Exec::add($parser);
  $parser->addCommand('staticcomp', \&MozParser::Ignore::ignoreFunc);
  $parser->addMapping('dist/bin', 'bin');
  $parser->addMapping('xpiroot/', '');
  $parser->parse("$topobjdir/dist/packages", MozPackages::getPackagesFor($package));
  MozStage::stage($parser, $packageStageDir);
  MozParser::XPTMerge::mergeTo($parser, "$packageStageDir/bin/components/$package.xpt");
  MozPackager::calcDiskSpace($packageStageDir);
  MozParser::Preprocess::preprocessTo($parser, "$topsrcdir/config/preprocessor.pl", $packageStageDir);
  MozParser::Exec::exec($parser, $packageStageDir);
  MozStage::makeZIP($packageStageDir, $packageDistPath);

  # We can't remove the stage, because makecfgini needs it to
  # compute space.
}

# Copy the GRE installer to the Ns' stage area
mkdir("$gDirStageProduct/gre", 0775);
MozPackager::doCopy("$gDirDistGre/$seiGreFileNameSpecific", "$gDirStageProduct/gre");

mkdir "$gDirDistProduct/uninstall", 0775;
mkdir "$gDirDistProduct/setup", 0775;
mkdir "$gDirDistProduct/cd", 0775;

MakeExeZip($gDirDistGre, $seiGreFileNameSpecific, "$gDirDistProduct/xpi/$seizGreFileNameSpecific");

MozPackager::system("perl -w $gDirPackager/makeuninstallini.pl $gDefaultProductVersion < $gDirPackager/windows/uninstall.it > $gDirDistProduct/uninstall/uninstall.ini");

MozPackager::doCopy("$gDirPackager/windows/defaults_info.ini", "$gDirDistProduct/uninstall");
MozPackager::doCopy("$gDirInstall/uninstall.exe", "$gDirDistProduct/uninstall");

# build the self-extracting .exe (uninstaller) file.
MozPackager::_verbosePrint(1, "Building self-extracting uninstaller ($seuFileNameSpecific)");
MozPackager::doCopy("$gDirInstall/$seiFileNameGeneric", "$gDirStageProduct/$seuFileNameSpecific");
MozPackager::system("$gDirInstall/nsztool.exe ".
                    MozPackager::makeWinPath("$gDirStageProduct/$seuFileNameSpecific"). " ".
                    MozPackager::makeWinPath("$gDirDistProduct/uninstall/*.*"));

MakeExeZip($gDirStageProduct, $seuFileNameSpecific, "$gDirDistProduct/xpi/$seuzFileNameSpecific");

unlink "$gDirStageProduct/$seuFileNameSpecific";

# Make config.ini file
MozPackager::system("perl -w $gDirPackager/makecfgini.pl $gDefaultProductVersion $gDirStageProduct $gDirDistProduct/xpi $inXpiURL < $gDirPackager/windows/config.it > $gDirDistProduct/setup/config.ini");

# Make install.ini file
MozPackager::system("perl -w $gDirPackager/makecfgini.pl $gDefaultProductVersion $gDirStageProduct $gDirDistProduct/xpi $inXpiURL < $gDirPackager/windows/install.it > $gDirDistProduct/setup/install.ini");

# copy necessary setup files
MozPackager::doCopy("$gDirInstall/setup.exe", "$gDirDistProduct/setup");
MozPackager::doCopy("$gDirInstall/setuprsc.dll", "$gDirDistProduct/setup");
MozPackager::doCopy("$topsrcdir/LICENSE", "$gDirDistProduct/setup/license.txt");

MozPackager::_verbosePrint(1, "Creating stub installer $gDirDistProduct/$seiFileNameSpecificStub");

# build the self-extracting .exe (installer) file.
MozPackager::doCopy("$gDirInstall/$seiFileNameGeneric", "$gDirDistProduct/$seiFileNameSpecificStub");

MozPackager::system("$gDirInstall/nsztool.exe ".
                    MozPackager::makeWinPath("$gDirDistProduct/$seiFileNameSpecificStub"). " ".
                    MozPackager::makeWinPath("$gDirDistProduct/setup/*.*"));

# create the xpi for launching the stub installer
MozPackager::_verbosePrint(1, "Creating stub installer XPI $gDirDistProduct/$seiStubRootName.xpi");

{
  my $parser = new MozParser;
  MozParser::Preprocess::add($parser);
  $parser->addMapping('xpiroot/', '');
  $parser->parse("$topobjdir/dist/packages", MozPackages::getPackagesFor('seamonkey-installer-stub-en-xpi'));
  MozStage::stage($parser, "$gDirStageProduct/$seiStubRootName");
  MozPackager::calcDiskSpace("$gDirStageProduct/$seiStubRootName");
  MozParser::Preprocess::preprocessTo($parser, "$topsrcdir/config/preprocessor.pl", "$gDirStageProduct/$seiStubRootName");
  MozStage::makeZIP("$gDirStageProduct/$seiStubRootName", "$gDirDistProduct/$seiStubRootName.xpi");
}

MozPackager::_verbosePrint(1, "Staging compact disc files to $gDirDistProduct/cd");

foreach $file (<$gDirDistProduct/setup/*.*>, <$gDirDistProduct/xpi/*.*>) {
  MozPackager::doCopy($file, "$gDirDistProduct/cd");
}

MozPackager::_verbosePrint(1, "Creating full installer $gDirDistProduct/$seiFileNameSpecific");

MozPackager::doCopy("$gDirInstall/$seiFileNameGeneric", "$gDirDistProduct/$seiFileNameSpecific");

MozPackager::system("$gDirInstall/nsztool.exe ".
                    MozPackager::makeWinPath("$gDirDistProduct/$seiFileNameSpecific"). " ".
                    MozPackager::makeWinPath("$gDirDistProduct/cd/*.*"));

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

           -objDir <path>            : path to the objdir.  default is topsrcdir

           -stagePath <staging path> : full path to where the mozilla components are staged at
                                       Default stage path, if this is not set, is:
                                         [mozilla]/stage

           -distPath <dist path>     : full path to where the mozilla dist dir is at.
                                       Default stage path, if this is not set, is:
                                         [mozilla]/dist

           -aurl <archive url>       : either ftp:// or http:// url to where the
                                       archives (.xpi, .exe, .zip, etc...) reside

           -v                        : verbose. Will increase message output. May be repeated.
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
