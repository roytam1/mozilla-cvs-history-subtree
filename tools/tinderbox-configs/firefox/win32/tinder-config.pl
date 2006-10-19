$BuildAdministrator = 'benjamin@smedbergs.us';

$DisplayServer = ':0.0';

$ProductName              = "Firefox";
$VendorName               = 'Mozilla';

$RunMozillaTests          = 0;

$CVSCheckoutTimeout       = 600;

$MozConfigFileName = 'mozconfig';

$UseMozillaProfile = 0;

$moz_cvsroot   = ':pserver:anonymous@cvs-mirror.mozilla.org:/cvsroot';

#- Set these proper values for your tinderbox server
#$Tinderbox_server = 'tinderbox-daemon@tinderbox.mozilla.org';

#- Set if you want to build in a separate object tree
$ObjDir = 'obj-combined';
$SubObjDir = 'browser/';

$UserComment = 'bsmedberg';

$BuildTree  = 'MozillaExperimental';
$BuildNameExtra = 'FF-XR';

$BinaryName = 'application.ini';
$RequireExecutableBinary = 0;

#$LogCompression = 'bzip2';
#$LogEncoding = 'base64';

$blat = 'c:/Util/blat.exe';
$use_blat = 1;

$Make = 'make';
