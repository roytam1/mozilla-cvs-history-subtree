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
"dump-packages.pl

  Dumps (for debugging purposes) all of the packages/package contents
  from the package files in dist/packages.

Usage

  perl make-packages.pl [options] packages...

  -i, --ignore-missing
    Silently continue when a file listed in a package is missing. NOT
    RECOMMENDED. See -w

  -o, --objdir=path
    The build directory. Default is the source directory.

  -l, --package-list=file
    The packages list. Defaults to srcdir/build/package/packages.list

  -p, --packages-dir=path
    The directory of package files. Defaults to objdir/dist/packages

  -u, --report-unpackaged
    Report files that are in dist/bin but are not in any packages.

  -v, --verbose
    Report more status information about actions being performed. This
    option may be listed up to three times.

  -w, --warn-missing
    Issue a warning when a file listed in a package is missing. The
    default behavior is to abort.\n";
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

$packageList = "$topsrcdir/build/package/packages.list";
$objdir      = $topsrcdir;

Getopt::Long::Configure ("bundling");

my $objdir = "";
my $packagesDir = "";
my $warnMissing = 0;
my $ignoreMissing = 0;

GetOptions("help|h|?"            => \&PrintUsage,
           "objdir|o=s"          => \$objdir,
           "package-list|l=s"    => \$packageList,
           "packages-dir|p=s"    => \$packagesDir,
           "warn-missing|w"      => \$warnMissing,
           "ignore-missing|i"    => \$ignoreMissing,
           "report-unpackaged|u" => \$reportUnpackaged,
           "verbose|v+"          => \$MozPackager::verbosity);

$packageList || die("Specify --package-list=");

$packageList = File::Spec->rel2abs($packageList);

if ($packagesDir) {
    $packagesDir = File::Spec->rel2abs($packagesDir);
} else {
    $packagesDir = File::Spec->catdir("dist", "packages")
}    

chdir $objdir if $objdir;
-d "dist" || die("directory dist/ not found... perhaps you forgot to specify --objdir?");

$MozParser::missingFiles = 2 if $ignoreMissing;
$MozParser::missingFiles = 1 if $warnMissing;

MozPackages::parsePackageList($packageList);

$\ = "\n";

foreach my $package (keys %MozPackages::packages) {
    print "Package: $package";
    print "Dependencies:";
    foreach my $dependency (@{$MozPackages::packages{$package}}) {
        print $dependency;
    }
    print "";
    my $parser = new MozParser;
    MozParser::XPTDist::add($parser);
    MozParser::Touch::add($parser, File::Spec->catfile("dist", "dummy.file"));
    MozParser::Preprocess::add($parser);
    MozParser::Optional::add($parser);
    MozParser::Ignore::add($parser);
    MozParser::Exec::add($parser);
    MozParser::StaticComp::add($parser);
    $parser->addCommand('error', \&MozParser::Ignore::ignoreFunc);
    $parser->addMapping("dist/bin", "dist/bin");
    $parser->addMapping("dist/lib", "dist/lib");
    $parser->addMapping("dist/include", "dist/include");
    $parser->addMapping("dist/idl", "dist/idl");
    $parser->addMapping("xpiroot", "xpiroot");
    $parser->addMapping("approot", "approot");
    $parser->addMapping("installer", "installer");
    $parser->parse($packagesDir, $package);

    print "Files:";
    my $files = $parser->{'files'};
    foreach my $result (keys %$files) {
        print "$files->{$result}\t$result";
    }
    print "";

    @staticComps = MozParser::StaticComp::getComponents($parser);
    print "Static Components: ". scalar(@staticComps);
    foreach my $staticComp (@staticComps) {
        print "$staticComp->{'name'} ($staticComp->{'lib'})";
    }
    print "";

    if ($reportUnpackaged) {
        %packaged = (%packaged,
                     map({$_ => 1} values(%$files)));
        push @ignores, MozParser::Ignore::getIgnoreList($parser);
    }
}

if ($reportUnpackaged) {
    print "Unpackaged files:";
    $alldist = `find dist/bin ! -type d`;
    @alldist = split /[\n\r]+/, $alldist;
    foreach $distFile (@alldist) {
        if (!exists(${packaged{$distFile}})
            && grep($distFile =~ /$_/, @ignores) == 0) {
            print $distFile;
        }
    }
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
