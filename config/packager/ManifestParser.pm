#!/usr/bin/perl -w
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

package ManifestParser;

sub new {
    return bless {
        # Hash of package names to build (hash-set)
        'packages' => { },

        # list of mappings from tree-filenames to archive-filenames
        # each mapping is an list with two members
        # for example, in an RPM script this would be
        # [ ['dist/bin','usr/lib/mozilla-1.4'] ]
        # files without appropriate mappings throw errors
        'mappings' => [ ],

        # set of handlers for !cmd rules: key is the "cmd" string and value
        # is a reference to a function($parser, $params, FILE)
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
    
    die("File not found: $file") unless -e $file;
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
            $self->_parseDir("$dir/$filename");
        } else {
            $self->_parseFile("$dir/$filename");
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
            $self->{'commands'}->{$1}($self, $2, $fileh);
            next;
        }

        # files
        my @files = split(' ', $line);

        next if (scalar(@files) == 0); #blank line

        # if a dist location is not specified, use the mappings
        if (scalar(@files) == 1) {
            foreach my $mapping (@{$self->{'mappings'}}) {
                if ((my $match = $files[0]) =~
                        s/^\Q$mapping->[0]/\Q$mapping->[1]/) {
                    $files[1] = $match;
                    last;
                }
            }
            $files[1] || die("In file $file, line $.: no mapping for $files[0]");
        }

        scalar(@files) == 2 || die("Parse error in $file:\n$line\n");

        $self->{'files'}->{$files[1]} = $files[0];
    }
}

return 1;
