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
# The Original Code is Mozilla packaging scripts.
#
# The Initial Developer of the Original Code is
# Benjamin Smedberg <bsmedberg@covad.net>.
# Portions created by the Initial Developer are Copyright (C) 2003
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
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

sub PrintUsage
{
    print
"stage-packages.pl

  Parses package list and package files, and copies (stages) the files
  for a set of packages to a staging area. It will optionally also
  create a tarball from the package.

Usage

  perl stage-packages.pl [options] packages...

  -c, --command-handler=handler[,handler...]
    Specify a comma-separated list of command handlers. Currently
    recognized handlers are xptmerge, xptdist, touch, preprocess,
    optional, exec, and staticcomp.

  -d, --compute-disk-space
    This sets environment variables before preprocessing based on
    the disk space used by the package(s). Used mainly by install.js
    scripts.
 
  --dmg-volume-name
    If '--make-package dmg=file' is specified, a dmg package name must
    be specified.

  -i, --ignore-missing
    Silently continue when a file listed in a package is missing. NOT
    RECOMMENDED. See -w

  -f, --force-copy
    By default, files will be symlinked instead of being copied, if
    that would not cause problems. This flags forces a true copy.

  --make-package packagetype=file
    After staging, this flag will make a tarball \"file\". Currently
    recognized package types are xpi, zip, tgz, bz2, and dmg.

  -m, --mapping sourcepath=destpath
    Specifies mappings for the staging process. A common mapping would
    be --mapping dist/bin=bin --mapping xpiroot/=

  -o, --objdir=path
    The build directory. Default is the source directory.

  -l, --package-list=file
    The packages list. Defaults to srcdir/build/package/packages.list

  -p, --packages-dir=path
    The directory of package files. Defaults to objdir/dist/packages

  -s, --stage-directory
    The directory to which files should be copied. This directory
    is cleared before use. Default is objdir/stage

  -v, --verbose
    Report more status information about actions being performed. This
    option may be listed up to three times.

  -w, --warn-missing
    Issue a warning when a file listed in a package is missing. The
    default behavior is to abort.

  -x, --xpt-merge-file=file
    If the xptmerge command handler is specified, this indicates
    the actual file resulting from the merge. The default places the
    merged XPT file into dist/bin/components in the package, using the
    name of the first package on the command line.\n";
    exit;
}

use File::Spec;
use File::Path;
use Getopt::Long;
use File::Basename;
use Cwd;

$DEPTH = "../..";
$topsrcdir = GetTopSrcDir();
push @INC, "$topsrcdir/build/package";
require MozPackager;

$preprocessor = "$topsrcdir/config/preprocessor.pl";
$packageList  = "$topsrcdir/build/package/packages.list";
$objdir       = $topsrcdir;

Getopt::Long::Configure ("bundling");

GetOptions("help|h|?"             => \&PrintUsage,
           "objdir|o=s"           => \$objdir,
           "package-list|l=s"     => \$packageList,
           "packages-dir|p=s"     => \$packagesDir,
           "warn-missing|w"       => \$warnMissing,
           "ignore-missing|i"     => \$ignoreMissing,
           "verbose|v+"           => \$MozPackager::verbosity,
           "command-handlers|c=s" => \@handlers,
           "xpt-merge-file|x=s"   => \$xptMergeFile,
           "stage-directory|s=s"  => \$stageDir,
           "mapping|m=s"          => \%mappings,
           "force-copy|f"         => \$MozPackager::forceCopy,
           "make-package=s"       => \%makePackages,
           "dmg-volume-name=s"    => \$dmgVolumeName,
           "compute-disk-space|d" => \$calcDiskSpace);

@handlers = split(/,/,join(',',@handlers));

if ($packagesDir) {
    $packagesDir = File::Spec->rel2abs($packagesDir);
} else {
    $packagesDir = File::Spec->catdir("dist", "packages")
}    
if ($stageDir) {
    $stageDir = File::Spec->rel2abs($stageDir);
} else {
    $stageDir = "stage";
}

foreach $packageType (keys %makePackages) {
    $makePackages{$packageType} = File::Spec->rel2abs($makePackages{$packageType});
}

chdir $objdir;
-d "dist" || die("directory dist/ not found... perhaps you forgot to specify --objdir?");

MozPackager::_verbosePrint(1, "removing $stageDir");
if (-d $stageDir) {
    rmtree($stageDir) || die ("Could not remove '$stageDir'");
}

$MozParser::missingFiles = 2 if $ignoreMissing;
$MozParser::missingFiles = 1 if $warnMissing;

MozPackages::parsePackageList($packageList);

my @packages;
foreach my $package (@ARGV) {
    push @packages, MozPackages::getPackagesFor($package);
    $xptMergeFile = "dist/bin/components/$package.xpt" if !$xptMergeFile;
}

my $parser = new MozParser;
my $xptMerge = 0;
my $preprocess = 0;
my $doExec = 0;
my $doTouch = 0;

HANDLER: foreach my $handler (@handlers) {
    if ($handler eq "xptmerge") {
        MozParser::XPTMerge::add($parser);
        $xptMerge = 1;
        next HANDLER;
    }
    if ($handler eq "xptdist") {
        MozParser::XPTDist::add($parser);
        next HANDLER;
    }
    if ($handler eq "touch") {
        my $dummyFile = File::Spec->catfile("dist", "dummy-file");
        MozParser::Touch::add($parser);
        $doTouch = 1;
        next HANDLER;
    }
    if ($handler eq "preprocess") {
        MozParser::Preprocess::add($parser);
        $preprocess = 1;
        next HANDLER;
    }
    if ($handler eq "optional") {
        MozParser::Optional::add($parser);
        next HANDLER;
    }
    if ($handler eq "exec") {
        MozParser::Exec::add($parser);
        $doExec = 1;
        next HANDLER;
    }
    if ($handler eq "staticcomp") {
        $parser->addCommand('staticcomp', \&MozParser::Ignore::ignoreFunc);
        next HANDLER;
    }
    die("Unrecognized command-handler $handler");
}

foreach my $mapping (keys %mappings) {
    $parser->addMapping($mapping, $mappings{$mapping});
}

$parser->parse($packagesDir, @packages);

MozStage::stage($parser, $stageDir);

if ($xptMerge) {
    MozParser::XPTMerge::mergeTo($parser, MozPackager::joinfile($stageDir, $parser->findMapping($xptMergeFile)));
}

if ($doTouch) {
    MozParser::Touch::touchTo($parser, $stageDir);
}

if ($calcDiskSpace) {
    MozPackager::calcDiskSpace($stageDir)
}

if ($preprocess) {
    MozParser::Preprocess::preprocessTo($parser, $preprocessor, $stageDir);
}

if ($doExec) {
    MozParser::Exec::exec($parser, $stageDir);
}

foreach $packageType (keys %makePackages) {
    my $packageFunc;
    if ($packageType eq 'xpi') {
        $packageFunc = \&MozStage::makeZIP;
    } elsif ($packageType eq 'zip') {
        $packageFunc = \&MozStage::makeZIP;
    } elsif ($packageType eq 'tgz') {
        $packageFunc = \&MozStage::makeTGZ;
    } elsif ($packageType eq 'bz2') {
        $packageFunc = \&MozStage::makeBZ2;
    } elsif ($packageType eq 'dmg') {
        die "--dmg-volume-name must be specified." if !defined($dmgVolumeName);
        $packageFunc = \&MozStage::makeDMG;
    } else {
        die "Unknown package type: '$packageType'";
    }

    &$packageFunc($stageDir, $makePackages{$packageType}, $topsrcdir, $dmgVolumeName);
}

sub GetTopSrcDir
{
    my $rootDir = File::Spec->catdir(dirname($0), $DEPTH);
    my $savedCwd = cwd();

    chdir $rootDir;
    $rootDir = cwd();
    chdir $savedCwd;
    return $rootDir;
}
