#!/usr/bin/perl
#

#
# This script gets called after a full mozilla build & test.
# Use this to build and test an embedded or commercial branding of Mozilla.
#
# Edit, and rename this file to post-mozilla.pl to activate.
# ./build-seamonkey-utils.pl will call PostMozilla::main() after
# a successful build and testing of mozilla.  This package can report
# status via the $TinderUtils::build_status variable.  Yeah this is a hack,
# but it works for now.  Feel free to improve this mechanism to properly
# return values & stuff.  -mcafee
#
# Contributors: 
# leaf@mozilla.org - release processing/packaging/uploading

# Assumptions:
#  no objdir 
#  ssh-agent running or empty passphrase RSA key
#  

use strict;

package PostMozilla;

sub print_log {
    my ($text) = @_;
    print LOG $text;
    print $text;
}

# Cache builds in a dated directory if:
#  * The hour of the day is $Settings::build_hour or higher 
#    (set in tinder-config.pl)
#    -and-
#  * the "last-built" file indicates a day before today's date
#
sub cacheit {
  my ($c_hour, $c_yday, $target_hour, $last_build_day) = @_;
  TinderUtils::print_log "current hour = $c_hour\n";
  TinderUtils::print_log "target hour = $target_hour\n";
  TinderUtils::print_log "current julian day = $c_yday\n";
  TinderUtils::print_log "last build julian day = $last_build_day\n";
  if (($c_hour > $target_hour) && 
      (($last_build_day < $c_yday) || ($c_yday == 0))) {
        return 1;
      } else {
	return 0;
      }
}



sub returnStatus{
  # build-anvil-util.pl expects an array, if no package is uploaded,
  # a single-element array with 'busted', 'testfailed', or 'success' works.
  my ($logtext, @status) = @_;
  TinderUtils::print_log "$logtext\n";
  return @status;
}

sub pushit {
  my ($ssh_server,$upload_path,$upload_target) = @_;
  # need to convert the path in case we're using activestate perl
  # check for platform
  #my $upload_target_dos = `cygpath -w $upload_target`;
  #chop($upload_target_dos);

  unless ( -e $upload_target) {
    TinderUtils::print_log "No $upload_target to upload\n";
    return 0;
  }

#for leaf
  TinderUtils::run_shell_command "ssh -1 -i /home/leaf/.ssh/cltbld/.ssh/identity -l cltbld $ssh_server mkdir -p $upload_path";
  TinderUtils::run_shell_command "scp -i /home/leaf/.ssh/cltbld/.ssh/identity -oProtocol=1 -r $upload_target cltbld\@$ssh_server:$upload_path";
  TinderUtils::run_shell_command "ssh -1 -i /home/leaf/.ssh/cltbld/.ssh/identity -l cltbld $ssh_server chmod -R 755 $upload_path/$upload_target";

# for cltbld
  #TinderUtils::run_shell_command "ssh -1 -l cltbld $ssh_server mkdir -p $upload_path";
  #TinderUtils::run_shell_command "scp -oProtocol=1 $upload_target cltbld\@$ssh_server:$upload_path";
  #TinderUtils::run_shell_command "ssh -1 -l cltbld $ssh_server chmod -R 755 $upload_path";
  return 1;
}

sub process_talkback {
# this is where we
# * generate a master.ini
# * upload symbol-files
# * shove talkback files into dist for packaging.
# 

}

sub make_packages {
  my ($build_dir, $build_date) = @_;
  my $save_dir  = `pwd`;

#DEBUG_LEAF
    print_log "current directory is $save_dir\n";
    print_log "cd to $build_dir/$Settings::Topsrcdir/xpinstall\n";
#end DEBUG_LEAF

  chdir "$build_dir/$Settings::Topsrcdir/xpinstall";
  $ENV{"INSTALLER_URL"} = "http://ftp.mozilla.org/pub/mozilla.org/pub/mozilla/tinderbox-builds/$build_date";
  system "make installer";
#  if ($DEBUG_LEAF) {
    print_log "post |make installer|\n";
# DEBUG_LEAF
  chdir $save_dir;

}

sub push_to_stage {
  my ($build_dir, $build_date) = @_;
  my $save_dir = `pwd`;
  chdir "$build_dir/$Settings::Topsrcdir/installer";
  mkdir $build_date;
  system "cp sea/* $build_date"; 
  system "cp stub/* $build_date";
  system "cp -r raw/xpi $build_date/linux-xpi";
  pushit ("stage.mozilla.org", "/home/ftp/pub/mozilla/tinderbox-builds", $build_date);
  chdir $save_dir;

}

sub pad_digit {
  my ($digit) = @_;
  if ($digit < 10) { return "0" . $digit };
  return $digit;
}

sub main {
  # Get build directory from caller.
  my ($mozilla_build_dir) = @_;
 
  #
  # (0: $sec, 1: $min, 2: $hour, 3: $mday, 4: $mon, 5: $year, 
  #  6: $wday, 7: $yday, 8: $isdst)
  my ($hour, $mday, $mon, $year, $yday) = (localtime (time)) [2,3,4,5,7];
  $year = $year + 1900;
  $mon = $mon + 1;
  $hour = pad_digit ($hour);
  $mday = pad_digit ($mday);
  $mon = pad_digit ($mon);
  my $build_date = "$year-$mon-$mday-$hour";
  #
  my $last_build_day = -1;
  if ( -e "$mozilla_build_dir/last-built"){
    ($last_build_day) = (localtime((stat "$mozilla_build_dir/last-built")[9]))[7];
  }
  
  print_log "build dir is $mozilla_build_dir \n";
  TinderUtils::print_log "Post-Mozilla build goes here.\n";

  # someday, we'll have talkback
  #process_talkback;
  
  make_packages ($mozilla_build_dir, $build_date);
  if (cacheit($hour, $yday, "8", $last_build_day)) { 
    push_to_stage ($mozilla_build_dir, $build_date);
    unlink "$mozilla_build_dir/last-built";
    open BLAH, ">$mozilla_build_dir/last-built"; 
    close BLAH;
  }

  # Report some kind of status to parent script.
  #
  #	 {'busted', 'testfailed', 'success'}
  #

  # Report a fake success, for example's sake.
  # Eventually return an array 
  &returnStatus (('success'));
}

# Need to end with a true value, (since we're using "require").
1;
