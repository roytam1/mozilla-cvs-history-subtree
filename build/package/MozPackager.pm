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

package MozPackages;

# A global list of packages is kept at in build/package/packages.list
# This function takes a package name and returns a list of all the
# package dependencies. This is normally fed directly into MozParser::parse

sub getPackagesFor {
    my ($package, $packageListFile) = @_;

    my $fileh;
    open $packageListFile, $fileh || die("Could not open $packageListFile");

    local %MozPackages::packages;
    while (my $line = <$fileh>) {
        chop $line;
        chomp $line;

        # ignore blank lines and comment lines (beginning with #)
        next if (! $line);
        next if ($line =~ /^#/);

        $line =~ /^([-[:alnum:]]+)[[:space:]]*:(.*)$/ &&
            die("Could not parse package list, line $.: $line");

        exists($MozPackages::packages{$1}) && die("Package $1 defined twice.");
        $MozPackages::packages{$1} = \split(' ', $2);
    }

    local %MozPackages::packageList; # hash set of package names
    _recursePackage($package);
    return keys(%MozPackages::packageList);
}

sub _recursePackage {
    my ($package) = @_;

    exists($MozPackages::packages{$package}) || die("Package $package does not exist.");
    $MozPackages::packageList{$package} = 1;

    foreach my $packageName (@{$MozPackages::packages{$package}}) {
        _recursePackage($packageName);
    }
}

package MozParser;

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
}

sub addCommand {
    my $self = shift;
    my ($command, $function) = @_;
    $self->{'commands'}->{$command} = $function;
}

# 
# parse($path, @packages)
# Parse files in $path to retrieve information about @packages
#
sub parse {
    my $self = shift;
    my ($file, @packages) = @_;
    
    -e $file || die("Path not found: $file");

    foreach my $package (@packages) {
        $self->{'packages'}->{$package} = 1;
    }

    # File::Find is not in Perl 5.6, so we manually walk the directory tree
    (-d $file) ? $self->_parseDir($file) : $self->_parseFile($self);

    return $self->{'files'};
}

sub _parseDir {
    my $self = shift;
    my ($dir) = @_;
    my $handle;
    my @entries;

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

    my $fileh;
    open $fileh, $file || die("Could not open file: $file");
    while (my $line = <$fileh>) {
        chop $line;

        # [packagename1 packagename2]
        if ($line =~ /^\[([^\[\]]*)\]$/) {
            my @packages = split(' ', $1);
            $process = 0;
            foreach my $package (@packages) {
                if (exists(${$self->{'packages'}}{$package})) {
                    $process = 1;
                    last;
                }
            }
            next;
        }

        next if (! $process);

        # !command args...
        if ($line =~ /^!(\S+)\s+(.*)$/) {
            exists($self->{'commands'}->{$1})
                || die("In file $file, line $.: command processor $1 not defined");
            $self->{'commands'}->{$1}($self, $2, $fileh, $file);
            next;
        }

        # files
        my @files = split(' ', $line);

        next if (scalar(@files) == 0); #blank line
        scalar(@files) <= 2 || die("Parse error in $file, line $.: unrecognized text \"$line\".");

        -e $files[0] || die("In $file, line $.: \"$files[0]\" not found.");
        
        # if a dist location is not specified, use the mappings
        if (scalar(@files) == 1) {
            $files[1] = $self->_findMapping($files[0], $file);
        }

        $self->_addFile($files[0], $files[1]);
    }
}

sub _findMapping {
    my $self = shift;
    my ($mapfile, $filename) = @_;
    
    foreach my $mapping (@{$self->{'mappings'}}) {
        if ((my $match = $mapfile) =~
            s/^\Q$mapping->[0]/\Q$mapping->[1]/) {
            return $match;
        }
    }
    die("In file $filename, line $.: no mapping for $mapfile");
}

sub _addFile {
    my $self = shift;
    # translate forward-slash into platform delimiter
    my ($source, $result) = map(File::Spec->catfile(split('/', $_)), @_);

    warn("$result mapped twice: $source and $self->{'files'}->{$result}" )
        if ($self->{'files'}->{$result});

    $self->{'files'}->{$result} = $source;
}

#
# Merges all xpt files into one in the package
#
package MozParser::XPTMerge;

sub add {
    my ($parser) = @_;

    $parser->{'xptfiles'} = [ ];
    $parser->addCommand("xpt", \&_commandFunc);
}

sub mergeTo {
    my ($parser, $mergeOut) = @_;

    # XXXbsmedberg : this will not work in cross-compile environments,
    # but I'm not sure anyone cares
    !system ("dist/bin/xpt_link", $mergeOut, @{$parser->{'xptfiles'}}) ||
        die("xpt_link failed: code ". ($? >> 8));
}

sub _commandFunc {
    my ($parser, $args, $file, $filename) = @_;

    my @args = split ' ', $args;
    scalar(@args) == 1 || die("At $filename, line $.: unrecognized xpt options \"$args\".");

    -e $args[0] || die("In $filename, line $.: xptfile $args[0] not found.");

    push @{$parser->{'xptfiles'}}, $args[0];
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

    -e $args[0] || die("In $file, line $.: xptfile $args[0] not found.");

    my $mapping = $parser->_findMapping($args[0]);
    $parser->_addFile($args[0], $mapping);
}

package MozStage::Utils;

my $cansymlink = eval {symlink('', ''); 1; };

# this performs a symlink if possible on this system, otherwise peforms
# a regular "copy".
sub symCopy {
    my ($from, $to) = @_;

    if ($cansymlink) {
        # $from is relative to the *current* directory, so we need to make it
        # relative to $to
        my $relfrom = File::Spec->abs2rel($from, $to);
        symlink($relfrom, $to) ||
            die("symlink $relfrom, $to failed.");
    } else {
        copy($from, $to) ||
            die("copy $from, $to failed.");
    }
}    

package MozStage;

sub stage {
    my ($fileHash, $destDir) = @_;

    for my $dest (keys %$fileHash) {
        MozStage::Utils::symCopy($fileHash->{$dest}, $dest);
    }
}

return 1;
