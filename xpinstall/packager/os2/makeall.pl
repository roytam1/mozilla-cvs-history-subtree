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

$seiFileNameGeneric       = "stubinstall.exe";
$seiFileNameGenericRes    = "stubinstall.res";
$seiFileNameSpecific      = "mozilla-os2-installer.exe";
$seiStubRootName          = "mozilla-os2-stub-installer";
$seiFileNameSpecificStub  = "$seiStubRootName.exe";
$seuFileNameSpecific      = "MozillaUninstall.exe";
$seuzFileNameSpecific     = "mozillauninstall.zip";
$seiGreFileNameSpecific   = "gre-os2-installer.exe";
$seizGreFileNameSpecific  = "gre-os2-installer.zip";

$topobjdir                = "$topsrcdir"                 if !defined($topobjdir);
$inStagePath              = "$topobjdir/stage"           if !defined($inStagePath);
$inXpiURL                 = "ftp://not.supplied.invalid" if !defined($inXpiURL);

# because we change directory to stage the XPI packages, make all paths absolute
$topobjdir   = File::Spec->rel2abs($topobjdir);
$inStagePath = File::Spec->rel2abs($inStagePath);
  
chdir $topobjdir;

MozPackager::_verbosePrint(2,
"os2/makeall.pl
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
$gDirStageProduct     = "$inStagePath/seamonkey";

MozPackager::makeDirEmpty($gDirStageProduct);
MozPackager::makeDirEmpty($gDirDistProduct);

mkdir "$gDirDistProduct/xpi", 0775;
mkdir "$gDirDistProduct/setup", 0775;
mkdir "$gDirDistProduct/cd", 0775;
mkdir "$gDirDistProduct/uninstall", 0775;

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

# Stage our XPI packages.
# Since we're not using a GRE yet, include the GRE files
# in browser stage/XPI
%gPackages = ('xpfe-browser-xpi gecko ^xpi-bootstrap' => 'browser',
              'xpfe-lang-enUS-xpi'     => 'langenus',
              'xpfe-locale-US-xpi'     => 'regus',
              'xpfe-default-US-xpi'    => 'deflenus',
              'xpfe-mailnews-xpi'      => 'mail',
              'psm-xpi'                => 'psm',
              'venkman-xpi'            => 'venkman',
              'inspector-xpi'          => 'inspector',
              'chatzilla-xpi'          => 'chatzilla',
              'spellcheck-enUS-xpi'    => 'spellcheck',
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
  $parser->addMapping('dist/bin', 'bin');
  $parser->addMapping('xpiroot/', '');
  my @packages = map(MozPackages::getPackagesFor($_), split(' ', $package));
  $parser->parse("$topobjdir/dist/packages", @packages);

  MozStage::stage($parser, $packageStageDir);
  MozParser::XPTMerge::mergeTo($parser, "$packageStageDir/bin/components/$package.xpt");
  MozPackager::calcDiskSpace($packageStageDir);
  MozParser::Preprocess::preprocessTo($parser, "$topsrcdir/config/preprocessor.pl", $packageStageDir);
  MozParser::Exec::exec($parser, $packageStageDir);
  MozStage::makeZIP($packageStageDir, $packageDistPath);

  # We can't remove the stage, because makecfgini needs it to
  # compute space.
}

MozPackager::system("perl -w $gDirPackager/makeuninstallini.pl $gDefaultProductVersion < $gDirPackager/os2/uninstall.it > $gDirDistProduct/uninstall/uninstall.ini");

#MozPackager::doCopy("$gDirPackager/os2/defaults_info.ini", "$gDirDistProduct/uninstall");
MozPackager::doCopy("$gDirInstall/uninstall.exe", "$gDirDistProduct/uninstall");

# build the self-extracting .exe (uninstaller) file.
MozPackager::_verbosePrint(1, "Building self-extracting uninstaller ($seuFileNameSpecific)");
MozPackager::doCopy("$gDirInstall/$seiFileNameGeneric", "$gDirStageProduct/$seuFileNameSpecific");
#MozPackager::system("$gDirInstall/nsztool.exe $gDirStageProduct/$seuFileNameSpecific $gDirDistProduct/uninstall/*.*");
addFileResources("$gDirStageProduct/$seuFileNameSpecific",
                 "$gDirDistProduct/uninstall");

MakeExeZip($gDirStageProduct, $seuFileNameSpecific, "$gDirDistProduct/xpi/$seuzFileNameSpecific");

unlink "$gDirStageProduct/$seuFileNameSpecific";

# Make config.ini file
print "XXXXX perl -w $gDirPackager/makecfgini.pl $gDefaultProductVersion $gDirStageProduct $gDirDistProduct/xpi $inXpiURL < $gDirPackager/os2/config.it > $gDirDistProduct/setup/config.ini";
MozPackager::system("perl -w $gDirPackager/makecfgini.pl $gDefaultProductVersion $gDirStageProduct $gDirDistProduct/xpi $inXpiURL < $gDirPackager/os2/config.it > $gDirDistProduct/setup/config.ini");

# Make install.ini file
MozPackager::system("perl -w $gDirPackager/makecfgini.pl $gDefaultProductVersion $gDirStageProduct $gDirDistProduct/xpi $inXpiURL < $gDirPackager/os2/install.it > $gDirDistProduct/setup/install.ini");

# copy necessary setup files
MozPackager::doCopy("$gDirInstall/setup.exe", "$gDirDistProduct/setup");
MozPackager::doCopy("$gDirInstall/setuprsc.dll", "$gDirDistProduct/setup");
MozPackager::doCopy("$topsrcdir/LICENSE", "$gDirDistProduct/setup/license.txt");

MozPackager::_verbosePrint(1, "Creating stub installer $gDirDistProduct/$seiFileNameSpecificStub");

# build the self-extracting .exe (installer) file.
MozPackager::doCopy("$gDirInstall/$seiFileNameGeneric", "$gDirDistProduct/$seiFileNameSpecificStub");

#MozPackager::system("$gDirInstall/nsztool.exe $gDirDistProduct/$seiFileNameSpecificStub $gDirDistProduct/setup/*.*");
addFileResources("$gDirDistProduct/$seiFileNameSpecificStub",
                 "$gDirDistProduct/setup");

# create the xpi for launching the stub installer
MozPackager::_verbosePrint(1, "Creating stub installer XPI $gDirDistProduct/$seiStubRootName.xpi");

$parser = new MozParser;
MozParser::Preprocess::add($parser);
$parser->addMapping('xpiroot/', '');
$parser->parse("$topobjdir/dist/packages", MozPackages::getPackagesFor('seamonkey-installer-stub-en-xpi'));
MozStage::stage($parser, "$gDirStageProduct/$seiStubRootName");
MozPackager::calcDiskSpace("$gDirStageProduct/$seiStubRootName");
MozParser::Preprocess::preprocessTo($parser, "$topsrcdir/config/preprocessor.pl", "$gDirStageProduct/$seiStubRootName");
MozStage::makeZIP("$gDirStageProduct/$seiStubRootName", "$gDirDistProduct/$seiStubRootName.xpi");

MozPackager::_verbosePrint(1, "Staging compact disc files to $gDirDistProduct/cd");

mkdir("$gDirDistProduct/cd", 0775);

foreach $file (<$gDirDistProduct/setup/*.*>, <$gDirDistProduct/xpi/*.*>) {
  MozPackager::doCopy($file, "$gDirDistProduct/cd");
}

MozPackager::_verbosePrint(1, "Creating full installer $gDirDistProduct/$seiFileNameSpecific");

MozPackager::doCopy("$gDirInstall/$seiFileNameGeneric", "$gDirDistProduct/$seiFileNameSpecific");

#MozPackager::system("$gDirInstall/nsztool.exe $gDirDistProduct/$seiFileNameSpecific $gDirDistProduct/setup/*.* $gDirDistProduct/xpi/*.*");
addFileResources("$gDirDistProduct/$seiFileNameSpecific",
                 "$gDirDistProduct/cd");

exit(0);

sub addFileResources {
  my ($binary, $stubDir) = @_;

  my @stubFiles = <$stubDir/*.*>;
  die("When creating executable $binary: no files found in $stubDir")
      if (! scalar(@stubFiles));

  my $resStageDir = "$gDirStageProduct/makeres";

  MozPackager::makeDirEmpty($resStageDir);
  open my $oFile, ">$resStageDir/stubfiles.rc";
  print $oFile "#include <os2.h>\n";
  print $oFile "STRINGTABLE DISCARDABLE\n";
  print $oFile "BEGIN\n";

  $currentResourceID = 10000+1;
  foreach $entry ( @stubFiles ) 
  {
    $filename = basename($entry);
    print $oFile "$currentResourceID \"$filename\"\n";
    $currentResourceID++;
  }
  print $oFile "END\n";
  $currentResourceID = 10000+1;
  foreach $entry ( @stubFiles ) 
  {
    print $oFile "RESOURCE RT_RCDATA $currentResourceID \"$entry\"\n";
    $currentResourceID++;
  }
  close($oFile);
  system("rc -r $resStageDir/stubfiles.rc $resStageDir/stubfiles.res");

  # we need to chomp off the final byte of stubinstall.res and join it with our stubfiles.res
  MozPackager::doCopy("$gDirInstall/$seiFileNameGenericRes", $resStageDir);
  my $resSize = -s "$resStageDir/$seiFileNameGenericRes";
  truncate("$resStageDir/$seiFileNameGenericRes", $resSize - 1);

  MozPackager::system("cat $resStageDir/$seiFileNameGenericRes $resStageDir/stubfiles.res > $resStageDir/complete.res");

  system("rc $resStageDir/complete.res $binary");
}

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
