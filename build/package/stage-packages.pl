use File::Spec;
use Getopt::Long;
use MozPackager;

Getopt::Long::Configure ("bundling");

my $objdir = "";
my $packageList = "";
my $packagesDir = "";
my $warnMissing = 0;
my $ignoreMissing = 0;
my $xptMergeFile = "";
my @handlers;
my $stageDir = "";
my %mappings;
my $preprocessor = "";
my $xpiResult = "";
my $calcDiskSpace = 0;

# xxxbsmedberg: need usage()

# package names are specified after all options

GetOptions("objdir|o=s"           => \$objdir,
           "package-list|l=s"     => \$packageList,
           "packages-dir|p=s"     => \$packagesDir,
           "warn-missing|w"       => \$warnMissing,
           "ignore-missing|i"     => \$ignoreMissing,
           "verbose|v+"           => \$MozPackager::verbosity,
           "command-handlers|c=s" => \@handlers,
           "xpt-merge-file|x=s"   => \$xptMergeFile,
           "stage-directory|s=s"  => \$stageDir,
           "mapping|m=s"          => \%mappings,
           "force-copy|f"         => \$MozStage::forceCopy,
           "preprocessor|p=s"     => \$preprocessor,
           "make-xpi=s"           => \$xpiResult,
           "compute-disk-space|d" => \$calcDiskSpace);

@handlers = split(/,/,join(',',@handlers));

$packageList || die("Specify --package-list=");

if ($packagesDir) {
    $packagesDir = File::Spec->rel2abs($packagesDir);
} else {
    $packagesDir = File::Spec->catdir("dist", "packages")
}    
if ($stageDir) {
    $stageDir = File::Spec->rel2abs($stageDir);
} else {
    $stageDir = "stage";
}
if ($xpiResult) {
    $xpiResult = File::Spec->rel2abs($xpiResult);
}

chdir($objdir) if $objdir;
-d "dist" || die("directory dist/ not found... perhaps you forgot to specify --objdir?");

MozPackager::_verbosePrint(1, "removing $stageDir");
system "rm -rf $stageDir" ||
    die("rm -rf $stageDir failed: code ". ($? >> 8));

$MozParser::missingFiles = 2 if $ignoreMissing;
$MozParser::missingFiles = 1 if $warnMissing;

MozPackages::parsePackageList($packageList);

my @packages;
foreach my $package (@ARGV) {
    push @packages, MozPackages::getPackagesFor($package);
    $xptMergeFile = "dist/bin/components/$package.xpt" if !$xptMergeFile;
}

my $parser = new MozParser;
my $xptMerge = 0;
my $preprocess = 0;

HANDLER: foreach my $handler (@handlers) {
    if ($handler eq "xptmerge") {
        MozParser::XPTMerge::add($parser);
        $xptMerge = 1;
        next HANDLER;
    }
    if ($handler eq "xptdist") {
        MozParser::XPTDist::add($parser);
        next HANDLER;
    }
    if ($handler eq "touch") {
        my $dummyFile = File::Spec->catfile("dist", "dummy-file");
        system ("touch", $dummyFile);
        MozParser::Touch::add($parser, $dummyFile);
        next HANDLER;
    }
    if ($handler eq "preprocess") {
        die("--preprocessor not specified on command line.") if (!$preprocessor);
        MozParser::Preprocess::add($parser);
        $preprocess = 1;
        next HANDLER;
    }
    if ($handler eq "optional") {
        MozParser::Optional::add($parser);
        next HANDLER;
    }
    die("Unrecognized command-handler $handler");
}

foreach my $mapping (keys %mappings) {
    $parser->addMapping($mapping, $mappings{$mapping});
}

my $files = $parser->parse($packagesDir, @packages);

MozStage::stage($files, $stageDir);

if ($xptMerge) {
    $xptMergeFile = File::Spec->catfile($stageDir, split('/', $parser->findMapping($xptMergeFile)));
    MozParser::XPTMerge::mergeTo($parser, $xptMergeFile);
}

if ($calcDiskSpace) {
    MozStage::Utils::calcDiskSpace($stageDir)
}

if ($preprocess) {
    MozParser::Preprocess::preprocessTo($parser, $preprocessor, $stageDir);
}

if ($xpiResult) {
    MozStage::makeXPI($stageDir, $xpiResult);
}
