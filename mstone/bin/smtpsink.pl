/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is the Netscape Mailstone utility,
 * released March 17, 2000.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1999-2000 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):     Sean O'Rourke <sean@sendmail.com>
 *                     Thom O'Connor <thom@sendmail.com>
 *                     Sendmail, Inc.
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License Version 2 or later (the "GPL"), in
 * which case the provisions of the GPL are applicable instead of
 * those above.  If you wish to allow use of your version of this file
 * only under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GPL.  If you do not delete the
 * provisions above, a recipient may use your version of this file
 * under either the NPL or the GPL.
 */

use Thread qw(async);
use FileHandle qw(_IOLBF);
use Socket;
use strict;
use Getopt::Long;

# options
my $maxconn = SOMAXCONN;
my $t_banner = 0;
my $t_from = 0;
my $t_rcpt = 0;
my $t_dot = 0;
my $port = 25;
my $cs = '';
my $log = '';
my $proto = getprotobyname('tcp');

# statistics
my ($cmds, $errs, $bytesw, $bytesr, $msgs);

GetOptions('maxconn:i'		=> \$maxconn,
	   'banner-delay:i'	=> \$t_banner,
	   'from-delay:i'	=> \$t_from,
	   'rcpt-delay:i'	=> \$t_rcpt,
	   'dot-delay:i'	=> \$t_dot,
	   'log:s'		=> \$log,
	   'checksums:s'	=> sub { use Digest::MD5; $cs = $_[0]; })
    || &usage;

if (@ARGV == 1) {
    ($port) = @ARGV;
}

if ($log) {
    if (open(LOG, ">$log")) {
	print STDERR "Logging messages to $log\n";
    } else {
	warn ("Cannot open logfile $log\n");
	$log = '';
    }
}

socket(S, PF_INET, SOCK_STREAM, $proto) || die $!;
setsockopt(S, SOL_SOCKET, SO_REUSEADDR, pack("l", 1)) || die $!;

(bind(S, sockaddr_in($port, INADDR_ANY))
 && listen(S, $maxconn))
    || die $!;

my $addr;
my $fh = new FileHandle;
my $cnt = 0;

$SIG{INT} = sub { close(LOG); print STDERR "done\n"; exit 0; };

while(my $addr = accept($fh, S)) {
    if (++$cnt % 100 == 0) {
	print STDERR "$cnt\r";
    }
    my $thr = async { do_smtp($addr, $fh); };
    $thr->join;
    $fh = new FileHandle;
}

sub usage
{
    print STDERR <<EOS;
Usage: $0 [options] [port]
EOS
    exit -1;
}

sub netline
{
    my $fh = shift;
    my $line = $fh->getline;
    $line =~ s/\r\n$// if $line;
    $line;
}

sub millisleep
{
    my $t = shift;
    select(undef, undef, undef, $t) if $t > 0;
}

sub netprint($@)
{
    my ($fh, @stuff) = @_;
    foreach (@_) {
	s/([^\r])\n/$1\r\n/g;
	$fh->print($_);
    }
}

sub do_smtp
{
    my ($addr, $s) = @_;
    my $buf;
    my ($port, $iaddr) = sockaddr_in($addr);
    my $name = gethostbyaddr($iaddr, AF_INET);
    $s->setvbuf($buf, _IOLBF, 1024);

    millisleep($t_banner);
    $s->print("220 wazzup, bro?\r\n");
    my %state = ('conn'		=> $s,
		 'host'		=> $name);
    my %funcs = ('helo'		=> \&do_helo,
		 'ehlo'		=> \&do_helo,
		 'word'		=> \&do_helo,

		 'quit'		=> \&do_quit,
		 'latr'		=> \&do_quit,

		 'mail'		=> \&do_from,
		 'rcpt'		=> \&do_rcpt,
		 'data'		=> \&do_data,
		 'rset'		=> \&do_ok,
		 'vrfy'		=> \&do_ok,
		 'noop'		=> \&do_ok,
		);
    while (my $line = netline($s))
    {
	my ($cmd, $arg) = ($line =~ /^\s*(\S+)\s*(.*)$/);
	die "cmd = `$cmd'" unless ($cmd = lc($cmd));
	# fail 1% of commands
	if ($funcs{$cmd}) {
	    &{$funcs{$cmd}}(\%state, $cmd, $arg, \%funcs);
	} else {
	    $s->print("500 5.0.0 no.  Just... no.\r\n");
	}
    }
    return 0;
}

sub already_said_that
{
    my ($state, $cmd, $arg) = @_;
    $state->{conn}->print("503 5.0.0 Dude, you already said that.\r\n");
}

sub do_helo
{
    my ($state, $cmd, $arg) = @_;
    $state->{helohost} = $arg;
    die unless $cmd;
    if ($cmd eq 'helo') {
	if ($arg eq $state->{host}) {
	    $state->{conn}->print("221 hello Mr. Honest\r\n");
	} else {
	    $state->{conn}->print("221 We know where you live, $state->{iaddr}\r\n");
	}
    } elsif ($cmd eq 'ehlo') {
	my $esmtp = <<EOS;
250-localhost is pleased to make your acquaintance, and offers:
250 8bitmime
EOS
	if ($arg) {
	    netprint($state->{conn}, $esmtp);
	} else {
	    $state->{conn}->print("501 5.0.0 tell me more...\r\n");
	    return;
	}
    } else {
	$state->{conn}->print("221 2.0.0 peace brother\r\n");
    }
}

sub do_quit
{
    my ($state, $cmd) = @_;
    $state->{conn}->print( "221 2.0.0 drop in any time\r\n");
    close ($state->{conn});
}

sub do_from
{
    my ($state, $cmd, $args, $funcs) = @_;
    millisleep($t_from);
    $state->{conn}->print( "250 2.1.0 Okay, keep talking\r\n");
    $funcs->{rcpt} = \&do_rcpt;
}

sub do_rcpt
{
    my ($state, $cmd, $args, $funcs) = @_;
    my ($rcpt) = ($args =~ /to\:\s*(.+)/i);
    millisleep($t_rcpt);
    $state->{conn}->print("250 2.1.5 ${rcpt}'s cool\r\n");
    $funcs->{data} = \&do_data;
}

sub do_data
{
    my ($state, $cmd, $args, $funcs) = @_;
    $state->{conn}->print( "354 up to the dot...\r\n");
    if ($cs) {
	my $line;
	my $md5 = Digest::MD5->new;
	# skip headers
    header: while ($_ = netline($state->{conn})) {
	    last header if /^$/;
	    last body if /^\.$/;
	}

    body: while ($_ = $state->{conn}->getline) {
	    if (/^=CS=MD5=(.+)\r/) {
		my $sum = $1;
		my $end = netline($state->{conn});

		if ($end ne '.') {
		    print STDERR "Fake checksum?: $sum\n";
		    while (netline($state->{conn}) ne '.') { }
		} elsif (lc($sum) eq lc($md5->hexdigest)) {
#  		    print STDERR "MD5 sum OK.\n";
		} else {
		    print STDERR "MD5 sum mismatch: $sum, ",
			$md5->hexdigest, "\n";
		}
		last body;
	    } elsif (/^\.\r$/) {
  		print STDERR "no checksum\n" if $cs =~ /^r/;
		last body;
	    }
	    print LOG $_ if $log;
	    $md5->add($_);
	}
	print LOG ".\r\n" if $log;
    } else {
	local ($/) = "\r\n.\r\n";
	$state->{conn}->getline;
    }
    millisleep($t_dot);
    if (rand() % 100 == 0) {
	$state->{conn}->print("451 ohshit\r\n");
    } else {
	$state->{conn}->print("250 2.0.0 I will deliver\r\n");
    }
}

sub do_ok
{
    my ($state) = @_;
    $state->{conn}->print("220 uhhuh\r\n");
}
