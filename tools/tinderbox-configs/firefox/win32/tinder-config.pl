$BuildAdministrator = 'benjamin@smedbergs.us';

$DisplayServer = ':0.0';

$ProductName              = "Firefox";
$VendorName               = 'Mozilla';

$RunMozillaTests          = 0;

$CVSCheckoutTimeout       = 3600;

$MozConfigFileName = 'mozconfig';

$UseMozillaProfile = 0;

$moz_cvsroot   = ':ext:xrbld@cvs.mozilla.org:/cvsroot';

#- Set these proper values for your tinderbox server
#$Tinderbox_server = 'tinderbox-daemon@tinderbox.mozilla.org';

#- Set if you want to build in a separate object tree
$ObjDir = 'obj-combined';
$SubObjDir = 'browser/';

$XULRunnerApp = 1;

$UserComment = 'bsmedberg';

$BuildTree  = 'MozillaExperimental';
$BuildNameExtra = 'FF-XR';

$BinaryName = 'application.ini';
$RequireExecutableBinary = 0;

#$LogCompression = 'bzip2';
#$LogEncoding = 'base64';

$blat = '/d/mozilla-build/blat261/full/blat.exe';
$use_blat = 1;

$Make = 'make';

$ENV{MOZ_INSTALLER_USE_7ZIP} = '1';
$ENV{NO_EM_RESTART} = '1';
$ENV{MOZ_PACKAGE_NSIS} = '1';
$ENV{MOZ_CRASHREPORTER_NO_REPORT} = '1';

# Release build options
$ReleaseBuild  = 1;
$shiptalkback  = 0;
$ReleaseToLatest = 0; # Push the release to latest-<milestone>?
$ReleaseToDated = 1; # Push the release to YYYY-MM-DD-HH-<milestone>?
$build_hour    = "3";
$package_creation_path = "/browser/installer";
$ftp_path      = "/home/ftp/pub/firefox/nightly/experimental/ff-xr";
$url_path      = "http://ftp.mozilla.org/pub/mozilla.org/firefox/nightly/experimental/ff-xr";
$tbox_ftp_path = "/home/ftp/pub/firefox/tinderbox-builds/experimental";
$tbox_url_path = "http://ftp.mozilla.org/pub/mozilla.org/firefox/tinderbox-builds/experimental";
$milestone     = "xulrunner-apps";
#$notify_list   = 'build-announce@mozilla.org';
$stub_installer = 0;
$sea_installer = 1;
$archive       = 1;
