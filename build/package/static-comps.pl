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
"static-comps.pl

  Parses package list and package files, and outputs static module
  names and/or the libraries needed to link a static build.

Usage

  perl static-comps.pl [options] packages...

  -i, --libs=file
    Output the list of static component libs to 'file'.

  -m, --module-names=file
    Output the list of static component module names to 'file'.

  -o, --objdir=path
    The build directory. Default is the source directory.

  -l, --package-list=file
    The packages list. Defaults to srcdir/build/package/packages.list

  -p, --packages-dir=path
    The directory of package files. Defaults to objdir/dist/packages

  -v, --verbose
    Report more status information about actions being performed. This
    option may be listed up to three times.";

    exit 1;
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
           "verbose|v+"           => \$MozPackager::verbosity,
           "libs|i=s"             => \$libsFilename,
           "modules-names|m=s"    => \$modulesFilename);

if ($packagesDir) {
    $packagesDir = File::Spec->rel2abs($packagesDir);
} else {
    $packagesDir = File::Spec->catdir("dist", "packages")
}    

if (! ($libsFilename || $modulesFilename)) {
    PrintUsage();
}

if ($libsFilename) {
    open $libsFile, ">$libsFilename" ||
        die("Could not open '$libsFilename' for writing.");
}
if ($modulesFilename) {
    open $modulesFile, ">$modulesFilename" ||
        die("Could not open '$modulesFilename' for writing.");
}

chdir $objdir;
-d "dist" || die("directory dist/ not found... perhaps you forgot to specify --objdir?");

# At the point we're linking libs, we don't give a rat's ass
# about missing chrome files and whatnot.
$MozParser::missingFiles = 2;

MozPackages::parsePackageList($packageList);

my @packages;
foreach my $package (@ARGV) {
    push @packages, MozPackages::getPackagesFor($package);
    $xptMergeFile = "dist/bin/components/$package.xpt" if !$xptMergeFile;
}

my $parser = new MozParser;

$parser->addCommand('xpt',        \&MozParser::Ignore::ignoreFunc);
$parser->addCommand('touch',      \&MozParser::Ignore::ignoreFunc);
$parser->addCommand('ignore',     \&MozParser::Ignore::ignoreFunc);
MozParser::Optional::add($parser);
MozParser::Preprocess::add($parser);
MozParser::Exec::add($parser);
MozParser::StaticComp::add($parser);

$parser->addMapping("dist/bin", "dummy");
$parser->addMapping("approot");

$parser->parse($packagesDir, @packages);
@staticComps = MozParser::StaticComp::getComponents($parser);

foreach $staticComp (@staticComps) {
    print $libsFile "$staticComp->{'lib'}\n"
        if ($libsFile);
    print $modulesFile "$staticComp->{'name'}\n"
        if ($modulesFile);
}

close $libsFile if ($libsFile);
close $componentsFile if ($modulesFile);

exit 0;

sub GetTopSrcDir
{
    my $rootDir = File::Spec->catdir(dirname($0), $DEPTH);
    my $savedCwd = cwd();

    chdir $rootDir;
    $rootDir = cwd();
    chdir $savedCwd;
    return $rootDir;
}
