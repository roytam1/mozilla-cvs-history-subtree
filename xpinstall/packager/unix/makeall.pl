#!/usr/bin/perl -w
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
# Samir Gehani <sgehani@netscape.com>
# Benjamin Smedberg <bsmedberg@covad.net>
# 

#
# This perl script builds the xpi, config.ini, and js files.
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
           'stub-installer|b=s'    => \$inStubInstallName,
           'full-installer|f=s'    => \$inFullInstallName,
           'verbose|v+'            => \$MozPackager::verbosity);

$topobjdir                = "$topsrcdir"                 if !defined($topobjdir);
$inStagePath              = "$topobjdir/stage"           if !defined($inStagePath);
$inXpiURL                 = "ftp://not.supplied.invalid" if !defined($inXpiURL);
$inStubInstallName        = "mozilla-stub-installer"     if !defined($inStubInstallName);
$inFullInstallName        = "mozilla-full-installer"     if !defined($inFullInstallName);

$topobjdir   = File::Spec->rel2abs($topobjdir);
$inStagePath = File::Spec->rel2abs($inStagePath);

chdir $topobjdir;

$gStripCmd = 'strip ';
$gStripCmd .= '-f ' if ($^O eq 'irix');
$gStripCmd .= '-g ' if ($^O eq 'beos');

$gDefaultProductVersion   = MozPackager::GetProductY2KVersion($topobjdir, $topsrcdir, $topsrcdir);

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

MozPackager::_verbosePrint(1, "
Display version  : $versionMain
Xpinstall version: $gDefaultProductVersion");

$gDirPackager         = "$topsrcdir/xpinstall/packager";
$gDirDistProduct      = "$topobjdir/dist/install-seamonkey";
$gDirStageProduct     = "$inStagePath/seamonkey";

MozPackager::makeDirEmpty($gDirStageProduct);
MozPackager::makeDirEmpty($gDirDistProduct);

mkdir "$gDirDistProduct/xpi", 0775;
mkdir "$gDirDistProduct/setup", 0775;
mkdir "$gDirDistProduct/cd", 0775;

$ENV{XPI_VERSION}              = $gDefaultProductVersion;
$ENV{XPI_COMPANYNAME}          = "mozilla.org";
$ENV{XPI_PRODUCTNAME}          = "Mozilla";
# product name without the version string
$ENV{XPI_PRODUCTNAMEINTERNAL}  = "Mozilla";
$ENV{XPI_MAINEXEFILE}          = "Mozilla.exe";
$ENV{XPI_XPINSTALLVERSION}     = "$gDefaultProductVersion";

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
              'talkback-xpi'           => 'talkback',
              'xpi-bootstrap'          => 'xpcom');

MozPackages::parsePackageList("$topsrcdir/build/package/packages.list");

foreach $package (keys %gPackages) {
  MozPackager::_verbosePrint(1, "Making $gPackages{$package}.xpi");
  my $packageStageDir = "$gDirStageProduct/$gPackages{$package}";
  my $packageDistPath = "$gDirDistProduct/xpi/$gPackages{$package}.xpi";

  my $parser = new MozParser;
  MozParser::XPTMerge::add($parser);
  MozParser::Touch::add($parser);
  MozParser::Preprocess::add($parser);
  MozParser::Optional::add($parser);
  MozParser::Exec::add($parser);
  $parser->addCommand('staticcomp', \&MozParser::Ignore::ignoreFunc);
  $parser->addMapping('dist/bin', 'bin');
  $parser->addMapping('xpiroot/', '');
  my @packages = map(MozPackages::getPackagesFor($_), split(' ', $package));
  $parser->parse("$topobjdir/dist/packages", @packages);

  MozStage::stage($parser, $packageStageDir, $gStripCmd, 'so', '');
  MozParser::XPTMerge::mergeTo($parser, "$packageStageDir/bin/components/$package.xpt");
  MozParser::Touch::touchTo($parser, $packageStageDir); 
  MozPackager::calcDiskSpace($packageStageDir);
  MozParser::Preprocess::preprocessTo($parser, "$topsrcdir/config/preprocessor.pl", $packageStageDir);
  MozParser::Exec::exec($parser, $packageStageDir); 
  MozStage::makeZIP($packageStageDir, $packageDistPath);

  # We can't remove the stage, because makecfgini needs it to
  # compute space.
}

# Make the config.ini file
MozPackager::system("perl -w $gDirPackager/makecfgini.pl $gDefaultProductVersion $gDirStageProduct $gDirDistProduct/xpi $inXpiURL < $gDirPackager/unix/config.it > $gDirDistProduct/config.ini");

$stubStageDir = "$gDirDistProduct/setup";
$stubDistPath = "$gDirDistProduct/$inStubInstallName.tar.gz";

{
  my $parser = new MozParser;
  $parser->addMapping('installer/', '');
  $parser->parse("$topobjdir/dist/packages", MozPackages::getPackagesFor("seamonkey-installer-stub-en"));
  MozStage::stage($parser, $stubStageDir, $gStripCmd, 'so', '');
  MozStage::makeTGZ($stubStageDir, $stubDistPath);
}

$fullStageDir = "$gDirDistProduct/cd";
$fullDistPath = "$gDirDistProduct/$inFullInstallName.tar.gz";

{
  my $parser = new MozParser;
  $parser->addMapping('installer/', '');
  $parser->parse("$topobjdir/dist/packages", MozPackages::getPackagesFor("seamonkey-installer-en"));
  MozStage::stage($parser, $fullStageDir, $gStripCmd, 'so', '');
  MozStage::makeTGZ($fullStageDir, $fullDistPath);
}

# end of script
exit(0);

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
