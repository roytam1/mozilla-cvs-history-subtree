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

# this is a wrapper around File::Spec->catfile that splits on '/'
# and rejoins the path
sub joinfile {
    return File::Spec->catfile(map(split('/', $_), @_));
}

sub joindir {
    return File::Spec->catdir(map(split('/', $_), @_));
}

# level 0, no progress
# level 1, basic "parsing file blah"
# level 2, more detail
# level 3, graphic detail
$MozPackager::verbosity = 0;

sub _verbosePrint {
    my $vlevel = shift;
    local $\ = "\n";
    print STDERR @_ if ($vlevel <= $MozPackager::verbosity);
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
    scalar keys %MozPackages::packages ||
        die("Call parsePackageList() before parse()");

    my $self = shift;
    my ($file, @packages) = @_;
    
    -e $file || die("Path not found: $file");

    foreach my $package (@packages) {
        $self->{'packages'}->{$package} = 1;
    }

    # File::Find is not in early perls, so we manually walk the directory tree
    (-d $file) ? $self->_parseDir($file) : $self->_parseFile($self);

    return $self->{'files'};
}

sub findMapping {
    my $self = shift;
    my ($mapfile, $filename) = @_;
    
    foreach my $mapping (@{$self->{'mappings'}}) {
        if ((my $match = $mapfile) =~
            s/^\Q$mapping->[0]/\Q$mapping->[1]/) {

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
        if (-d "$dir/$filename") {
            $self->_parseDir(File::Spec->catdir($dir, $filename));
        } else {
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
        MozPackager::_verbosePrint(3, "File $file, line $.");
        chop $line;

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
        if ($line =~ /^!(\S+)\s+(.*)$/) {
            exists($self->{'commands'}->{$1})
                || die("In file $file, line $.: command processor $1 not defined");

            MozPackager::_verbosePrint(3, "Calling '$1' command processor.");

            $self->{'commands'}->{$1}($self, $2, $fileh, $file);
            next;
        }

        # files
        my @files = split(' ', $line);

        next if (scalar(@files) == 0); #blank line
        scalar(@files) <= 2 || die("Parse error in $file, line $.: unrecognized text \"$line\".");

        _checkFile($files[0], $file, $.);
        
        # if a dist location is not specified, default
        if (scalar(@files) == 1) {
            $files[1] = $files[0];
        }

        $self->_addFile($files[0], $files[1], $file);
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

    MozPackager::_verbosePrint(1, "Merging XPT files to $mergeOut: ", join(", ", keys %{$parser->{'xptfiles'}}));

    # XXXbsmedberg : this will not work in cross-compile environments,
    # but I'm not sure anyone cares
    my $linkCommand = File::Spec->catfile("dist", "bin", "xpt_link");

    MozStage::Utils::ensureDirs($mergeOut);
    system ($linkCommand, $mergeOut, map(MozPackager::joinfile($_), keys %{$parser->{'xptfiles'}})) &&
        die("xpt_link failed: code ". ($? >> 8));
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
        my $commandLine = $preprocessor. ' '.
            $parser->{'ppfiles'}->{$ppfile}.
            ' > '. $output;

        MozStage::Utils::ensureDirs($output);

        system $commandLine;
        die("Error during preprocessing, command $commandLine returned, code ". ($? >>8)) if ($? >>8);
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
        $parser->_addFile($args[0], $args[1]);
    }
}

package MozStage::Utils;

use File::Copy;

my $cansymlink = eval {symlink('', ''); 1; };

# this global var may be set by clients who want to force a copy
# instead of a symlink
$MozStage::forceCopy = 0;

sub ensureDirs {
    my ($to) = @_;

    my ($volume, $dirs, $file) = File::Spec->splitpath($to);
    my @dirs = File::Spec->splitdir($dirs);
    
    my $numdirs = scalar(@dirs);
    # now create directories
    my $i = 0;
    while ($i < $numdirs) {
        my $dirname = File::Spec->catpath($volume, File::Spec->catdir(@dirs[0..$i]));
        mkdir $dirname if (! -e $dirname);
        ++$i;
    }
}

# this performs a symlink if possible on this system, otherwise
# peforms a regular "copy". It creates the directory structure if
# necessary.
sub symCopy {
    my ($from, $to) = @_;

    MozPackager::_verbosePrint(1, "Copying $from\t to $to");

    ensureDirs($to);

    if ($cansymlink && !$MozStage::forceCopy) {
        # $from is relative to the current (objdir) directory, so we need to
        # make it relative to $to
        my ($tovol, $topath, $tofile) = File::Spec->splitpath($to);
        my $todir = File::Spec->catpath($tovol, $topath);
        my $relfrom = File::Spec->abs2rel($from, $todir);

        MozPackager::_verbosePrint(1, "Symlinking $relfrom, $to");
        
        symlink($relfrom, $to) ||
            die("symlink $relfrom, $to failed.");
    } else {
        copy($from, $to) ||
            die("copy $from, $to failed.");
    }
}    

sub calcDiskSpace {
    my ($stageDir) = @_;

    MozPackager::_verbosePrint(1, "Calculating disk space for XPI package.");
    $ENV{'XPI_SPACEREQUIRED'} = int(_realDiskSpace($stageDir) / 1024) + 1;
    
    opendir(my $dirHandle, $stageDir) ||
        die("Could not open directory $stageDir for listing.");

    while (my $dir = readdir($dirHandle)) {
        if (! ($dir =~ /^\./)) {
            MozPackager::_verbosePrint(1, "Calculating disk space for subdir $dir");
            my $realDir = File::Spec->catdir($stageDir, $dir);
            if (-d $realDir) {
                $ENV{"XPI_SPACEREQUIRED_\U$dir"} = int(_realDiskSpace($realDir) / 1024) + 1;
            }
        }
    }
    closedir($dirHandle);
}

sub _realDiskSpace {
    my ($path) = @_;

    my $dsProg = File::Spec->catfile('dist', 'install', 'ds32.exe');

    my $spaceRequired = 0;

    if (-e $dsProg) {
        # We're on win32, use ds32.exe
        my $command = "$dsProg /D /L0 /A /S /C 32768 $path";
        $spaceRequired = `$command` ||
            die("Program failed: '$command' returned code ". ($? <<8));
    } else {
        opendir(my $dirHandle, $path) ||
            die("Could not open directory $path for listing.");

        while (my $dir = readdir($dirHandle)) {
            if (! ($dir =~ /^\./)) {
                my $realDir = File::Spec->catdir($path, $dir);
                if (-d $realDir) {
                    $spaceRequired += _realDiskSpace($realDir);
                } else {
                    $spaceRequired += (-s File::Spec->catfile($path, $dir));
                }
            }
        }
        closedir($dirHandle);
    }

    return $spaceRequired;
}

package MozStage;

use Cwd;

sub stage {
    my ($fileHash, $destDir) = @_;

    for my $dest (keys %$fileHash) {
        my $from = MozPackager::joinfile($fileHash->{$dest});
        my $to   = MozPackager::joinfile($destDir, $dest);
        MozStage::Utils::symCopy($from, $to);
    }
}

sub makeXPI {
    my ($stageDir, $xpiFile) = @_;

    MozPackager::_verbosePrint(1, "Making XPI file.");
    MozStage::Utils::ensureDirs($xpiFile);

    unlink $xpiFile if -e $xpiFile;

    my $savedCwd = cwd();
    $xpiFile = File::Spec->rel2abs($xpiFile);

    chdir $stageDir;
    my $command = "zip -r -D -9 $xpiFile *";
    system $command ||
        die("Program '$command' failed with code ". ($? <<8));
    chdir $savedCwd;
}

return 1;
