use File::Spec;
use Getopt::Long;
use MozPackager;

Getopt::Long::Configure ("bundling");

my $objdir = "";
my $packagesDir = "";
my $warnMissing = 0;
my $ignoreMissing = 0;

# xxxbsmedberg: need usage()

GetOptions("objdir|o=s"         => \$objdir,
           "package-list|l=s"   => \$packageList,
           "packages-dir|p=s"   => \$packagesDir,
           "warn-missing|w"     => \$warnMissing,
           "ignore-missing|i"   => \$ignoreMissing,
           "verbose|v+"         => \$MozPackager::verbosity);

$packageList || die("Specify --package-list=");

$packageList = File::Spec->rel2abs($packageList);

if ($packagesDir) {
    $packagesDir = File::Spec->rel2abs($packagesDir);
} else {
    $packagesDir = File::Spec->catdir("dist", "packages")
}    

chdir $objdir if $objdir;
-d "dist" || die("directory dist/ not found... perhaps you forgot to specify --objdir?");

$MozParser::missingFiles = 2 if $ignoreMissing;
$MozParser::missingFiles = 1 if $warnMissing;

MozPackages::parsePackageList($packageList);

$\ = "\n";

foreach my $package (keys %MozPackages::packages) {
    print "Package: $package";
    print "Dependencies:";
    foreach my $dependency (@{$MozPackages::packages{$package}}) {
        print $dependency;
    }
    print "";
    my $parser = new MozParser;
    MozParser::XPTDist::add($parser);
    MozParser::Touch::add($parser, File::Spec->catfile("dist", "dummy.file"));
    $parser->addMapping("dist/bin", "bin");
    $parser->addMapping("dist/lib", "lib");
    $parser->addMapping("dist/include", "include");
    $parser->addMapping("dist/idl", "idl");
    $parser->addMapping("xpiroot", "xpiroot");
    my $files = $parser->parse($packagesDir, $package);

    print "Files:";
    foreach my $result (keys %$files) {
        print "$files->{$result}\t$result";
    }
    print "";
}
