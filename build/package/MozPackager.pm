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

use strict;
use warnings;
use File::Spec;

package MozPackager;

use File::Copy;
use File::Path;
use Cwd;

# this is a wrapper around File::Spec->catfile that splits on '/'
# and rejoins the path
sub joinfile {
    return File::Spec->catfile(map(split('/', $_), @_));
}

sub joindir {
    return File::Spec->catdir(map(split('/', $_), @_));
}

# Performs a copy and dies on failure.
sub doCopy {
    my ($from, $to) = @_;

    symCopy($from, $to, 1);
}

# Executes a system call and dies if the call fails.
sub system {
    if (scalar(@_) > 1) {
        die("MozPackager::system() called with more than one argument!");
    }
    CORE::system($_[0]) && die("Error executing '$_[0]': code ". ($?>>8));
}

sub makeWinPath {
    my ($cygpath) = @_;

    my $realpath = $cygpath;
    if ($^O eq 'cygwin' && $cygpath =~ m|^/|) {
        $realpath =~ s/\*/\\\*/g;
        $realpath = `cygpath -m $realpath`;
        chop $realpath;
        die("Could not get windows path (cygpath -m) of $cygpath, code ". ($? >>8)) if ($? || !$realpath);
    }

    return $realpath;
}

# level 0, no progress
# level 1, basic "parsing file blah"
# level 2, more detail
# level 3, graphic detail
$MozPackager::verbosity = 0;

sub _verbosePrint {
    my $vlevel = shift;
    local $\ = "\n";
    print STDOUT @_ if ($vlevel <= $MozPackager::verbosity);
}

my $cansymlink = eval {symlink('', ''); 1; };

# global var may be set by clients who want to force a copy
# instead of a symlink
$MozPackager::forceCopy = 0;

# this is very much like File::Path::mkpath(), except that the last element
# is a file, which is stripped off first
sub ensureDirs {
    my ($to, $permissions) = @_;

    $permissions = 0775 if (!$permissions);

    my ($volume, $dirs, $file) = File::Spec->splitpath($to);
    my @dirs = File::Spec->splitdir($dirs);
    
    my $dirPath = File::Spec->catpath($volume, $dirs, '');
    mkpath($dirPath, 0, $permissions);
}

sub makeDirEmpty {
    my ($dir, $permissions) = @_;

    $permissions = 0775 if (!$permissions);

    rmtree($dir) if (-d $dir);
    unlink($dir) if (-f $dir);
    mkpath($dir, 0, $permissions);
}

# copies files in a directory to another location
sub copyFiles {
    my($aSrc, $aDestDir) = @_;

    ensureDirs($aDestDir);

    if (-d $aSrc) {
        opendir(my $sDir, $aSrc) || die("Couldn't open directory '$aSrc'");

        while(my $file = readdir($sDir)) {
            (warn("Recursive copy not implemented."), next)
                if (-d "$aSrc/$file");
            symcopy("$aSrc/$file", "$aDestDir/$file");
        }
        closedir $sDir;
    } elsif (-f $aSrc) {
        my ($vol, $dirs, $leaf) = File::Spec->splitpath($aSrc);
        symcopy($aSrc, "$aDestDir/$leaf");
    } else {
        die "File not found: $aSrc";
    }
}

# performs a symlink if possible on this system, otherwise
# peforms a regular "copy". It creates the directory structure if
# necessary.
sub symCopy {
    my ($from, $to, $forceCopy) = @_;

    $from = File::Spec->canonpath($from);
    $to   = File::Spec->canonpath($to);

    MozPackager::_verbosePrint(1, "Copying $from\t to $to");

    ensureDirs($to);

    if ((!$MozPackager::forceCopy) && (!$forceCopy) && $cansymlink) {
        # make $from absolute... apparently abs2rel doesn't work correctly when there are ../
        # elements of the path
        my $absfrom = File::Spec->rel2abs($from);

        MozPackager::_verbosePrint(1, "Symlinking $absfrom, $to");
        
        symlink($absfrom, $to) ||
            die("symlink $absfrom, $to failed, code $!");
    } else {
        copy($from, $to) ||
            die("copy $from, $to failed, code $!");
        my $perms = (stat($from))[2] & 07777;
        chmod $perms, $to || die("Could not chmod $to");;
    }
}

sub calcDiskSpace {
    my ($stageDir) = @_;

    MozPackager::_verbosePrint(1, "Calculating disk space for XPI package.");
    $ENV{'XPI_SPACEREQUIRED'} = int(realDiskSpace($stageDir) / 1024) + 1;
    
    opendir(my $dirHandle, $stageDir) ||
        die("Could not open directory $stageDir for listing.");

    while (my $dir = readdir($dirHandle)) {
        if ( (! ($dir =~ /^\./)) && -d "$stageDir/$dir") {
            MozPackager::_verbosePrint(1, "Calculating disk space for subdir $dir");
            my $realDir = File::Spec->catdir($stageDir, $dir);
            if (-d $realDir) {
                $ENV{"XPI_SPACEREQUIRED_\U$dir"} = int(realDiskSpace($realDir) / 1024) + 1;
            }
        }
    }
    closedir($dirHandle);
}

sub realDiskSpace {
    my ($path) = @_;

    my $dsProg = File::Spec->catfile('dist', 'install', 'ds32.exe');

    my $spaceRequired = 0;

    if (-e $dsProg) {
        # We're on win32, use ds32.exe
        my $command = "$dsProg /D /L0 /A /S /C 32768 $path";
        $spaceRequired = `$command`;
        die("Program failed: '$command' returned code ". ($? <<8))
            if ($?);
    } else {
        opendir(my $dirHandle, $path) ||
            die("Could not open directory $path for listing.");

        while (my $dir = readdir($dirHandle)) {
            next if ($dir eq '.' || $dir eq '..');
            my $realDir = File::Spec->catdir($path, $dir);
            if (-d $realDir) {
                $spaceRequired += realDiskSpace($realDir);
            } else {
                $spaceRequired += -s File::Spec->catfile($path, $dir);
            }
        }
        closedir($dirHandle);
    }

    return $spaceRequired;
}

# To retrieve a build id ($aDefine) from $aBuildIDFile (normally
# $topobjdir/dist/include/nsBuildID.h).
sub GetProductBuildID
{
  my($aBuildIDFile, $aDefine) = @_;
  my($line);
  my($buildID);
  my($fpInIt);

  if(defined($ENV{DEBUG_INSTALLER_BUILD}))
  {
    print " GetProductBuildID\n";
    print "   aBuildIDFile  : $aBuildIDFile\n";
    print "   aDefine       : $aDefine\n";
  }

  if(!(-e $aBuildIDFile))
  {
    die "\n file not found: $aBuildIDFile\n";
  }

  # Open the input file
  open($fpInIt, $aBuildIDFile) || die "\n open $aBuildIDFile for reading: $!\n";

  # While loop to read each line from input file
  while($line = <$fpInIt>)
  {
    if($line =~ /#define.*$aDefine/)
    {
      $buildID = $line;
      chop($buildID);

      # only get the build id from string, excluding spaces
      $buildID =~ s/..*$aDefine\s+//;
      # strip out any quote characters
      $buildID =~ s/[\"\s]//g;
    }
  }
  close($fpInIt);
  return($buildID);
}

# GetGreFileVersion()
#   To build GRE's file version as follows:
#     * Use mozilla's milestone version for 1st 2 numbers of version x.x.x.x.
#     * Strip out any non numerical chars from mozilla's milestone version.
#     * Get the y2k ID from $topobjdir/dist/include/nsBuildID.h.
#     * Split the y2k ID exactly in 2 equal parts and use them for the last
#       2 numbers of the version x.x.x.x.
#         ie: y2k: 2003030510
#             part1: 20030
#             part2: 30510
#
#  XXX  XXX: Problem with this format! It won't work for dates > 65536.
#              ie: 20030 71608 (July 16, 2003 8am)
#
#            mozilla/config/version_win.pl has the same problem because these
#            two code need to be in sync with each other.
#
#     * Build the GRE file version given a mozilla milestone version of "1.4a"
#         GRE version: 1.4.20030.30510
sub GetGreFileVersion
{
  my($aDirTopObj, $aDirMozTopSrc)       = @_;
  my($fileBuildID)                      = "$aDirTopObj/dist/include/nsBuildID.h";

  if(defined($ENV{DEBUG_INSTALLER_BUILD}))
  {
    print " GetGreFileVersion\n";
    print "   aDirTopObj    : $aDirTopObj\n";
    print "   aDirMozTopSrc : $aDirMozTopSrc\n";
    print "   fileBuildID   : $fileBuildID\n";
  }

  my($initEmptyValues)                  = 1;
  my(@version)                          = undef;
  my($y2kDate)                          = undef;
  my($buildID_hi)                       = undef;
  my($buildID_lo)                       = undef;
  my($versionMilestone)                 = GetProductMilestoneVersion($aDirTopObj, $aDirMozTopSrc, $aDirMozTopSrc, $initEmptyValues);

  $versionMilestone =~ s/[^0-9.][^.]*//g; # Strip out non numerical chars from versionMilestone.
  @version          = split /\./, $versionMilestone;
  $y2kDate          = GetProductBuildID($fileBuildID, "NS_BUILD_ID");

  # If the buildID is 0000000000, it means that it's a non release build.
  # This also means that the GRE version (xpcom.dll's version) will be
  # 0.0.0.0.  We need to match this version for install to proceed
  # correctly.
  if($y2kDate eq "0000000000")
  {
    return("0.0.0.0");
  }

  my $length = length($y2kDate);
  print STDERR "y2kdate: length($length) = $y2kDate\n";

  $buildID_hi       = substr($y2kDate, 0, 5);
  $buildID_hi       =~ s/^0+//; # strip out leading '0's
  $buildID_hi       = 0 if($buildID_hi eq undef); #if buildID_hi happened to be all '0's, then set it to '0'
  $buildID_lo       = substr($y2kDate, 5);
  $buildID_lo       =~ s/^0+//; # strip out leading '0's
  $buildID_lo       = 0 if($buildID_lo eq undef); #if buildID_hi happened to be all '0's, then set it to '0'

  return("$version[0].$version[1].$buildID_hi.$buildID_lo");
}

# Retrieves the product's milestone version (ns or mozilla):
#   ie: "1.4a.0.0"
#
# If the milestone version is simply "1.4a", this function will prefil
# the rest of the 4 unit ids with '0':
#   ie: "1.4a" -> "1.4a.0.0"
#
# The milestone version is acquired from [topsrcdir]/config/milestone.txt
sub GetProductMilestoneVersion
{
  my($aDirTopObj, $aDirMozTopSrc, $aDirConfigTopSrc, $initEmptyValues) = @_;
  my($y2kDate)                          = undef;
  my($versionMilestone)                 = undef;
  my($counter)                          = undef;
  my(@version)                          = undef;
  my($saveCwd)                          = cwd();

  if(defined($ENV{DEBUG_INSTALLER_BUILD}))
  {
    print " GetProductMileStoneVersion\n";
    print "   aDirTopObj      : $aDirTopObj\n";
    print "   aDirMozTopSrc   : $aDirMozTopSrc\n";
    print "   aDirConfigTopSrc: $aDirConfigTopSrc\n";
  }

  chdir("$aDirMozTopSrc/config");
  $versionMilestone = `perl milestone.pl --topsrcdir $aDirConfigTopSrc`;

  if(defined($ENV{DEBUG_INSTALLER_BUILD}))
  {
    print "   versionMilestone: $versionMilestone\n";
  }

  chop($versionMilestone);
  chdir($saveCwd);

  if(defined($initEmptyValues) && ($initEmptyValues eq 1))
  {
    @version = split /\./, $versionMilestone;

    # Initialize any missing version blocks (x.x.x.x) to '0'
    for($counter = $#version + 1; $counter <= 3; $counter++)
    {
      $version[$counter] = "0";
    }
    return("$version[0].$version[1].$version[2].$version[3]");
  }
  return($versionMilestone);
}

# Retrieves the products's milestone version from either the ns tree or the
# mozilla tree.
#
# However, it will use the y2k compliant build id only from:
#   .../mozilla/dist/include/nsBuildID.h
#
# in the last value:
#   ie: milestone.txt               : 1.4a
#       nsBuildID.h                 : 2003030510
#       product version then will be: 1.4a.0.2003030510
#
# The milestone version is acquired from [topsrcdir]/config/milestone.txt
sub GetProductY2KVersion
{
  my($aDirTopObj, $aDirMozTopSrc, $aDirConfigTopSrc, $aDirMozTopObj) = @_;

  $aDirMozTopObj = $aDirTopObj if(!defined($aDirMozTopObj));

  if(defined($ENV{DEBUG_INSTALLER_BUILD}))
  {
    print " GetProductY2KVersion\n";
    print "   aDirTopObj      : $aDirTopObj\n";
    print "   aDirMozTopObj   : $aDirMozTopObj\n";
    print "   aDirMozTopSrc   : $aDirMozTopSrc\n";
    print "   aDirConfigTopSrc: $aDirConfigTopSrc\n";
  }

  my($fileBuildID)                      = "$aDirMozTopObj/dist/include/nsBuildID.h";
  my($initEmptyValues)                  = 1;
  my(@version)                          = undef;
  my($y2kDate)                          = undef;
  my($versionMilestone)                 = GetProductMilestoneVersion($aDirTopObj, $aDirMozTopSrc, $aDirConfigTopSrc, $initEmptyValues);

  @version = split /\./, $versionMilestone;
  $y2kDate = GetProductBuildID($fileBuildID, "NS_BUILD_ID");
  return("$version[0].$version[1].$version[2].$y2kDate");
}

package MozPackages;

# Parse packages.list into the global var %MozPackages::packageList
sub parsePackageList {
    my ($packageListFile) = @_;
    
    my $fileh;
    open $fileh, $packageListFile || die("Could not open $packageListFile");

    MozPackager::_verbosePrint(1, "Parsing package list $packageListFile");

    # this hash is keyed on the package name, the data is a reference
    # to a list of dependent packages
    %MozPackages::packages = ( );

    while (my $line = <$fileh>) {
        MozPackager::_verbosePrint(3, "Parsing $packageListFile, line $.");
        chomp $line;

        # ignore blank lines and comment lines (beginning with #)
        next if ($line =~ /^#/);
        next if ($line =~ /^[[:space:]]*$/);

        $line =~ /^([-[:alnum:]]+)[[:space:]]*\:(.*)$/ ||
            die("Could not parse package list, line $.: $line");

        exists($MozPackages::packages{$1}) && die("Package $1 defined twice.");
        $MozPackages::packages{$1} = [split(' ', $2)];

        MozPackager::_verbosePrint(2, "Package $1 found, dependencies: ", join(", ", @{$MozPackages::packages{$1}}));
    }
    close $fileh;
}

# This function takes a package name and returns a list of all the
# package dependencies. This is normally fed directly into MozParser::parse
sub getPackagesFor {
    my ($package) = @_;

    scalar keys %MozPackages::packages ||
        die("Call parsePackageList() before getPackagesFor()");

    MozPackager::_verbosePrint(1, "Recursing dependencies of package $package");

    my %myPackages; # hash set of package names
    _recursePackage($package, \%myPackages);
    return keys(%myPackages);
}

sub _recursePackage {
    my ($package, $myPackages) = @_;
    
    MozPackager::_verbosePrint(1, "Recursing package $package");

    my $unPackage = ((my $realPackage = $package) =~ s/^\^//);
    ($MozPackages::packages{$realPackage})
        || die("Package $package does not exist.");
    $myPackages->{$package} = 1;

    foreach my $packageName (@{$MozPackages::packages{$realPackage}}) {
        if ($unPackage) {
            die("Cannot process double-negative packages, recursing $package, found $packageName")
                if ($packageName =~ /^\^/);
            $packageName = '^'. $packageName;
        }
        _recursePackage($packageName, $myPackages);
    }
}

package MozParser;

# this is a global variable...
# 0 == die (default)
# 1 == warn
# 2 == ignore (not recommended)
$MozParser::missingFiles = 0;

sub new {
    return bless {
        # Hash of package names to build (hash-set, no data)
        'packages' => { },

        # list of mappings from tree-filenames to archive-filenames
        # each mapping is an list with two members
        # for example, in an RPM script this would be
        # [ ['dist/bin','usr/lib/mozilla-1.4'] ]
        # files without appropriate mappings throw errors
        'mappings' => [ ],

        # set of handlers for !cmd rules: key is the "cmd" string and value
        # is a reference to a function($parser, $params, $fileh, $filename)
        'commands' => { },

        # this is the "result-tree", except the mapping may seem backwards
        # result-file => source-file
        # This is because the same source-file can map to multiple result
        # files, but never the other way around.
        'files' => { },
    };
}

sub addMapping {
    my $self = shift;
    my ($source, $result) = @_;
    my $mappings = $self->{'mappings'};
    my $mapping = [$source, $result];
    push(@$mappings, $mapping);

    MozPackager::_verbosePrint(2, "Added mapping $source => $result");
}

sub addCommand {
    my $self = shift;
    my ($command, $function) = @_;
    $self->{'commands'}->{$command} = $function;

    MozPackager::_verbosePrint(2, "Added command $command ($function)");
}

# 
# parse($path, @packages)
# Parse files in $path to retrieve information about @packages
#
sub parse {
    scalar %MozPackages::packages ||
        die("Call parsePackageList() before parse()");

    my $self = shift;
    my ($file, @packages) = @_;
    
    -e $file || die("Path not found: $file");

    foreach my $package (@packages) {
        $self->{'packages'}->{$package} = 1;
    }

    # File::Find is not in early perls, so we manually walk the directory tree
    (-d $file) ? $self->_parseDir($file) : $self->_parseFile($self);
}

sub findMapping {
    my $self = shift;
    my ($mapfile, $filename) = @_;
    
    foreach my $mapping (@{$self->{'mappings'}}) {
        if ((my $match = $mapfile) =~
            s/^\Q$mapping->[0]/$mapping->[1]/) {

            MozPackager::_verbosePrint(3, "Mapping $mapfile to $match");
            return $match;
        }
    }
    die("In file $filename, line $.: no mapping for $mapfile");
}

sub _parseDir {
    my $self = shift;
    my ($dir) = @_;
    my $handle;
    my @entries;

    MozPackager::_verbosePrint(1, "Parsing directory $dir");

    opendir $handle, $dir || die("Could not open directory: $dir");
    @entries = grep(!/^\./, readdir($handle));
    closedir $handle;
    foreach my $filename (@entries) {
        if (-d File::Spec->catdir($dir, $filename)) {
            $self->_parseDir(File::Spec->catdir($dir, $filename));
        } else {
            next if ($filename eq '.headerlist');
            $self->_parseFile(File::Spec->catfile($dir, $filename));
        }
    }
}

sub _parseFile {
    my $self = shift;
    my ($file) = @_;
    my $process = 0;

    MozPackager::_verbosePrint(1, "Parsing file $file");

    my $fileh;
    open $fileh, $file || die("Could not open file: $file");
    while (my $line = <$fileh>) {
        chomp $line;
        MozPackager::_verbosePrint(3, "File $file, line $.: '$line'");

        # [packagename1 packagename2]
        if ($line =~ /^\[([^\[\]]*)\]$/) {
            my @packages = split(' ', $1);

            MozPackager::_verbosePrint(2, "Package list: ", join(", ", @packages));

            $process = 0;
            foreach my $package (@packages) {
                exists($MozPackages::packages{$package}) ||
                    die("At $file, line $.. Unknown package $package.");

                if (exists(${$self->{'packages'}}{'^'. $package})) {
                    # if we find a negative-package ^packagename, stop
                    # immediately
                    $process = 0;
                    MozPackager::_verbosePrint(2, "Found package ^$package");
                    last;
                }

                if (exists(${$self->{'packages'}}{$package})) {
                    $process = 1;
                    MozPackager::_verbosePrint(2, "Found package $package");
                }
            }
            MozPackager::_verbosePrint(2, "Skipping lines...") unless $process;
            next;
        }

        next if (! $process);

        # !command args...
        if ($line =~ s/^!(\S+)\s*//) {
            exists($self->{'commands'}->{$1})
                || die("In file $file, line $.: command processor $1 not defined");

            MozPackager::_verbosePrint(3, "Calling '$1' command processor.");

            $self->{'commands'}->{$1}($self, $line, $fileh, $file);
            next;
        }

        # files
        $line =~ s/^\s//;
        my ($source, $dest, $extra) = split(/(?:(?<!\\)\s)+/, $line, 3);

        next if (! $source); #blank line
        $extra && die("Parse error in $file, line $.: unrecognized text '$line'.");

        $source =~ s/\\(\s)/$1/g;
        $dest   =~ s/\\(\s)/$1/g if ($dest);
 
        _checkFile($source, $file, $.);
        
        # if a dist location is not specified, default
        if (! $dest) {
            $dest = $source;
        }

        $self->_addFile($source, $dest, $file);
    }
    close $fileh;
}

sub _addFile {
    my $self = shift;
    my ($source, $result, $filename) = @_;
    my $mapping = $self->findMapping($result, $filename);

    if ($self->{'files'}->{$mapping}) {
        if ($self->{'files'}->{$mapping} ne $source) {
            die("At $filename, line $., mapping $mapping mapped to two different files.
Old file: $self->{'files'}->{$mapping}
New file: $source");
        } else {
            warn("At $filename, line $., mapping $mapping mapped twice to the same file $source" );
        }
    }

    MozPackager::_verbosePrint(2, "Adding file $source => $mapping");

    $self->{'files'}->{$mapping} = $source;
}

# not an object function, this implements the
# $MozParser::missingFiles warning

sub _checkFile {
    my ($fileName, $sourceFile, $lineNo) = @_;

    # 2 == ignore
    $MozParser::missingFiles == 2 && return;

    my $pfile = MozPackager::joinfile($fileName);

    (my $fExists = -f $pfile) && return;

    if ($MozParser::missingFiles == 0) {
        die("In file $sourceFile, line $lineNo: file $fileName not found or not a regular file.");
    }

    if ($MozParser::missingFiles == 1) {
        warn("In file $sourceFile, line $lineNo: file $fileName not found or not a regular file.");
        return;
    }
    
    die ("Unrecognized value for \$MozParser::missingFiles: $MozParser::missingFiles");
}


#
# Merges all xpt files into one in the package
#
package MozParser::XPTMerge;

sub add {
    my ($parser) = @_;

    $parser->{'xptfiles'} = { };
    $parser->addCommand("xpt", \&_commandFunc);
}

sub mergeTo {
    my ($parser, $mergeOut) = @_;

    if (scalar(keys %{$parser->{'xptfiles'}}) > 0) {
        MozPackager::_verbosePrint(1, "Merging XPT files to $mergeOut: ", join(", ", keys %{$parser->{'xptfiles'}}));

        MozPackager::ensureDirs($mergeOut);

        # grr, cygwin hackery
        $mergeOut = MozPackager::makeWinPath($mergeOut);

        # XXXbsmedberg : this will not work in cross-compile environments,
        # but I'm not sure anyone cares
        my $linkCommand = File::Spec->catfile("dist", "bin", "xpt_link");

        system ($linkCommand, $mergeOut, map(MozPackager::joinfile($_), keys %{$parser->{'xptfiles'}})) &&
            die("xpt_link failed: code ". ($? >> 8));
    }
}

sub _commandFunc {
    my ($parser, $args, $file, $filename) = @_;

    my @args = split ' ', $args;
    scalar(@args) == 1 || die("At $filename, line $.: unrecognized xpt options \"$args\".");

    MozParser::_checkFile($args[0], $filename, $.);
    MozPackager::_verbosePrint(2, "Marking XPT $args[0] for merging");

    $parser->{'xptfiles'}->{$args[0]} = 1;
}

#
# like XPTMerge, but just copy the XPT into the package
# (This is not object-oriented.)
#
package MozParser::XPTDist;

sub add {
    my ($parser) = @_;

    $parser->addCommand("xpt", \&_commandFunc);
}

sub _commandFunc {
    my ($parser, $args, $file, $filename) = @_;

    my @args = split(' ', $args);
    scalar(@args) == 1 || die("At $filename, line $.: unrecognized xpt options \"$args\".");

    MozParser::_checkFile($args[0], $filename, $.);

    $parser->_addFile($args[0], $args[0], $filename);
}

#
# "touches" an empty file in the package (used for .autoreg)
#
package MozParser::Touch;

sub add {
    my ($parser, $dummyPath) = @_;

    $parser->addCommand("touch", \&_commandFunc);
    $parser->{"touchFile"} = $dummyPath;
}

sub _commandFunc {
    my ($parser, $args, $file, $filename) = @_;

    my @args = split ' ', $args;
    scalar(@args) == 1 || die("At $filename, line $.: unrecognized !touch options \"$args\".");

    $parser->_addFile($parser->{"touchFile"}, $args[0], $filename);
}

# preprocesses into the destination path
# !preprocess destpath options-and-files
# preprocessing takes place as a separate step so that
# required disk size can be computed from the completed stage
package MozParser::Preprocess;

# stageDir may be "" to indicate testing only
sub add {
    my ($parser) = @_;

    $parser->addCommand("preprocess", \&_commandFunc);
    $parser->{'ppfiles'} = { };
}

sub preprocessTo {
    my ($parser, $preprocessor, $stageDir) = @_;
    $preprocessor = ($ENV{'PERL'} ? $ENV{'PERL'} : "perl"). ' '. $preprocessor;

    MozPackager::_verbosePrint(1, "Preprocessing package files to $stageDir");

    foreach my $ppfile (keys %{$parser->{'ppfiles'}}) {
        MozPackager::_verbosePrint(2, "Preprocessing $ppfile");
        my $output = MozPackager::joinfile($stageDir, $ppfile);
        MozPackager::ensureDirs($output);

        MozPackager::system($preprocessor. ' '.
                            $parser->{'ppfiles'}->{$ppfile}.
                            ' > '. $output);
    }
}

sub _commandFunc {
    my ($parser, $args, $file, $filename) = @_;

    my @args = split ' ', $args;
    scalar(@args) >= 2 || die("At $filename, line $.: Insufficient number of arguments to !preprocess '$args'");

    $parser->{'ppfiles'}->{$parser->findMapping($args[0], $filename)} =
        join(' ', @args[1 .. scalar(@args)-1]);
}

package MozParser::Optional;

sub add {
    my ($parser) = @_;

    $parser->addCommand('optional', \&_commandFunc);
}

sub _commandFunc {
    my ($parser, $args, $file, $filename) = @_;

    my @args = split ' ', $args;

    $args[1] = $args[0] if (scalar(@args) == 1);
    scalar(@args) == 2 || die("At $filename, line $.: bad arguments to !optional flag!");

    # now we read the comment (if any) for a not-found file
    my $comment = "";
    my $bang = "";
    while (read($file, $bang, 2) == 2) {
        # is this a continuation line?
        if ($bang ne '!=') {
            # reset the filepos back two chars
            seek($file, -2, 1);
            last;
        }

        $comment .= <$file>;
    }

    $comment .= "\n";

    if (! -e MozPackager::joinfile($args[0])) {
        if ($MozParser::missingFiles < 2) {
            warn($comment);
        }
    } else {
        $parser->_addFile($args[0], $args[1], $filename);
    }
}

package MozParser::Exec;

sub add {
    my ($parser) = @_;

    $parser->addCommand('exec', \&_commandFunc);
    $parser->{'execCommands'} = [ ];
}

sub _commandFunc {
    my ($parser, $args, $file, $filename) = @_;

    my $command = "";
    my $readchars;

    while(1) {
        $args =~ s/\%([^\%]+)\%/'%'. $parser->findMapping($1, $filename). '%'/ge;
        $command .= $args;

        my $bang;
        $readchars = read($file, $bang, 2);
        last if ($readchars != 2 || $bang ne '!=');
    } continue {
        $args = <$file>;
    }

    # reset the filepos back two chars
    seek($file, 0-$readchars, 1) || die("Couldn't seek backwards!")
        if $readchars;

    push @{$parser->{'execCommands'}}, $command;
}

sub exec {
    my ($parser, $stageDir) = @_;

    foreach my $command (@{$parser->{'execCommands'}}) {
        $command =~ s/\%([^\%]+)\%/MozPackager::makeWinPath(MozPackager::joinfile($stageDir, $1))/ge;

        MozPackager::system($command);
    }
}

package MozParser::Ignore;

sub add {
    my ($parser) = @_;

    $parser->addCommand('ignore', \&_commandFunc);
    $parser->{'ignorePaths'} = [ ];
}

sub _commandFunc {
    my ($parser, $args, $file, $filename) = @_;

    push @{$parser->{'ignorePaths'}}, $args;
}

sub getIgnoreList {
    my ($parser) = @_;

    return @{$parser->{'ignorePaths'}};
}

# as a bonus, this function can be used to ignore any !command
# dump-packages uses this to ignore !error directives
sub ignoreFunc {
}

package MozStage;

use Cwd;

# stage files from a parser into a destination directory.
# If $stripCommand is defined, files with the appropriate extensions
# are stripped. The empty extension '' gets special treatment so as
# not to strip shell scripts.

sub stage {
    my ($parser, $destDir, $stripCommand, @extensions) = @_;
    my $fileHash = $parser->{'files'};

    my %extensions = map( {$_, 1} @extensions);

    MozPackager::_verbosePrint(1, "Staging files to $destDir");

    for my $dest (keys %$fileHash) {
        my $from = MozPackager::joinfile($fileHash->{$dest});
        my $to   = MozPackager::joinfile($destDir, $dest);

        my $strip;
        if ($stripCommand) {
            if ($to =~ /\.([[:alpha:]]+)$/) {
                if (exists($extensions{$1})) {
                    MozPackager::_verbosePrint(3, "Found extension '$1'");
                    $strip = 1;
                }
            } else {
                if (exists($extensions{''})) {
                    MozPackager::_verbosePrint(3, "Found empty extension.");
                    # if we have a file with no extension, figure out whether it's a shell script or a binary
                    $strip = 1 if (-B $from && -x $from);
                }
            }
        }

        # if we're going to strip it, force a copy instead of a symlink
        # so that we don't affect the files in the tree
        MozPackager::symCopy($from, $to, $strip);

        if ($strip) {
            MozPackager::_verbosePrint(2, "Stripping $to");
            MozPackager::system("$stripCommand $to");
        }
    }
}

# since an .XPI is just a zipfile, we use the same command for both
sub makeZIP {
    my ($stageDir, $xpiFile) = @_;

    MozPackager::_verbosePrint(1, "Making ZIP/XPI file '$xpiFile'.");
    MozPackager::ensureDirs($xpiFile);

    unlink $xpiFile if -e $xpiFile;

    my $savedCwd = cwd();
    $xpiFile = File::Spec->rel2abs($xpiFile);

    my $qFlag = ($MozPackager::verbosity) ? '' : '-q';
    $qFlag .= '-v ' if ($MozPackager::verbosity > 1);

    chdir $stageDir;
    MozPackager::system("zip -r -D -9 $qFlag $xpiFile *");
    chdir $savedCwd;
}

# xxxbsmedberg - we need a way to configure the tar flags for various
# platforms... the "easy" way is through the environment, but $^O may
# be better.

sub makeTGZ {
    my ($stageDir, $tgzFile) = @_;

    MozPackager::_verbosePrint(1, "Making TGZ file '$tgzFile'");
    MozPackager::ensureDirs($tgzFile);

    unlink $tgzFile if -e $tgzFile;

    my $savedCwd = cwd();
    $tgzFile = File::Spec->rel2abs($tgzFile);

    my $qFlag = ($MozPackager::verbosity) ? '' : '-q';
    my $vFlag = ($MozPackager::verbosity > 1) ? '-v ' : '';

    chdir $stageDir;
    MozPackager::system("tar $vFlag -chf - * | gzip -f9 $qFlag $vFlag > $tgzFile");
    chdir $savedCwd;
}

sub makeBZ2 {
    my ($stageDir, $bz2File) = @_;

    MozPackager::_verbosePrint(1, "Making BZ2 file '$bz2File'");
    MozPackager::ensureDirs($bz2File);

    unlink $bz2File if -e $bz2File;

    my $savedCwd = cwd();
    $bz2File = File::Spec->rel2abs($bz2File);

    my $qFlag = ($MozPackager::verbosity) ? '' : '-q';
    my $vFlag = ($MozPackager::verbosity > 1) ? '-v ' : '';

    chdir $stageDir;
    MozPackager::system("tar $vFlag -chf - * | bzip2 -f $qFlag $vFlag > $bz2File");
    chdir $savedCwd;
}

sub makeDMG {
    my ($stageDir, $dmgFile, $topsrcdir, $volumeName) = @_;

    MozPackager::_verbosePrint(1, "Making DMG file '$dmgFile'");
    MozPackager::ensureDirs($dmgFile);

    unlink $dmgFile if -e $dmgFile;

    MozPackager::system("$topsrcdir/build/package/mac_osx/make-diskimage $dmgFile $stageDir $volumeName");
}

return 1;
