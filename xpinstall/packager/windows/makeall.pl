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

GetOptions('help|h|?'              => \&PrintUsage,
           'objdir|o=s'            => \$topobjdir,
           'stagepath|s=s'         => \$inStagePath,
           'distpath|d=s'          => \$inDistPath,
           'archive-uri|aurl|a=s'  => \$inXpiURL,
           'redirect-uri|rurl|r=s' => \$inRedirIniURL);

$DEPTH = "../../..";
$topsrcdir = GetTopSrcDir();

# ensure that Packager.pm is in @INC, since we might not be called from
# mozilla/xpinstall/packager
push(@INC, "$topsrcdir/build/package");
require MozPackager;

if(defined($ENV{DEBUG_INSTALLER_BUILD})) {
  $MozPackager::verbosity++;
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
$inDistPath               = "$topobjdir/dist"            if !defined($inDistPath);
$inXpiURL                 = "ftp://not.supplied.invalid" if !defined($inXpiURL);
$inRedirIniURL            = $inXpiURL                    if !defined($inRedirIniURL);

# because we change directory to stage the XPI packages, make all paths absolute
$topobjdir   = File::Spec->rel2abs($topobjdir);
$inStagePath = File::Spec->rel2abs($inStagePath);
$inDistPath  = File::Spec->rel2abs($inDistPath);
  
if(defined($ENV{DEBUG_INSTALLER_BUILD}))
{
  print " windows/makeall.pl\n";
  print "   topobjdir  : $topobjdir\n";
  print "   topsrcdir  : $topsrcdir\n";
  print "   inStagePath: $inStagePath\n";
  print "   inDistPath : $inDistPath\n";
}

$gDefaultProductVersion   = MozPackager::GetProductY2KVersion($topobjdir, $topsrcdir, $topsrcdir);

print "\n";
print " Building Mozilla\n";
print "  Raw version id   : $gDefaultProductVersion\n";

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

print "  Display version  : $versionMain\n";
print "  Xpinstall version: $gDefaultProductVersion\n";
print "\n";

$gDirPackager         = "$topsrcdir/xpinstall/packager";
$gDirStageProduct     = "$inStagePath/mozilla";
$gDirDistInstall      = "$inDistPath/install";
$gDirDistInstGre      = "$inDistPath/inst_gre";

if (-d $gDirStageProduct) {
  rmtree($gDirStageProduct) || die("Error removing stage directory $gDirStageProduct.");
}
mkdir $gDirStageProduct, 0775;

if (-d "$gDirDistInstall/xpi") {
  rmtree("$gDirDistInstall/xpi") || die("Error removing XPI directory $gDirDistInstall/xpi");
}
mkdir "$gDirDistInstall/xpi", 0775;

# Build GRE installer package first before building Mozilla!  GRE installer is required by the mozilla installer.
MozPackager::system("perl -w \"$gDirPackager/win_gre/makeall.pl\" -objDir \"$topobjdir\" -stagePath \"$inStagePath\" -distPath \"$inDistPath\" -aurl $inXpiURL -rurl $inRedirIniURL");

if(defined($ENV{DEBUG_INSTALLER_BUILD}))
{
  print " back in windows/makeall.pl\n";
  print "   inStagePath: $inStagePath\n";
  print "   inDistPath : $inDistPath\n";
}

$ENV{XPI_VERSION}              = $gDefaultProductVersion;
$ENV{XPI_COMPANYNAME}          = "mozilla.org";
$ENV{XPI_PRODUCTNAME}          = "Mozilla";
# product name without the version string
$ENV{XPI_PRODUCTNAMEINTERNAL}  = "Mozilla";
$ENV{XPI_MAINEXEFILE}          = "Mozilla.exe";
$ENV{XPI_UNINSTALLFILE}        = $seuFileNameSpecific;
$ENV{XPI_UNINSTALLFILEZIP}     = $seuzFileNameSpecific;
$ENV{XPI_XPINSTALLVERSION}     = "$gDefaultProductVersion";

# The following variables are for displaying version info in the 
# the installer.
$ENV{XPI_USERAGENT}            = "$versionMain ($versionLanguage)";
$ENV{XPI_USERAGENTSHORT}       = "$versionMain";
$ENV{XPI_DISTINSTALLPATH}      = "$gDirDistInstall";

# GetProductBuildID() will return the build id for GRE located here:
#      NS_BUILD_ID in nsBuildID.h: 2003030610
$ENV{XPI_GREBUILDID}           = MozPackager::GetProductBuildID("$inDistPath/include/nsBuildID.h", "NS_BUILD_ID");

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

print "\n";
print " GRE build id       : $ENV{XPI_GREBUILDID}\n";
print " GRE file version   : $ENV{XPI_GREFILEVERSION}\n";
print " GRE special version: $ENV{XPI_GREUNIQUEID}\n";
print "\n";
print " Building $ENV{XPI_PRODUCTNAME} $ENV{XPI_USERAGENT}...\n";
print "\n";

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

$savedCwd = cwd();
chdir $topobjdir;

$dummyFile = "$gDirStageProduct/dummy.touch";
unlink $dummyFile if (-e $dummyFile);
system('touch', $dummyFile);
MozPackages::parsePackageList("$topsrcdir/build/package/packages.list");

foreach $package (keys %gPackages) {
  print "Making $gPackages{$package}.xpi\n";
  my $packageStageDir = "$gDirStageProduct/$gPackages{$package}";
  if (-d $packageStageDir) {
    rmtree($packageStageDir) ||
        die("Error removing stage directory $packageStageDir.");
  }
  my $packageDistPath = "$gDirDistInstall/xpi/$gPackages{$package}.xpi";

  my $parser = new MozParser;
  MozParser::XPTMerge::add($parser);
  MozParser::Touch::add($parser, $dummyFile);
  MozParser::Preprocess::add($parser);
  MozParser::Optional::add($parser);
  $parser->addMapping('dist/bin', 'bin');
  $parser->addMapping('xpiroot/', '');
  my $files = $parser->parse("$inDistPath/packages", MozPackages::getPackagesFor($package));
  MozStage::stage($files, $packageStageDir);
  MozParser::XPTMerge::mergeTo($parser, "$packageStageDir/bin/components/$package.xpt");
  MozPackager::calcDiskSpace($packageStageDir);
  MozParser::Preprocess::preprocessTo($parser, "$topsrcdir/config/preprocessor.pl", $packageStageDir);
  MozStage::makeXPI($packageStageDir, $packageDistPath);

  # We can't remove the stage, because makecfgini needs it to
  # compute space.
}

chdir $savedCwd;

# Copy the GRE installer to the Ns' stage area
if(!(-e "$gDirDistInstGre/$seiGreFileNameSpecific"))
{
  die "\"$gDirDistInstGre/$seiGreFileNameSpecific\": file missing\n";
}
mkdir "$gDirStageProduct/gre";
copy("$gDirDistInstGre/$seiGreFileNameSpecific", "$gDirStageProduct/gre") ||
  die "copy(\"$gDirDistInstGre/$seiGreFileNameSpecific\", \"$gDirStageProduct/gre\"): $!\n";

# Check for existence of staging path
if(!(-d "$gDirStageProduct"))
{
  die "\n Invalid path: $gDirStageProduct\n";
}

# Make sure gDirDistInstall exists
if(!(-d "$gDirDistInstall"))
{
  mkdir ("$gDirDistInstall",0775);
}

if(-d "$gDirDistInstall/uninstall")
{
  unlink <$gDirDistInstall/uninstall/*>;
}
else
{
  mkdir ("$gDirDistInstall/uninstall",0775);
}

if(-d "$gDirDistInstall/setup")
{
  unlink <$gDirDistInstall/setup/*>;
}
else
{
  mkdir ("$gDirDistInstall/setup",0775);
}

if(!(-e "$inDistPath/inst_gre/$seiGreFileNameSpecific"))
{
  die "\"$inDistPath/inst_gre/$seiGreFileNameSpecific\": file missing\n";
}
MakeExeZip("$inDistPath/inst_gre", $seiGreFileNameSpecific, $seizGreFileNameSpecific);

if(MakeUninstall())
{
  exit(1);
}
if(MakeConfigFile())
{
  exit(1);
}

# Copy the setup files to the dist setup directory.
copy("install.ini", "$gDirDistInstall") || 
  die "copy install.ini $gDirDistInstall: $!\n";
copy("install.ini", "$gDirDistInstall/setup") ||
  die "copy install.ini $gDirDistInstall/setup: $!\n";
copy("config.ini", "$gDirDistInstall") ||
  die "copy config.ini $gDirDistInstall: $!\n";
copy("config.ini", "$gDirDistInstall/setup") ||
  die "copy config.ini $gDirDistInstall/setup: $!\n";
copy("$gDirDistInstall/setup.exe", "$gDirDistInstall/setup") ||
  die "copy $gDirDistInstall/setup.exe $gDirDistInstall/setup: $!\n";
copy("$gDirDistInstall/setuprsc.dll", "$gDirDistInstall/setup") ||
  die "copy $gDirDistInstall/setuprsc.dll $gDirDistInstall/setup: $!\n";

# copy license file for the installer
copy("$topsrcdir/LICENSE", "$gDirDistInstall/license.txt") ||
  die "copy $topsrcdir/LICENSE $gDirDistInstall/license.txt: $!\n";
copy("$topsrcdir/LICENSE", "$gDirDistInstall/setup/license.txt") ||
  die "copy $topsrcdir/LICENSE $gDirDistInstall/setup/license.txt: $!\n";


# copy the lean installer to stub\ dir
print "\n****************************\n";
print "*                          *\n";
print "*  creating Stub files...  *\n";
print "*                          *\n";
print "****************************\n";
print "\n $gDirDistInstall/stub/$seiFileNameSpecificStub\n";

# build the self-extracting .exe (installer) file.
copy("$gDirDistInstall/$seiFileNameGeneric", "$gDirDistInstall/$seiFileNameSpecificStub") ||
  die "copy $gDirDistInstall/$seiFileNameGeneric $gDirDistInstall/$seiFileNameSpecificStub: $!\n";

MozPackager::system("$gDirDistInstall/nsztool.exe $gDirDistInstall/$seiFileNameSpecificStub $gDirDistInstall/setup/*.*");

if(-d "$gDirDistInstall/stub")
{
  unlink <$gDirDistInstall/stub/*>;
}
else
{
  mkdir ("$gDirDistInstall/stub",0775);
}
copy("$gDirDistInstall/$seiFileNameSpecificStub", "$gDirDistInstall/stub") ||
  die "copy $gDirDistInstall/$seiFileNameSpecificStub $gDirDistInstall/stub: $!\n";

# create the xpi for launching the stub installer
print "\n************************************\n";
print "*                                  *\n";
print "*  creating stub installer xpi...  *\n";
print "*                                  *\n";
print "************************************\n";
print "\n $gDirDistInstall/$seiStubRootName.xpi\n\n";

$savedCwd = cwd();
chdir $topobjdir;

$parser = new MozParser;
MozParser::Preprocess::add($parser);
$parser->addMapping('xpiroot/', '');
my $files = $parser->parse("$inDistPath/packages", MozPackages::getPackagesFor('seamonkey-stub-en-xpi'));
MozStage::stage($files, "$gDirStageProduct/$seiStubRootName");
MozPackager::calcDiskSpace("$gDirStageProduct/$seiStubRootName");
MozParser::Preprocess::preprocessTo($parser, "$topsrcdir/config/preprocessor.pl", "$gDirStageProduct/$seiStubRootName");
MozStage::makeXPI("$gDirStageProduct/$seiStubRootName", "$gDirDistInstall/$seiStubRootName.xpi");
# rmtree($packageStageDir);

chdir $savedCwd;

# group files for CD
print "\n************************************\n";
print "*                                  *\n";
print "*  creating Compact Disk files...  *\n";
print "*                                  *\n";
print "************************************\n";
print "\n $gDirDistInstall/cd\n";

if(-d "$gDirDistInstall/cd")
{
  unlink <$gDirDistInstall/cd/*>;
}
else
{
  mkdir ("$gDirDistInstall/cd",0775);
}

copy("$gDirDistInstall/$seiFileNameSpecificStub", "$gDirDistInstall/cd") ||
  die "copy $gDirDistInstall/$seiFileNameSpecificStub $gDirDistInstall/cd: $!\n";

@packageFiles = (map("$gDirDistInstall/xpi/$_.xpi", values(%gPackages)),
                 "$gDirDistInstall/xpi/gre-win32-installer.zip",
                 "$gDirDistInstall/xpi/mozillauninstall.zip");

foreach $packageFile (@packageFiles) {
  copy($packageFile, "$gDirDistInstall/cd") ||
    die "copy $packageFile $gDirDistInstall/cd: $!\n";
}

# create the big self extracting .exe installer
print "\n**************************************************************\n";
print "*                                                            *\n";
print "*  creating Self Extracting Executable Full Install file...  *\n";
print "*                                                            *\n";
print "**************************************************************\n";
print "\n $gDirDistInstall/$seiFileNameSpecific\n";

if(-d "$gDirDistInstall/sea")
{
  unlink <$gDirDistInstall/sea/*>;
}
else
{
  mkdir ("$gDirDistInstall/sea",0775);
}
copy("$gDirDistInstall/$seiFileNameGeneric", "$gDirDistInstall/$seiFileNameSpecific") ||
  die "copy $gDirDistInstall/$seiFileNameGeneric $gDirDistInstall/$seiFileNameSpecific: $!\n";

$packageFiles = join(' ', @packageFiles);

MozPackager::system("$gDirDistInstall/nsztool.exe $gDirDistInstall/$seiFileNameSpecific $gDirDistInstall/setup/*.* $packageFiles");

copy("$gDirDistInstall/$seiFileNameSpecific", "$gDirDistInstall/sea") ||
  die "copy $gDirDistInstall/$seiFileNameSpecific $gDirDistInstall/sea: $!\n";

unlink <$gDirDistInstall/$seiFileNameSpecificStub>;

print " done!\n\n";

# end of script
exit(0);

sub MakeExeZip
{
  my($aSrcDir, $aExeFile, $aZipFile) = @_;
  my($saveCwdir);

  $saveCwdir = cwd();
  chdir($aSrcDir);
  if(system("zip $gDirDistInstall/xpi/$aZipFile $aExeFile"))
  {
    chdir($saveCwdir);
    die "\n Error: zip $gDirDistInstall/xpi/$aZipFile $aExeFile";
  }
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

           -rurl <redirect.ini url>  : either ftp:// or http:// url to where the
                                       redirec.ini resides.  If not supplied, it
                                       will be assumed to be the same as archive
                                       url.
       \n";
}

sub MakeConfigFile
{
  chdir("$gDirPackager/windows");
  # Make config.ini file
  MozPackager::system("perl -w makecfgini.pl config.it $gDefaultProductVersion $gDirStageProduct $gDirDistInstall/xpi $inRedirIniURL $inXpiURL");

  # Make install.ini file
  MozPackager::system("perl makecfgini.pl install.it $gDefaultProductVersion $gDirStageProduct $gDirDistInstall/xpi $inRedirIniURL $inXpiURL");

  return(0);
}

sub MakeUninstall
{
  chdir("$gDirPackager/windows");
  if(MakeUninstallIniFile())
  {
    return(1);
  }

  # Copy the uninstall files to the dist uninstall directory.
  copy("uninstall.ini", "$gDirDistInstall") ||
    die "copy uninstall.ini $gDirDistInstall: $!\n";
  copy("uninstall.ini", "$gDirDistInstall/uninstall") ||
    die "copy uninstall.ini $gDirDistInstall/uninstall: $!\n";
  copy("defaults_info.ini", "$gDirDistInstall") ||
    die "copy defaults_info.ini $gDirDistInstall: $!\n";
  copy("defaults_info.ini", "$gDirDistInstall/uninstall") ||
    die "copy defaults_info.ini $gDirDistInstall/uninstall: $!\n";
  copy("$gDirDistInstall/uninstall.exe", "$gDirDistInstall/uninstall") ||
    die "copy $gDirDistInstall/uninstall.exe $gDirDistInstall/uninstall: $!\n";

  # build the self-extracting .exe (uninstaller) file.
  print "\nbuilding self-extracting uninstaller ($seuFileNameSpecific)...\n";
  copy("$gDirDistInstall/$seiFileNameGeneric", "$gDirDistInstall/$seuFileNameSpecific") ||
    die "copy $gDirDistInstall/$seiFileNameGeneric $gDirDistInstall/$seuFileNameSpecific: $!\n";
  if(system("$gDirDistInstall/nsztool.exe $gDirDistInstall/$seuFileNameSpecific $gDirDistInstall/uninstall/*.*"))
  {
    print "\n Error: $gDirDistInstall/nsztool.exe $gDirDistInstall/$seuFileNameSpecific $gDirDistInstall/uninstall/*.*\n";
    return(1);
  }

  MakeExeZip($gDirDistInstall, $seuFileNameSpecific, $seuzFileNameSpecific);
  unlink <$gDirDistInstall/$seuFileNameSpecific>;
  return(0);
}

sub MakeUninstallIniFile
{
  # Make config.ini file
  MozPackager::system("perl -w makeuninstallini.pl uninstall.it $gDefaultProductVersion");
  return(0);
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
