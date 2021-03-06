#!/usr/bin/perl
#

#
# This script gets called after a full mozilla build & test.
#
# packages and delivers mozilla bits
# Assumptions:
#  mozilla tree
#  produced builds get put in Topsrcdir/installer/sea
#
#
# tinder-config variables that you should set:
#  package_creation_path: directory to run 'make installer' to create an
#                         installer, and 'make' to create a zip/tar build
#  ftp_path: directory to upload nightly builds to
#  url_path: absolute URL to nightly build directory
#  tbox_ftp_path: directory to upload tinderbox builds to
#  tbox_url_path: absolute URL to tinderbox builds directory
#  notify_list: list of email addresses to notify of nightly build completion
#  build_hour: upload the first build completed after this hour as a nightly
#  ssh_server: server to upload build to via ssh
#  ssh_user: user to log in to ssh with
#  ssh_version: if set, force ssh protocol version N
#  milestone: suffix to append to date and latest- directory names
#  stub_installer: (0/1) whether to upload a stub installer
#  sea_installer: (0/1) whether to upload a sea (blob) installer
#  archive: (0/1) whether to upload an archive (tar or zip) build
#  clean_objdir: (0/1) whether to wipe out the objdir (same as the srcdir if no
#                objdir set) when starting a release build cycle
#  clean_srcdir: (0/1) whether to wipe out the srcdir for a release cycle
#
#  windows-specific variables:
#   as_perl_path: cygwin-ized path to Activestate Perl's bin directory

use strict;
use Sys::Hostname;
use POSIX qw(mktime);

package PostMozilla;

use Cwd;

# This is set in PreBuild(), and is checked after each build.
my $cachebuild = 0;

sub do_installer { return TinderUtils::is_windows() || 
                          TinderUtils::is_linux() ||
                          TinderUtils::is_os2(); }

sub print_locale_log {
    my ($text) = @_;
    print LOCLOG $text;
    print $text;
}

sub run_locale_shell_command {
    my ($shell_command) = @_;
    local $_;

    my $status = 0;
    chomp($shell_command);
    print_locale_log "$shell_command\n";
    open CMD, "$shell_command $Settings::TieStderr |" or die "open: $!";
    print_locale_log $_ while <CMD>;
    close CMD or $status = 1;
    return $status;
}

sub mail_locale_started_message {
    my ($start_time, $locale) = @_;
    my $msg_log = "build_start_msg.tmp";
    open LOCLOG, ">$msg_log";

    my $platform = $Settings::OS =~ /^WIN/ ? 'windows' : 'unix';

    my $BuildTreeLocale;
    if ( defined($Settings::BuildTreeLocale) ) {
        $BuildTreeLocale = $Settings::BuildTreeLocale;
    } else {
        $BuildTreeLocale = "$Settings::BuildTree-%s";
    }

    print_locale_log "\n";
    print_locale_log "tinderbox: tree: " . sprintf($BuildTreeLocale, $locale) . "\n";
    print_locale_log "tinderbox: builddate: $start_time\n";
    print_locale_log "tinderbox: status: building\n";
    print_locale_log "tinderbox: build: $Settings::BuildName $locale\n";
    print_locale_log "tinderbox: errorparser: $platform\n";
    print_locale_log "tinderbox: buildfamily: $platform\n";
    print_locale_log "tinderbox: version: $::Version\n";
    print_locale_log "tinderbox: END\n";
    print_locale_log "\n";

    close LOCLOG;

    if ($Settings::blat ne "" && $Settings::use_blat) {
        system("$Settings::blat $msg_log -t $Settings::Tinderbox_server");
    } else {
        system "$Settings::mail $Settings::Tinderbox_server "
            ." < $msg_log";
    }
    unlink "$msg_log";
}

sub mail_locale_finished_message {
    my ($start_time, $build_status, $logfile, $locale) = @_;

    # Rewrite LOG to OUTLOG, shortening lines.
    open OUTLOG, ">$logfile.last" or die "Unable to open logfile, $logfile: $!";

    my $platform = $Settings::OS =~ /^WIN/ ? 'windows' : 'unix';

    my $BuildTreeLocale;
    if ( defined($Settings::BuildTreeLocale) ) {
        $BuildTreeLocale = $Settings::BuildTreeLocale;
    } else {
        $BuildTreeLocale = "$Settings::BuildTree-%s";
    }

    # Put the status at the top of the log, so the server will not
    # have to search through the entire log to find it.
    print OUTLOG "\n";
    print OUTLOG "tinderbox: tree: " . sprintf($BuildTreeLocale, $locale) . "\n";
    print OUTLOG "tinderbox: builddate: $start_time\n";
    print OUTLOG "tinderbox: status: $build_status\n";
    print OUTLOG "tinderbox: build: $Settings::BuildName $locale\n";
    print OUTLOG "tinderbox: errorparser: $platform\n";
    print OUTLOG "tinderbox: buildfamily: $platform\n";
    print OUTLOG "tinderbox: version: $::Version\n";
    print OUTLOG "tinderbox: utilsversion: $::UtilsVersion\n";
    print OUTLOG "tinderbox: logcompression: $Settings::LogCompression\n";
    print OUTLOG "tinderbox: logencoding: $Settings::LogEncoding\n";
    print OUTLOG "tinderbox: END\n";

    if ($Settings::LogCompression eq 'gzip') {
        open GZIPLOG, "gzip -c $logfile |" or die "Couldn't open gzip'd logfile: $!\n";
        TinderUtils::encode_log(\*GZIPLOG, \*OUTLOG);
        close GZIPLOG;
    }
    elsif ($Settings::LogCompression eq 'bzip2') {
        open BZ2LOG, "bzip2 -c $logfile |" or die "Couldn't open bzip2'd logfile: $!\n";
        TinderUtils::encode_log(\*BZ2LOG, \*OUTLOG);
        close BZ2LOG;
    }
    else {
        open LOCLOG, "$logfile" or die "Couldn't open logfile, $logfile: $!";
        TinderUtils::encode_log(\*LOCLOG, \*OUTLOG);
        close LOCLOG;
    }    
    close OUTLOG;
    unlink($logfile);

    # If on Windows, make sure the log mail has unix lineendings, or
    # we'll confuse the log scraper.
    if ($platform eq 'windows') {
        open(IN,"$logfile.last") || die ("$logfile.last: $!\n");
        open(OUT,">$logfile.new") || die ("$logfile.new: $!\n");
        while (<IN>) {
            s/\r\n$/\n/;
            print OUT "$_";
        } 
        close(IN);
        close(OUT);
        File::Copy::move("$logfile.new", "$logfile.last") or die("move: $!\n");
    }

    if ($Settings::ReportStatus and $Settings::ReportFinalStatus) {
        if ($Settings::blat ne "" && $Settings::use_blat) {
            system("$Settings::blat $logfile.last -t $Settings::Tinderbox_server");
        } else {
            system "$Settings::mail $Settings::Tinderbox_server "
                ." < $logfile.last";
        }
    }
}

sub stagesymbols {
  my $builddir = shift;
  if (TinderUtils::is_windows()) {
    TinderUtils::run_shell_command("$Settings::Make -C $builddir deliver");
  }
}

sub makefullsoft {
  my $builddir = shift;
  my $srcdir = shift;
  if ($^O eq "cygwin") {
    # need to convert the path in case we're using activestate perl
    $builddir = `cygpath -u $builddir`;
  }
  chomp($builddir);

  my $rv = 0;

  # Check to see whether we should be using a prebuilt Talkback extension vs.
  # compiling one from scratch.
  if ($Settings::UsePrebuiltTalkback && -e $Settings::UsePrebuiltTalkback) {
    TinderUtils::run_shell_command("cd $builddir && tar jxvf $Settings::UsePrebuiltTalkback && cp -r fullsoft $srcdir");
  } elsif ($Settings::MofoRoot) {
    $ENV{CVS_RSH} = "ssh" unless defined($ENV{CVS_RSH});
    my $fullsofttag = " ";
    $fullsofttag = " -r $Settings::BuildTag"
          unless not defined($Settings::BuildTag) or $Settings::BuildTag eq '';
    TinderUtils::run_shell_command("cd $srcdir && cvs -d$Settings::MofoRoot co $fullsofttag -d fullsoft talkback/fullsoft");
  } else {
    # We're missing some key piece of data we need to build Talkback when 
    # we think we should be able to. FAIIL.
    return 1;
  }

  if ($Settings::ObjDir ne '') {
    $rv = TinderUtils::run_shell_command("mkdir -p $builddir/fullsoft && cd $builddir/fullsoft && $srcdir/build/autoconf/make-makefile -d ..");
  } else {
    $rv = TinderUtils::run_shell_command("cd $builddir/fullsoft && $builddir/build/autoconf/make-makefile -d ..");
  }

  my $buildid = get_buildid(objdir=>$builddir);
  my $set_fc_build = $buildid ? 'FC_BUILD='.$buildid : '';

  if (!$rv) {
    if (!$Settings::UsePrebuiltTalkback) {
      TinderUtils::run_shell_command("$Settings::Make -C $builddir/fullsoft $set_fc_build");
    } else {
      # We need to update the Talkback master.ini file with the current
      # build ID.
      TinderUtils::print_log("Setting build ID for prebuilt Talkback.\n");
      my $talkback_path = TinderUtils::getPathToTalkbackClient("$builddir/dist/bin");
      if ($talkback_path and $buildid) {
        unless (TinderUtils::setBuildIdInTalkbackMasterConfig(talkbackClientPath => $talkback_path,
                                                              newBuildId         => $buildid)) {

            TinderUtils::print_log("Failed to set build ID for prebuilt Talkback.\n");
            $rv = 1;
        }
      } else {
        TinderUtils::print_log("Could not find Talkback path ($talkback_path) or build ID ($buildid).\n");
        $rv = 1;
      }
    }
  }

  if (!$rv) {
    if ($Settings::BinaryName ne 'Camino') {
      TinderUtils::run_shell_command("$Settings::Make -C $builddir/fullsoft fullcircle-push $set_fc_build");
    } else {
      # Something completely different
      TinderUtils::run_shell_command("$Settings::Make -C $builddir/fullsoft fullcircle-push $set_fc_build UPLOAD_FILES='`find -X $builddir/dist/Camino.app -type f -perm -111 -exec file {} \\; | grep \": Mach-O\" | sed \"s/: Mach-O.*//\" | xargs`'");
    }
  }

  if (!$rv and TinderUtils::is_mac()) {
    if ($Settings::BinaryName eq 'Camino') {
      TinderUtils::run_shell_command("rsync -a --copy-unsafe-links $builddir/dist/bin/components/*qfa* $builddir/dist/bin/components/talkback $builddir/dist/Camino.app/Contents/MacOS/components");
    } else {
      TinderUtils::run_shell_command("$Settings::Make -C $builddir/$Settings::mac_bundle_path");
    }

    if ($Settings::MacUniversalBinary) {
      # Regenerate universal binary, now including Talkback
      TinderUtils::run_shell_command("$Settings::Make -C $Settings::Topsrcdir -f $Settings::moz_client_mk $Settings::MakeOverrides postflight_all");
    }
  }

  return $rv;
}

sub processtalkback {
  # first argument is whether to make a new talkback build on server
  #                and upload debug symbols
  # second argument is where we're building our tree
  # third argument is srcdir
  my $makefullsoft      = shift;
  my $builddir      = shift;   
  my $srcdir = shift;
  # put symbols in builddir/dist/buildid
  stagesymbols($builddir); 
  my $rv = 0;
  if ($makefullsoft) {
    $ENV{FC_UPLOADSYMS} = 1;
    $rv = makefullsoft($builddir, $srcdir);
  }
  return $rv;
}

sub packit {
  my ($packaging_dir, $package_location, $url, $stagedir, $builddir, $cachebuild) = @_;
  my $status = 0;

  if ($^O eq "cygwin") {
    # need to convert the path in case we're using activestate perl
    $builddir = `cygpath -u $builddir`;
  }
  chomp($builddir);

  TinderUtils::run_shell_command("mkdir -p $stagedir");

  if (do_installer()) {
    if (TinderUtils::is_windows()) {
      $ENV{INSTALLER_URL} = "$url/windows-xpi";
    } elsif (TinderUtils::is_linux()) {
      $ENV{INSTALLER_URL} = "$url/linux-xpi/";
    } elsif (TinderUtils::is_os2()) {
      $ENV{INSTALLER_URL} = "$url/os2-xpi/";
    } else {
      die "Can't make installer for this platform.\n";
    }
    TinderUtils::print_log("INSTALLER_URL is " . $ENV{INSTALLER_URL} . "\n");

    TinderUtils::run_shell_command("mkdir -p $package_location");

    # the Windows installer scripts currently require Activestate Perl.
    # Put it ahead of cygwin perl in the path.
    my $save_path;
    if (TinderUtils::is_windows()) {
      $save_path = $ENV{PATH};
      $ENV{PATH} = $Settings::as_perl_path.":".$ENV{PATH};
    }

    # one of the operations we care about saving status of
    if ($Settings::sea_installer || $Settings::stub_installer) {
      $status = TinderUtils::run_shell_command("$Settings::Make -C $packaging_dir installer");
    } else {
      $status = 0;
    }

    if (TinderUtils::is_windows()) {
      $ENV{PATH} = $save_path;
      #my $dos_stagedir = `cygpath -w $stagedir`;
      #chomp ($dos_stagedir);
    }

    my $push_raw_xpis;
    if ($Settings::stub_installer) {
      $push_raw_xpis = 1;
    } else {
      $push_raw_xpis = $Settings::push_raw_xpis;
    }

    if ($Settings::TestWithJprof) {
        TinderUtils::run_shell_command("cp $package_location/../*-jprof.html $stagedir/");
    }

    if (TinderUtils::is_windows() || TinderUtils::is_os2()) {
      if ($Settings::stub_installer) {
        TinderUtils::run_shell_command("cp $package_location/stub/*.exe $stagedir/");
      }
      if ($Settings::sea_installer) {
        TinderUtils::run_shell_command("cp $package_location/sea/*.exe $stagedir/");
      }

      # If mozilla/dist/install/*.msi exists, copy it to the staging
      # directory.
      my @msi = grep { -f $_ } <${package_location}/*.msi>;
      if ( scalar(@msi) gt 0 ) {
        my $msi_files = join(' ', @msi);
        TinderUtils::run_shell_command("cp $msi_files $stagedir/");
      }

      if ($push_raw_xpis) {
        # We need to recreate the xpis with compression on, for update.
        # Since we've already copied over the 7zip-compressed installer, just
        # re-run the installer creation with 7zip disabled to get compressed
        # xpi's, then copy them to stagedir.
        #
        # Also set MOZ_PACKAGE_MSI to null to avoid repackaging it
        # unnecessarily.
        my $save_msi = $ENV{MOZ_PACKAGE_MSI};
        my $save_7zip = $ENV{MOZ_INSTALLER_USE_7ZIP};
        $ENV{MOZ_PACKAGE_MSI} = "";
        $ENV{MOZ_INSTALLER_USE_7ZIP} = "";
        TinderUtils::run_shell_command("$Settings::Make -C $packaging_dir installer");
        $ENV{MOZ_PACKAGE_MSI} = $save_msi;
        $ENV{MOZ_INSTALLER_USE_7ZIP} = $save_7zip;
        TinderUtils::run_shell_command("cp -r $package_location/xpi $stagedir/windows-xpi");
      }
    } elsif (TinderUtils::is_linux()) {
      if ($Settings::stub_installer) {
        TinderUtils::run_shell_command("cp $package_location/stub/*.tar.* $stagedir/");
      }
      if ($Settings::sea_installer) {
        TinderUtils::run_shell_command("cp $package_location/sea/*.tar.* $stagedir/");
      }
      if ($push_raw_xpis) {
        my $xpi_loc = $package_location;
        if ($Settings::package_creation_path eq "/xpinstall/packager") {
          $xpi_loc = "$xpi_loc/raw";
        }
        TinderUtils::run_shell_command("cp -r $xpi_loc/xpi $stagedir/linux-xpi");
      }
    }
  } # do_installer

  # Extensions
  my $extensionStageDir;
  if ($Settings::ReleaseExtensionSubdir) {
    $extensionStageDir = $Settings::ReleaseExtensionSubdir;
  } elsif (TinderUtils::is_windows()) {
    $extensionStageDir = 'windows-xpi';
  } elsif (TinderUtils::is_os2()) {
    $extensionStageDir = 'os2-xpi';
  } elsif (TinderUtils::is_linux()) {
    $extensionStageDir = 'linux-xpi';
  } elsif (TinderUtils::is_mac()) {
    $extensionStageDir = 'mac-xpi';
  } else {
    return returnStatus('Platform specific staging directory unknown!', ('busted'));
  }

  foreach my $extension (@Settings::ReleaseExtensions) {
    my $extensionDistFile;
    if ($Settings::MacUniversalBinary) {
      $extensionDistFile = "$builddir/dist/universal/xpi-stage/$extension";
    } else {
      $extensionDistFile = "$builddir/dist/xpi-stage/$extension";
    }

    if ( scalar(grep { -f $_ } glob("$extensionDistFile")) eq 0 ) {
      return returnStatus("Extension $extensionDistFile for uploading was not found!", ('busted'));
    }

    TinderUtils::run_shell_command("mkdir -p $stagedir/$extensionStageDir") if ( ! -e "$stagedir/$extensionStageDir" );
    TinderUtils::run_shell_command("cp $extensionDistFile $stagedir/$extensionStageDir");
  }

  if ($Settings::archive) {
    TinderUtils::run_shell_command("$Settings::Make -C $packaging_dir");

    if (TinderUtils::is_windows()) {
      TinderUtils::run_shell_command("cp $package_location/../*.zip $stagedir/");
    } elsif (TinderUtils::is_mac()) {
      TinderUtils::run_shell_command("mkdir -p $package_location");

      # If .../*.dmg.gz exists, copy it to the staging directory.  Otherwise, copy
      # .../*.dmg if it exists.
      my @dmg;
      @dmg = grep { -f $_ } glob "${package_location}/../*.dmg.gz";
      if ( scalar(@dmg) eq 0 ) {
        @dmg = grep { -f $_ } glob "${package_location}/../*.dmg";
      }

      if ( scalar(@dmg) gt 0 ) {
        my $dmg_files = join(' ', @dmg);
        TinderUtils::print_log("Copying $dmg_files to $stagedir/\n");
        TinderUtils::run_shell_command("cp $dmg_files $stagedir/");
      } else {
        TinderUtils::print_log("No files to copy\n");
      }

      # Also copy any built archives (such as SDK)
      if (glob "$package_location/../*.zip") {
        TinderUtils::run_shell_command("cp $package_location/../*.zip $stagedir/");
      }
      if (glob "$package_location/../*.tar.*") {
        TinderUtils::run_shell_command("cp $package_location/../*.tar.* $stagedir/");
      }
    } elsif (TinderUtils::is_os2()) {
      TinderUtils::run_shell_command("cp $package_location/../*.zip $stagedir/");
    } else {
      my $archive_loc = "$package_location/..";
      if ($Settings::package_creation_path eq "/xpinstall/packager") {
        $archive_loc = "$archive_loc/dist";
      }
      TinderUtils::run_shell_command("cp $archive_loc/*.tar.* $stagedir/");
    }
  }

  if (!$status and $cachebuild and $Settings::update_package) {
    $status = update_create_package( objdir => $builddir,
                                     stagedir => $stagedir,
                                     url => $url,
                                     );
  }

  # need to reverse status, since it's a "unix" truth value, where 0 means 
  # success
  return ($status)?0:1;
}

sub pack_sdk {
  my ($packaging_dir, $package_location, $stagedir) = @_;
  TinderUtils::run_shell_command("make -C $packaging_dir make-sdk");

  if (TinderUtils::is_windows() || TinderUtils::is_os2()) {
    TinderUtils::run_shell_command("cp $package_location/../*.sdk.zip $stagedir/");
  } elsif (TinderUtils::is_mac()) {
    TinderUtils::run_shell_command("mkdir -p $package_location");
    TinderUtils::run_shell_command("cp $package_location/../*.sdk.tar.* $stagedir/");
  } else {
    my $archive_loc = "$package_location/..";
    if ($Settings::package_creation_path eq "/xpinstall/packager") {
      $archive_loc = "$archive_loc/dist";
    }
    TinderUtils::run_shell_command("cp $archive_loc/*.sdk.tar.* $stagedir/");
  }
}

sub get_buildid {
  my %args = @_;

  if (defined($Settings::buildid)) {
      return $Settings::buildid;
  }

  my $dist = $args{'dist'};
  my $objdir = $args{'objdir'};
  my $buildid;

  my $platformIni;
  if (defined($dist)) {
      $platformIni = "$dist/bin/platform.ini";
  }
  else {
      $platformIni = "$objdir/toolkit/xre/platform.ini";
  }
  if (-e $platformIni) {
      my $c = read_file($platformIni);
      if ($c =~ /^BuildID=(\d+)/m) {
          $buildid = $1;
      }
  }

  if (!defined($buildid) && $Settings::XULRunnerApp) {
    my @possibleBuildIdLocations = (
        "$objdir/xulrunner/dist/bin/platform.ini",
        "$objdir/xulrunner/toolkit/xre/platform.ini",        
    );
    foreach my $file (@possibleBuildIdLocations) {
      if (-e $file) {
        my $c = read_file($file);
        if ($c =~ /^BuildID=(\d+)/m) {
	  $buildid = $1;
          last;
        }
      }
    }
  }

  if (!defined($buildid) && defined($dist)) {
      # First try to get the build ID from the files in dist/.
      my $find_master = `find $dist -iname master.ini -print`;
      my @find_output = split(/\n/, $find_master);

      if (scalar(@find_output) gt 0) {
          my $master = read_file($find_output[0]);
          # BuildID = "2005100517"
          if ( $master =~ /^BuildID\s+=\s+\"(\d+)\"\s+$/m ) {
              $buildid = $1;
          }
      }
  }

  # If the first method of getting the build ID failed, grab it from config.
  if (!defined($buildid) and defined($objdir)) {
      if ( -f "$objdir/config/build_number" ) {
          $buildid = `cd $objdir/config/ && cat build_number`;
          chomp($buildid);
      }
  }

  if (defined($buildid)) {
      $Settings::buildid = $buildid;
  }

  return $buildid;
}

sub update_create_package {
    my %args = @_;

    my $status = 0;

    my $locale = $args{'locale'};
    my $objdir = $args{'objdir'};
    my $stagedir = $args{'stagedir'};
    my $distdir = $args{'distdir'};
    my $url = $args{'url'};

    # $distdir refers to the directory containing the app we plan to package.
    # Note: Other uses of $objdir/dist should not be changed to use $distdir.
    $distdir = "$objdir/dist" if !defined($distdir);
    $locale = "en-US" if !defined($locale);

    if ($^O eq "cygwin") {
      # need to convert paths for use by Cygwin utilities which treat
      # "drive:/path" interpret drive as a host.
      $objdir = `cygpath -u $objdir`;
      chomp($objdir);

      $stagedir = `cygpath -u $stagedir`;
      chomp($stagedir);

      $distdir = `cygpath -u $distdir`;
      chomp($distdir);
    }

    my($update_product, $update_version, $update_platform);
    my($update_appv, $update_extv);
    my $update_aus_host = $Settings::update_aus_host;

    if ( defined($Settings::update_product) ) {
        $update_product = $Settings::update_product;
    } else {
        TinderUtils::print_log("update_product is undefined, skipping update generation.\n");
        $status = 1;
        return $status;
    }

    if ( defined($Settings::update_version) ) {
        $update_version = $Settings::update_version;
    } else {
        TinderUtils::print_log("update_version is undefined, skipping update generation.\n");
        $status = 1;
        return $status;
    }

    if ( defined($Settings::update_platform) ) {
        $update_platform = $Settings::update_platform;
    } else {
        TinderUtils::print_log("update_platform is undefined, skipping update generation.\n");
        $status = 1;
        return $status;
    }

    if ( defined($Settings::update_appv) ) {
        $update_appv = $Settings::update_appv;
    } elsif ( defined($Settings::update_ver_file) ) {
        $update_appv = read_file("$Settings::TopsrcdirFull/$Settings::update_ver_file");
        chomp($update_appv);
    } else {
        TinderUtils::print_log("both update_appv and update_ver_file are undefined, skipping update generation.\n");
        $status = 1;
        return $status;
    }

    if ( defined($Settings::update_extv) ) {
        $update_extv = $Settings::update_extv;
    } elsif ( defined($Settings::update_ver_file) ) {
        $update_extv = read_file("$Settings::TopsrcdirFull/$Settings::update_ver_file");
        chomp($update_extv);
    } else {
        TinderUtils::print_log("both update_extv and update_ver_file are undefined, skipping update generation.\n");
        $status = 1;
        return $status;
    }

    # We're making an update.
    TinderUtils::print_log("\nGenerating complete update...\n");
    my $temp_stagedir = "$stagedir/build.$$";
    TinderUtils::run_shell_command("mkdir -p $temp_stagedir");

    my $up_temp_stagedir = $temp_stagedir;
    my $up_distdir = $distdir;
    if ($^O eq "cygwin") {
      # need to convert paths for use by the mar utility, which doesn't know
      # how to handle non-Windows paths.
      $up_temp_stagedir = `cygpath -m $up_temp_stagedir`;
      chomp($up_temp_stagedir);

      $up_distdir = `cygpath -m $up_distdir`;
      chomp($up_distdir);
    }

    TinderUtils::run_shell_command("$Settings::Make -C $objdir/tools/update-packaging full-update STAGE_DIR=$up_temp_stagedir DIST=$up_distdir AB_CD=$locale");

    my $update_file = "update.mar";
    my @updatemar;
    my $update_glob = "${temp_stagedir}/*.mar";
    @updatemar = grep { -f $_ } glob($update_glob);
    if ( scalar(@updatemar) ge 1 ) {
      $update_file = $updatemar[0];
      $update_file =~ s:^$temp_stagedir/(.*)$:$1:g;
    } else {
      TinderUtils::print_log("No MAR file found matching '$update_glob', update generation failed.\n");
      $status = 1;
      return $status;
    }

    TinderUtils::run_shell_command("cp -p $temp_stagedir/$update_file $stagedir/");

    my $update_path = "$stagedir/$update_file";
    my $update_fullurl = "$url/$update_file";

    if ( ! -f $update_path ) {
      TinderUtils::print_log("Error: Unable to get info on '$update_path' or include in upload because it doesn't exist!\n");
      $status = 1;
      return $status;
    } else {
      TinderUtils::run_shell_command("rm -rf $temp_stagedir");

      # Make update dist directory.
      TinderUtils::run_shell_command("mkdir -p $objdir/dist/update/");
      TinderUtils::print_log("\nGathering complete update info...\n");

      my $buildid = get_buildid( dist => $distdir );;

      TinderUtils::print_log("Got build ID $buildid.\n");
      # Gather stats for update file.
      update_create_stats( update => $update_path,
                           type => "complete",
                           output_file => "$objdir/dist/update/update.snippet",
                           url => $update_fullurl,
                           buildid => $buildid,
                           appversion => $update_appv,
                           extversion => $update_extv,
                         );

      # Push update information to update-staging/auslite.

      # Push the build schema 2 data.
      if ( $Settings::update_pushinfo ) {
          TinderUtils::print_log("\nPushing third-gen update info...\n");
          my $path = "/opt/aus2/build/0";
          $path = "$path/$update_product/$update_version/$update_platform/$buildid/$locale";

          my $ssh_opts = "";
          my $scp_opts = "";
          if (defined($Settings::ssh_key) && $Settings::ssh_key !~ /^\s*$/) {
             # NB: ssh_key must be quoted in the tinder-config.pl if the path contains spaces
             $ssh_opts .= " -i ".$Settings::ssh_key;
             $scp_opts .= " -i ".$Settings::ssh_key;
          }

          TinderUtils::run_shell_command("ssh $ssh_opts $Settings::ssh_user\@$update_aus_host mkdir -p $path");
          TinderUtils::run_shell_command("scp $scp_opts $objdir/dist/update/update.snippet.1 $Settings::ssh_user\@$update_aus_host:$path/complete.txt");
          TinderUtils::print_log("\nCompleted pushing update info...\n");
      }

    }

    TinderUtils::print_log("\nUpdate build completed.\n\n");
    return $status;
}

sub read_file {
  my ($filename) = @_;

  if ( ! -e $filename ) {
    die("read_file: file $filename doesn't exist!");
  }

  local($/) = undef;

  open(FILE, "$filename") or die("read_file: unable to open $filename for reading!");
  my $text = <FILE>;
  close(FILE);

  return $text;
}

sub update_create_stats {
  my %args = @_;
  my $update = $args{'update'};
  my $type = $args{'type'};
  my $output_file_base = $args{'output_file'};
  my $url = $args{'url'};
  my $buildid = $args{'buildid'};
  my $appversion = $args{'appversion'};
  my $extversion = $args{'extversion'};

  my($hashfunction, $size, $output, $output_file);

  ($size) = (stat($update))[7];

  # We only support md5 and sha1, and sha1 is the default, so create md5
  # sums if that's what requested; otherwise (it's either sha1 or bogus
  # input), sha1.
  $hashfunction = ($Settings::update_hash eq 'md5') ? 'md5' : 'sha1';

  my $hashvalue = TinderUtils::HashFile(function => $hashfunction,
   file => $update); 

  if ( defined($Settings::update_filehost) ) {
    $url =~ s|^([^:]*)://([^/:]*)(.*)$|$1://$Settings::update_filehost$3|g;
  }

  $output  = "$type\n";
  $output .= "$url\n";
  $output .= "$hashfunction\n";
  $output .= "$hashvalue\n";
  $output .= "$size\n";
  $output .= "$buildid\n";

  $output_file = "$output_file_base.0";
  if (defined($output_file)) {
    open(UPDATE_FILE, ">$output_file")
      or die "ERROR: Can't open '$output_file' for writing!";
    print UPDATE_FILE $output;
    close(OUTPUT_FILE);
  } else {
    printf($output);
  }

  $output  = "$type\n";
  $output .= "$url\n";
  $output .= "$hashfunction\n";
  $output .= "$hashvalue\n";
  $output .= "$size\n";
  $output .= "$buildid\n";
  $output .= "$appversion\n";
  $output .= "$extversion\n";

  $output_file = "$output_file_base.1";
  if (defined($output_file)) {
    open(UPDATE_FILE, ">$output_file")
      or die "ERROR: Can't open '$output_file' for writing!";
    print UPDATE_FILE $output;
    close(OUTPUT_FILE);
  } else {
    printf($output);
  }
}

sub packit_l10n {
  my ($srcdir, $objdir, $packaging_dir, $package_location, $url, $stagedir, $cachebuild) = @_;
  my $status = 0;

  TinderUtils::print_log("Starting l10n builds\n");

  foreach my $urlKey (keys(%Settings::WGetFiles)) {
    # We do this in case we need to munge $wgeturl in the if-branch below.
    my $wgeturl = $urlKey;
    if (defined($Settings::LocalizationVersionFile)) {
      my $l10nVersion = read_file("$Settings::TopsrcdirFull/$Settings::LocalizationVersionFile");
      chomp($l10nVersion);
      $l10nVersion =~ s/\r//g if (TinderUtils::is_windows()); # yay win32!
      $wgeturl =~ s/%version%/$l10nVersion/g;
    }

    my $status = TinderUtils::run_shell_command_with_timeout("wget -nv --output-document \"$Settings::WGetFiles{$urlKey}\" $wgeturl",
                                                             $Settings::WGetTimeout);
    if ($status->{exit_value} != 0) {
      TinderUtils::print_log("Error: wget failed or timed out.\n");
      return (("busted"));
    }
  }

  unless (open(ALL_LOCALES, "<$srcdir/$Settings::LocaleProduct/locales/all-locales")) {
      TinderUtils::print_log("Error: Couldn't read $srcdir/$Settings::LocaleProduct/locales/all-locales.\n");
      return (("testfailed"));
  }

  my @locales = <ALL_LOCALES>;
  close ALL_LOCALES;

  map { /([a-z-]+)/i; $_ = $1 } @locales;

  run_locale_shell_command "mkdir -p $stagedir";

  TinderUtils::print_log("Building following locales: @locales\n");

  my $start_time = TinderUtils::adjust_start_time(time());
  foreach my $locale (@locales) {
      mail_locale_started_message($start_time, $locale);
      TinderUtils::print_log("$locale...");

      my $logfile = "$Settings::DirName-$locale.log";
      my $tinderstatus = 'success';
      open LOCLOG, ">$logfile";

      # Make the log file flush on every write.
      my $oldfh = select(LOCLOG);
      $| = 1;
      select($oldfh);

    if (do_installer()) {
      if (TinderUtils::is_windows()) {
        $ENV{INSTALLER_URL} = "$url/windows-xpi";
      } elsif (TinderUtils::is_linux()) {
        $ENV{INSTALLER_URL} = "$url/linux-xpi/";
      } else {
        die "Can't make installer for this platform.\n";
      }
  
      run_locale_shell_command "mkdir -p $package_location";
  
      # the Windows installer scripts currently require Activestate Perl.
      # Put it ahead of cygwin perl in the path.
      my $save_path;
      if (TinderUtils::is_windows()) {
        $save_path = $ENV{PATH};
        $ENV{PATH} = $Settings::as_perl_path.":".$ENV{PATH};
      }

      # the one operation we care about saving status of
      $status = run_locale_shell_command "$Settings::Make -C $objdir/$Settings::LocaleProduct/locales installers-$locale $Settings::BuildLocalesArgs";
      if ($status != 0) {
        $tinderstatus = 'busted';
      }
  
      if ($tinderstatus eq 'success') {
        if (TinderUtils::is_windows()) {
          run_locale_shell_command "mkdir -p $stagedir/windows-xpi/";
          run_locale_shell_command "cp $package_location/*$locale.langpack.xpi $stagedir/windows-xpi/$locale.xpi";
        } elsif (TinderUtils::is_mac()) {
          # Instead of adding something here (which will never be called),
          # please add your code to the TinderUtils::is_mac() section below.
        } elsif (TinderUtils::is_linux()) {
          run_locale_shell_command "mkdir -p $stagedir/linux-xpi/";
          run_locale_shell_command "cp $package_location/*$locale.langpack.xpi $stagedir/linux-xpi/$locale.xpi";
        }
      }

      if (TinderUtils::is_windows()) {
        $ENV{PATH} = $save_path;
        #my $dos_stagedir = `cygpath -w $stagedir`;
        #chomp ($dos_stagedir);
      }
  
      if (TinderUtils::is_windows()) {
        if ($Settings::stub_installer && $tinderstatus ne 'busted' ) {
          run_locale_shell_command "cp $package_location/stub/*.exe $stagedir/";
        }
        if ($Settings::sea_installer && $tinderstatus ne 'busted' ) {
          run_locale_shell_command "cp $package_location/sea/*.exe $stagedir/";
        }
      } elsif (TinderUtils::is_linux()) {
        if ($Settings::stub_installer && $tinderstatus ne 'busted' ) {
          run_locale_shell_command "cp $package_location/stub/*.tar.* $stagedir/";
        }
        if ($Settings::sea_installer && $tinderstatus ne 'busted' ) {
          run_locale_shell_command "cp $package_location/sea/*.tar.* $stagedir/";
        }
      }
    } # do_installer
  
    if ($Settings::archive && $tinderstatus ne 'busted' ) {
      if (TinderUtils::is_windows()) {
        run_locale_shell_command "cp $package_location/../*$locale*.zip $stagedir/";
      } elsif (TinderUtils::is_mac()) {
        $status = run_locale_shell_command "$Settings::Make -C $objdir/$Settings::LocaleProduct/locales installers-$locale $Settings::BuildLocalesArgs";
        if ($status != 0) {
          $tinderstatus = 'busted';
        }
        run_locale_shell_command "mkdir -p $package_location";

        # If .../*.dmg.gz exists, copy it to the staging directory.  Otherwise, copy
        # .../*.dmg if it exists.
        my @dmg;
        @dmg = grep { -f $_ } glob "${package_location}/../*$locale*.dmg.gz";
        if ( scalar(@dmg) eq 0 ) {
          @dmg = grep { -f $_ } glob "${package_location}/../*$locale*.dmg";
        }

        if ( scalar(@dmg) gt 0 ) {
          my $dmg_files = join(' ', @dmg);
          TinderUtils::print_log("Copying $dmg_files to $stagedir/\n");
          TinderUtils::run_shell_command("cp $dmg_files $stagedir/");
        } else {
          TinderUtils::print_log("No files to copy\n");
        }

        if ($tinderstatus eq 'success') {
          run_locale_shell_command "mkdir -p $stagedir/mac-xpi/";
          run_locale_shell_command "cp $package_location/*$locale.langpack.xpi $stagedir/mac-xpi/$locale.xpi";
        }
      } else {
        my $archive_loc = "$package_location/..";
        if ($Settings::package_creation_path eq "/xpinstall/packager") {
          $archive_loc = "$archive_loc/dist";
        }
        run_locale_shell_command "cp $archive_loc/*$locale*.tar.* $stagedir/";
      }

      if ($tinderstatus eq 'success' and
          $cachebuild and
          $Settings::update_package) {
        if ( ! -d "$objdir/dist/l10n-stage" ) {
            die "packit_l10n: $objdir/dist/l10n-stage is not a directory!\n";
        }

        if ( ! -d "$objdir/dist/host" ) {
            die "packit_l10n: $objdir/dist/host is not a directory!\n";
        }

        system("cd $objdir/dist/l10n-stage; ln -s ../host .");
        $status = update_create_package( objdir => $objdir,
                                         stagedir => $stagedir,
                                         url => $url,
                                         locale => $locale,
                                         distdir => "$objdir/dist/l10n-stage",
                                         );

        $tinderstatus = 'busted' if ($status);
      }
    }

    for my $localetest (@Settings::CompareLocaleDirs) {
      my $originaldir = "$srcdir/$localetest/locales/en-US";
      my $localedir;
      if ($Settings::CompareLocalesAviary) {
        $localedir = "$srcdir/$localetest/locales/$locale";
      }
      else {
        $localedir = "$srcdir/../l10n/$locale/$localetest";
      }
      $status = run_locale_shell_command "$^X $srcdir/toolkit/locales/compare-locales.pl $originaldir $localedir";
      if ($tinderstatus eq 'success' && $status != 0) {
        $tinderstatus = 'testfailed';
      }
    }
    close LOCLOG;

    mail_locale_finished_message($start_time, $tinderstatus, $logfile, $locale);
    TinderUtils::print_log("$tinderstatus.\n");

  } # foreach

  # remove en-US files if we're building that on a different system
  if ($Settings::ConfigureOnly) {
    TinderUtils::run_shell_command("rm -f $stagedir/*en-US* $stagedir/*-xpi");
  }

  TinderUtils::print_log("locales completed.\n");

    # need to reverse status, since it's a "unix" truth value, where 0 means 
    # success
  return ($status)?0:1;

} # packit_l10n
        

sub pad_digit {
  my ($digit) = @_;
  if ($digit < 10) { return "0" . $digit };
  return $digit;
}

sub pushit {
  my %args = @_;

  my $ssh_server = $args{'server'};
  my $remote_path = $args{'remote_path'};
  my $package_name = $args{'package_name'};
  my $store_path = $args{'store_path'};
  my $store_path_packages = $args{'store_path_packages'};
  my $cachebuild = $args{'cachebuild'};
  my $start_time = $args{'start_time'};

  my $upload_directory = $store_path . "/" . $store_path_packages;

  my @cmds;

  unless ( -d $upload_directory) {
    TinderUtils::print_log("No $upload_directory to upload\n");
    return 0;
  }

  if ($^O eq "cygwin") {
    # need to convert the path in case we're using activestate perl
    $upload_directory = `cygpath -u $upload_directory`;
  }
  chomp($upload_directory);

  my $short_ud = $package_name;

  my $ssh_opts = "";
  my $scp_opts = "";
  if (defined($Settings::ssh_version) && $Settings::ssh_version ne '') {
    $ssh_opts = "-".$Settings::ssh_version;
    $scp_opts = "-oProtocol=".$Settings::ssh_version;
  }
  if (defined($Settings::ssh_key) && $Settings::ssh_key !~ /^\s*$/) {
    $ssh_opts .= " -i ".$Settings::ssh_key;
    $scp_opts .= " -i ".$Settings::ssh_key;
  }

  # The ReleaseToDated and ReleaseToLatest configuration settings give us the
  # ability to fine-tune where release files are stored.  ReleaseToDated
  # will store the release files in a directory of the form
  #
  #   nightly/YYYY/MM/YYYY-MM-DD-HH-<milestone>
  #
  # while ReleaseToLatest stores the release files in a directory of the form
  #
  #   nightly/latest-<milestone>
  #
  # Before we allowed the fine-tuning, we either published to both dated and
  # latest, or to neither.  In case some installations don't have these
  # variables defined yet, we want to set them to default values here.
  # Hopefully, though, we'll have also updated tinder-defaults.pl if we've
  # updated post-mozilla-rel.pl from CVS.
  #
  # Note that we traverse this code for "hourlies" as well as nightlies/clobbers
  # $package_name, and hence $short_ud,  would be (eg) 
  # "fx-linux-tbox-trunk/1202546936" if DependToDated is set, 
  # "fx-linux-tbox-trunk/" otherwise, and "2008-02-09-00-trunk" for clobbers.

  $Settings::ReleaseToDated = 1 if !defined($Settings::ReleaseToDated);
  $Settings::ReleaseToLatest = 1 if !defined($Settings::ReleaseToLatest);

  my $complete_remote_path; 
  if ( $Settings::ReleaseToDated ) {
    my $datedir = '';
    if( $cachebuild && ($short_ud =~ /^(\d{4})-(\d{2}).*/) ) {
      $datedir = "$1/$2/";
      $complete_remote_path = $remote_path . "/" . $datedir . $short_ud;
    } elsif (! $cachebuild && $Settings::DependToDated) {
      $datedir = $start_time;
      $complete_remote_path = $remote_path . "/" . $short_ud . "/" . $datedir;
    } else {
      $complete_remote_path = $remote_path . "/" . $short_ud;
    }
    my $makedirs = $complete_remote_path;
    if ($cachebuild && $Settings::ReleaseToLatest) {
      $makedirs .= " $remote_path/latest-$Settings::milestone";
    }
    push(@cmds,"ssh $ssh_opts -l $Settings::ssh_user $ssh_server mkdir -p $makedirs");
    # this is a workaround for pacifica-vm, which doesn't support the <dir>/. notation for scp (bug 383775)
    if ($^O eq "cygwin") {
      push(@cmds,"rsync -av -e \"ssh $ssh_opts\" $upload_directory/ $Settings::ssh_user\@$ssh_server:$complete_remote_path/");
    } else {
      push(@cmds,"scp -r -p $scp_opts $upload_directory/. $Settings::ssh_user\@$ssh_server:$complete_remote_path/");
    }
    push(@cmds,"ssh $ssh_opts -l $Settings::ssh_user $ssh_server chmod -R 775 $complete_remote_path");
    if ($Settings::ReleaseGroup ne '') {
      push(@cmds,"ssh $ssh_opts -l $Settings::ssh_user $ssh_server chgrp -R $Settings::ReleaseGroup $complete_remote_path");
    }

    if ( $cachebuild and $Settings::ReleaseToLatest ) {
      push(@cmds,"ssh $ssh_opts -l $Settings::ssh_user $ssh_server  rsync -avz $complete_remote_path/ $remote_path/latest-$Settings::milestone/");
    }

    if ( $cachebuild && $datedir ne '') {
      push(@cmds,"ssh $ssh_opts -l $Settings::ssh_user $ssh_server ln -sv ${datedir}${short_ud} $remote_path/");
    }
  } elsif ( $Settings::ReleaseToLatest ) {
    push(@cmds,"ssh $ssh_opts -l $Settings::ssh_user $ssh_server mkdir -p $remote_path/latest-$Settings::milestone");
    push(@cmds,"scp $scp_opts -r $upload_directory/. $Settings::ssh_user\@$ssh_server:$remote_path/latest-$Settings::milestone/");
    push(@cmds,"ssh $ssh_opts -l $Settings::ssh_user $ssh_server chmod -R 775 $remote_path/latest-$Settings::milestone/");
    if ($Settings::ReleaseGroup ne '') {
      push(@cmds,"ssh $ssh_opts -l $Settings::ssh_user $ssh_server chgrp -R $Settings::ReleaseGroup $remote_path/latest-$Settings::milestone/");
    }
  }

  my $oldwd = getcwd();
  chdir($store_path);

  my $upload_script_filename = "upload-packages.sh";
  open(UPLOAD_SCRIPT, ">$upload_script_filename") or die "ERROR: Couldn't create $upload_script_filename: $!";
  print UPLOAD_SCRIPT "#!/bin/sh\n\n";

  for my $c (@cmds) {
    print UPLOAD_SCRIPT "echo $c\n";
    print UPLOAD_SCRIPT "$c\n";
    print UPLOAD_SCRIPT "if [ \$? != 0 ]; then\n";
    print UPLOAD_SCRIPT "  echo \"command failed!\"\n";
    print UPLOAD_SCRIPT "  exit 1\n";
    print UPLOAD_SCRIPT "fi\n";
    print UPLOAD_SCRIPT "echo\n\n";
  }

  print UPLOAD_SCRIPT "exit 0\n";
  close(UPLOAD_SCRIPT);

  TinderUtils::run_shell_command("chmod 755 $upload_script_filename");
  TinderUtils::run_shell_command("./$upload_script_filename");

  chdir($oldwd);

  return 1;
}

sub reportRelease {
  my ($url, $datestamp)       = @_;

  if ($Settings::notify_list ne "") {
    my $donemessage =   "\n" .
                        "$Settings::OS $Settings::ProductName Build available at: \n" .
                        "$url \n";
    open(TMPMAIL, ">tmpmail.txt");
    print TMPMAIL "$donemessage \n";
    close(TMPMAIL);

    TinderUtils::print_log ("$donemessage \n");
    my $subject = "$datestamp $Settings::ProductName $Settings::OS Build Complete";
    if ($Settings::blat ne "" && $Settings::use_blat) {
      system("$Settings::blat tmpmail.txt -to $Settings::notify_list -s \"$subject\"");
    } else {
      system "$Settings::mail -s \"$subject\" $Settings::notify_list < tmpmail.txt";
    }
    unlink "tmpmail.txt";
  }
  return (("success", "$url"));
}

sub cleanup {
  # As it's a temporary file meant to control the last-built file, remove
  # last-built.new when we are done.

  if ( -e "last-built.new" ) {
    unlink "last-built.new";
  }
}

sub returnStatus{
  # build-seamonkey-util.pl expects an array, if no package is uploaded,
  # a single-element array with 'busted', 'testfailed', or 'success' works.
  my ($logtext, @status) = @_;
  TinderUtils::print_log("$logtext\n");
  return @status;
}

sub PreBuild {
  # assert that needed variables are defined as the build scripts expect
  if (TinderUtils::is_mac() and !defined($Settings::mac_bundle_path)) {
    die "ERROR: mac_bundle_path unset!";
  }

  # last-built.new is used to track a respin as it is currently happening and
  # later takes the place of last-built.  If it exists at this point in the
  # build, remove it because we do not want old last-built.new files to
  # introduce noise or odd conditions in the build.

  if ( -e "last-built.new" ) {
    unlink "last-built.new";
  }

  if ($Settings::ForceRebuild) {
    TinderUtils::print_log("Force rebuild requested; removing last-built\n");
    if (-e 'last-built') {
      unlink 'last-built';
      TinderUtils::print_log("Removal of last-built failed?\n")
       if (-e 'last-built');
    } else {
      TinderUtils::print_log("Force rebuild requested, but last-built not found.\n");
    }
  }

  # We want to be able to remove the last-built file at any time of
  # day to trigger a respin, even if it's before the designated build
  # hour.  This means that we want to do a cached build if any of the
  # following are true:
  # 1. there is no last-built file
  # 2. the last-built file is more than 24 hours old
  # 3. the last-built file is before the most recent occurrence of the
  #    build hour (which may not be today!)
  # (3) subsumes (2), so there's no need to check for (2) explicitly.

  if (!$Settings::OfficialBuildMachinery) {
    TinderUtils::print_log("Configured to release hourly builds only\n");
    $cachebuild = 0;
  }
  elsif ( -e "last-built" ) {
    my $last = (stat "last-built")[9];
    TinderUtils::print_log("Most recent nightly build: " .
      localtime($last) . "\n");

    # Figure out the most recent past time that's at
    # $Settings::build_hour.
    my $now = time;
    my $most_recent_build_hour =
      POSIX::mktime(0, 0, $Settings::build_hour,
                    (localtime($now))[3 .. 8]);
    if ($most_recent_build_hour > $now) {
      $most_recent_build_hour -= 86400; # subtract a day
    }
    TinderUtils::print_log("Most recent build hour:    " .
      localtime($most_recent_build_hour) . "\n");

    # If that time is older than last-built, then it's time for a new
    # nightly.
    $cachebuild = ($most_recent_build_hour > $last);
  } else {
    # No last-built file.  Either it's the first build or somebody
    # removed it to trigger a respin.
    TinderUtils::print_log("No record of generating a nightly build.\n");
    $cachebuild = 1;
  }

  if ($cachebuild) {
    # Within a cachebuild cycle (when it's determined a release build is
    # needed), create a last-built.new file.  Later, if last-built.new exists,
    # we move last-built.new to last-built.  If last-built.new doesn't exist,
    # we do nothing.
    #
    # This allows us to remove last-built.new and have the next build cycle
    # be a respin even if the current cycle already is one.

    if ( ! -e "last-built.new" ) {
      open BLAH, ">last-built.new";
      close BLAH;
    }

    TinderUtils::print_log("Starting nightly release build\n");
    # clobber the tree if set to do so
    $Settings::clean_objdir = 1 if !defined($Settings::clean_objdir);
    $Settings::clean_srcdir = 1 if !defined($Settings::clean_srcdir);
    if ($Settings::ObjDir eq '') {
      $Settings::clean_srcdir = ($Settings::clean_srcdir || $Settings::clean_objdir);
      $Settings::clean_objdir = 0;
    }
    if ($Settings::clean_objdir) {
      my $objdir = 'mozilla/'.$Settings::ObjDir;
      if ( -d $objdir) {
        TinderUtils::run_shell_command("rm -rf $objdir");
      }
    }
    if ($Settings::clean_srcdir) {
      if ( -d "mozilla") {
        TinderUtils::run_shell_command("rm -rf mozilla");
        $cachebuild = 1;
      }
    }
    if ( -d "l10n" ) {
      TinderUtils::run_shell_command("rm -rf l10n");
    }
  } else {
    TinderUtils::print_log("Starting non-release build\n");
  }
}

sub unix2dos {
  my ($path) = @_;
  -e $path or return;

  local($/, *FH);
  open(FH, "+<$path") || die("Couldn't open file $path for unix2dos");
  binmode(FH);
  my $data = <FH>;
  $data =~ s/(?<!\r)\n/\r\n/g;
  seek(FH, 0, 0);
  print FH $data || die("Couldn't write to file $path during unix2dos");
  close FH || die("Couldn't close file $path during unix2dos");
}
  
sub main {
  # Get build directory from caller.
  my ($mozilla_build_dir, $server_start_time) = @_;
  TinderUtils::print_log("Post-Build packaging/uploading commencing.\n");

  chdir $mozilla_build_dir;

  if (0 and $^O eq "cygwin") {
    $mozilla_build_dir = `cygpath -m $mozilla_build_dir`; # cygnusify the path
    chomp($mozilla_build_dir); # remove whitespace
  
    if ( -e $mozilla_build_dir ) {
      TinderUtils::print_log("Using cygnified $mozilla_build_dir to make packages in.\n");
    } else {
      TinderUtils::print_log("No cygnified $mozilla_build_dir to make packages in.\n");
      return (("testfailed")) ;
    }
  }

  my $srcdir = "$mozilla_build_dir/${Settings::Topsrcdir}";
  my $objdir = $srcdir;
  if ($Settings::ObjDir ne '') {
    $objdir .= "/${Settings::ObjDir}";
    if ($Settings::MacUniversalBinary) {
      $objdir .= '/ppc';
    }
  }
  unless ( -e $objdir) {
    TinderUtils::print_log("No $objdir to make packages in.\n");
    return (("testfailed")) ;
  }

  TinderUtils::run_shell_command("rm -f $objdir/dist/bin/codesighs");

  # set up variables with default values
  # need to modify the settings from tinder-config.pl
  my $package_creation_path = $objdir . $Settings::package_creation_path;
  my $package_location;
  if (TinderUtils::is_windows() || 
      TinderUtils::is_mac() || 
      TinderUtils::is_os2() || 
      $Settings::package_creation_path ne "/xpinstall/packager") {
    $package_location = $objdir . "/dist/install";
  } else {
    $package_location = $objdir . "/installer";
  }
  my $ftp_path = $Settings::ftp_path;
  my $url_path = $Settings::url_path;

  my $buildid = get_buildid(objdir=>$objdir);
  TinderUtils::print_log("buildid: $buildid\n");

  my $datestamp;
  if ($buildid ne '0000000000' &&
      $buildid =~ /^(\d{4})(\d{2})(\d{2})(\d{2})$/) {
    $datestamp="$1-$2-$3-$4-$Settings::milestone";
  }
  else {
    ## XXX - This code sucks.
    ## For l10n builds, we don't have an objdir or distdir, so getting the 
    ## build ID is... non-trivial. And since we have a release in a week, 
    ## it's time to resurrect old code that works.
    ## But only for the l10n builds. -jpr
    if ($Settings::BuildLocales) {
      my ($c_hour,$c_day,$c_month,$c_year,$c_yday) = (localtime(time))[2,3,4,5,7];
      $c_year       = $c_year + 1900; # ftso perl
      $c_month      = $c_month + 1; # ftso perl
      $c_hour       = pad_digit($c_hour);
      $c_day        = pad_digit($c_day);
      $c_month      = pad_digit($c_month);
      $datestamp = "$c_year-$c_month-$c_day-$c_hour-$Settings::milestone";
    } else {
      cleanup();
      return returnStatus('Could not find build id', ('busted'));
    }
  }

  my($package_dir, $store_name, $local_build_dir);

  my $pretty_build_name = TinderUtils::ShortHostname() . "-" . $Settings::milestone;

  if ($cachebuild) {
    TinderUtils::print_log("Uploading nightly release build\n");
    $store_name = $buildid;
    $package_dir = "$datestamp";
    $url_path         = $url_path . "/" . $package_dir;
  } else {
    $package_dir = $pretty_build_name;
    $ftp_path   = $Settings::tbox_ftp_path;
    $url_path   = $Settings::tbox_url_path . "/" . $package_dir;
  }

  if (!defined($store_name)) {
    $store_name = $pretty_build_name;
  }
  if (!TinderUtils::is_os2()) {
    my $rv = processtalkback($cachebuild && $Settings::shiptalkback, $objdir, "$mozilla_build_dir/${Settings::Topsrcdir}");
    if ($rv) {
      return returnStatus("Talkback processing failed.", ("busted"));
    }
  }

  if ($cachebuild && $Settings::crashreporter_buildsymbols) {
    if ($Settings::BinaryName ne 'Camino') {
      TinderUtils::run_shell_command("$Settings::Make -C $objdir buildsymbols");
    } else {
      TinderUtils::run_shell_command("$Settings::Make -C $objdir/camino buildcaminosymbols");
      if ($Settings::MacUniversalBinary) {
        my $i386objdir = $objdir . '/../i386';
        TinderUtils::run_shell_command("$Settings::Make -C $i386objdir/camino buildcaminosymbols");
      }
    }
  }
  if ($cachebuild && $Settings::crashreporter_pushsymbols) {
    if ($Settings::BinaryName ne 'Camino') {
      TinderUtils::run_shell_command("$Settings::Make -C $objdir uploadsymbols");
    } else {
      TinderUtils::run_shell_command("$Settings::Make -C $objdir/camino uploadcaminosymbols");
      if ($Settings::MacUniversalBinary) {
        my $i386objdir = $objdir . '/../i386';
        TinderUtils::run_shell_command("$Settings::Make -C $i386objdir/camino uploadcaminosymbols");
      }
    }
  }

  $local_build_dir = $package_location . "/" . $package_dir;

  if (!$Settings::ConfigureOnly) {
    unless (packit($package_creation_path,$package_location,$url_path,$local_build_dir,$objdir,$cachebuild)) {
      cleanup();
      return returnStatus("Packaging failed", ("testfailed"));
    }
    if ($Settings::BuildSDK) {
      pack_sdk($package_creation_path,$package_location,$local_build_dir);
      if ($Settings::MacUniversalBinary) {
        my $i386objdir = $objdir . '/../i386';
        pack_sdk($i386objdir . $Settings::package_creation_path,$i386objdir . '/dist/install',$local_build_dir);
      }
    }
  }

  if ($Settings::BuildLocales) {

    # Check for existing mar tool (only on non-Aviary branches).
    if (not $Settings::CompareLocalesAviary) {
      my $mar_tool = "$srcdir/modules/libmar/tool/mar";
      if (! -f $mar_tool) {
        TinderUtils::run_shell_command("$Settings::Make -C $srcdir/nsprpub && $Settings::Make -C $srcdir/config && $Settings::Make -C $srcdir/modules/libmar");
        # mar tool should exist now.
        if (! -f $mar_tool) {
          TinderUtils::print_log("Failed to build $mar_tool\n");
        }
      }
    }

    packit_l10n($srcdir,$objdir,$package_creation_path,$package_location,$url_path,$local_build_dir,$cachebuild);
  }

  my $store_home;
  $store_home = "$mozilla_build_dir/$store_name";

  if ($^O eq "cygwin") {
    # need to convert the path in case we're using activestate perl or
    # cygwin rsync
    $local_build_dir = `cygpath -u $local_build_dir`;
    $store_home = `cygpath -u $store_home`;
  }
  chomp($local_build_dir);
  chomp($store_home);

  if ( -e "$store_home/packages" ) {
    # remove old storage directory
    TinderUtils::print_log("Found old storage directory.  Removing.\n");
    TinderUtils::run_shell_command("rm -rf $store_home/packages");
  }

  # save the current build in the appropriate store directory
  TinderUtils::run_shell_command("mkdir -p $store_home/packages");
  TinderUtils::run_shell_command("cp -r -p -v $local_build_dir/. $store_home/packages/");

  if ($cachebuild) { 
    # remove saved builds older than a week and save current build
    TinderUtils::run_shell_command("find $mozilla_build_dir -maxdepth 1 -type d -name \"20????????\" -mtime +7 -prune -print | xargs rm -rf");
  }

  unless (pushit(server => $Settings::ssh_server, remote_path => $ftp_path, package_name => $package_dir,
                 store_path => $store_home, store_path_packages => "packages", cachebuild => $cachebuild, start_time => $server_start_time)) {
    cleanup();
    return returnStatus("Pushing package $local_build_dir failed", ("testfailed"));
  }

  TinderUtils::run_shell_command("rm -rf $local_build_dir");

  if ($cachebuild) { 
    # Above, we created last-built.new at the start of the cachebuild cycle.
    # If the last-built.new file still exists, move it to the last-built file.

    if ( -e "last-built.new" ) {
      system("mv last-built.new last-built");
    }

    return reportRelease ("$url_path\/", "$datestamp");
  } else {
    return (("success"));
  }
} # end main

# Need to end with a true value, (since we're using "require").
1;
