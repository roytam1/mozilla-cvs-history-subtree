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

# xxxbsmedberg: need usage()

# package names are specified after all options
# names in the form ^package-name are removed from the stage

GetOptions("objdir|o=s"           => \$objdir,
           "package-list|l=s"     => \$packageList,
           "packages-dir|p=s"     => \$packagesDir,
           "warn-missing|w"       => \$warnMissing,
           "ignore-missing|i"     => \$ignoreMissing,
           "verbose|v+"           => \$MozPackager::verbosity,
           "command-handlers|c=s" => \@handlers,
           "xpt-merge-file|x=s"   => \$xptMergeFile,
           "stage-directory|s=s"  => \$stageDir,
           "mapping|m=s"          => \%mappingsm,
           "force-copy|f"         => \$MozStage::forceCopy);

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

chdir($objdir) if $objdir;
-d "dist" || die("directory dist/ not found... perhaps you forgot to specify --objdir?");

MozPackager::_verbosePrint(1, "removing $stageDir");
system "rm -rf $stageDir" ||
    die("rm -rf $stageDir failed: code ". ($? >> 8));

$MozParser::missingFiles = 2 if $ignoreMissing;
$MozParser::missingFiles = 1 if $warnMissing;

my @packages;
my @unpackages;

MozPackages::parsePackageList($packageList);

foreach my $package (@ARGV) {
    if ($package =~ s/^\^//) {
        push @unpackages, MozPackages::getPackagesFor($package);
        next;
    }

    push @packages, MozPackages::getPackagesFor($package);
    $xptMergeFile = "dist/bin/components/$package.xpt" if !$xptMergeFile;
}

my $parser = new MozParser;
my $unparser = new MozParser;

my $xptMerge = 0;

HANDLER: foreach my $handler (@handlers) {
    if ($handler eq "xptmerge") {
        MozParser::XPTMerge::add($parser);
        MozParser::XPTMerge::add($unparser);
        $xptMerge = 1;
        next HANDLER;
    }
    if ($handler eq "xptdist") {
        MozParser::XPTDist::add($parser);
        MozParser::XPTDist::add($unparser);
        next HANDLER;
    }
    if ($handler eq "touch") {
        my $dummyFile = File::Spec->catfile("dist", "dummy-file");
        system ("touch", $dummyFile);
        MozParser::Touch::add($parser, $dummyFile);
        MozParser::Touch::add($unparser, $dummyFile);
        next HANDLER;
    }
    die("Unrecognized command-handler $handler");
}

foreach my $mapping (keys %mappings) {
    $parser->addMapping($mapping, $mappings{$mapping});
    $unparser->addMapping($mapping, $mappings{$mapping});
}

my $files = $parser->parse($packagesDir, @packages);
my $unfiles = $unparser->parse($packagesDir, @unpackages);

foreach $distfile (keys %$files) {
    if (exists $unfiles->{$distfile}) {
        if ($files->{$distfile} ne $unfiles->{$distfile}) {
            warn ("Dist/undist mismatch for file $distfile.\nDist = $files->{$distfile}\nUndist = $unfiles->{$distfile}");
        }
        delete $files->{$distfile};
    }
}

MozStage::stage($files, $stageDir);

if ($xptMerge) {
    MozParser::XPTMerge::removeFiles($parser, $unparser);

    $xptMergeFile = File::Spec->catfile($stageDir, split('/', $parser->findMapping($xptMergeFile)));
    MozParser::XPTMerge::mergeTo($parser, $xptMergeFile);
}
