#!perl

use File::Copy;
use File::Compare;

$infile = shift;
$controlfile = shift;

if (!defined($infile) || !defined($controlfile)) {
    print "usage: $0 file-to-trim file-of-trimmings\n";
    exit(1);
}

open(IN, "$infile") || die("$infile: $!\n");
binmode(IN);
open(OUT,">$infile.out") || die("$infile.out: $!\n");
binmode(OUT);
open(CONTROL, "$controlfile") || die ("$controlfile: $!\n");
binmode(CONTROL);

undef @list;
while (<CONTROL>) {
    chomp;
    push @list, $_;
}
close(CONTROL);

while ($tmp=<IN>) {
    chomp($tmp);
    print OUT "$tmp\n" if (!grep(/^$tmp$/,@list));
}
close(IN);
close(OUT);
if (compare("$infile", "$infile.out") == 0) {
    unlink("$infile.out");
} else {
    unlink("$infile");
    move("$infile.out","$infile");
}
