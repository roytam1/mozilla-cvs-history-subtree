########################################################
# symbols.pl 
#    generate a Module Definition file at build time
#    to export all symbols in Dogbert so that security
#    plug-in (a stand-alone DLL) can call any function 
#    in Dogbert.
# Created by hoi on 5/26/1998 
# 
########################################################

@objfiles = ();
@symbols = ();
%unique_symbols = {};
# Functions with the following prefixes will not be exported.
# SEC_GetDispatchTable & SEC_NumDispatchPoints are exported
# using linker facilities.
@unwanted_prefixes = ("__","Java_","java_","awt_","sun_","PR_", "LL_",
                      "SEC_GetDispatchTable", "SEC_NumDispatchPoints");

#Step 0: Get command line options
$input_file = $ARGV[0];
$header_file = "dispt.h";
$source_file = "dispt.c";
UsageAndExit() if (!$input_file);

# Step 1: Open dispt.h and dispt.c for output
open (DISPATCH_H, ">$header_file") or die("Can't open $header_file.\n");
open (DISPATCH_C, ">$source_file") or die("Can't open $source_file.\n");

# Step 3: Read link.cl to determine list of all obj / lib files
# needed to link the executable
GetObjFileList($input_file, \@objfiles);

# Step 4: Run DUMPBIN on every single obj / lib file
# to get the complelete list of global functions
CallDumpBin(\@objfiles, \@symbols, \%unique_symbols);

# Step 5: Write out the static part of dispt.h 
print DISPATCH_H ("#include \"nspr.h\"\n");
print DISPATCH_H ("\n");
print DISPATCH_H ("typedef void (*void_fun)(void);\n");
print DISPATCH_H ("\n");
print DISPATCH_H ("PR_EXTERN_DATA (void_fun) SEC_Dispatch_Table[];\n");
print DISPATCH_H ("\n");
print DISPATCH_H ("PR_EXTERN (void_fun*)  SEC_GetDispatchTable(void);\n");
print DISPATCH_H ("\n");
print DISPATCH_H ("PR_EXTERN (int) SEC_NumDispatchPoints(void);\n");
print DISPATCH_H ("\n");

# Step 6: Put all the symbols in dispt.h
foreach (sort(@symbols)) {
	print DISPATCH_H ("PR_EXTERN (void) $_(void);\n");
}
print DISPATCH_H ("\n");

# Step 7: Write out dispt.c
print DISPATCH_C ("#include \"dispt.h\"\n");
print DISPATCH_C ("\n");
print DISPATCH_C ("PR_IMPLEMENT_DATA (void_fun) SEC_Dispatch_Table[] = {\n");

$first_entry = 1;
foreach (sort(@symbols)) {
        if (!$first_entry) {
             print DISPATCH_C (",\n");
        }
	print DISPATCH_C ("\t$_");
        $first_entry = 0;
}

print DISPATCH_C ("\n};\n");
print DISPATCH_C ("\n");
print DISPATCH_C ("PR_IMPLEMENT (void_fun*)  SEC_GetDispatchTable(void) {\n");
print DISPATCH_C ("    return SEC_Dispatch_Table;\n");
print DISPATCH_C ("}\n");
print DISPATCH_C ("\n");
print DISPATCH_C ("PR_IMPLEMENT (int) SEC_NumDispatchPoints(void) {\n");
print DISPATCH_C ("    return (sizeof(SEC_Dispatch_Table)/sizeof(SEC_Dispatch_Table[0]));\n");
print DISPATCH_C ("}\n");

# ---------------  Sub-Routines -----------------
# FilterUnwantedPrefix() returns 1st argument
# if its prefix doesn't match any of those 
# defined in @unwanted_prefixes

sub FilterUnwantedPrefix {
        foreach $prefix (@unwanted_prefixes) {
                if ($_[0] =~ m/^$prefix/) {
                        return "";
                }
        }
        return $_[0];
}


#
# CallDumpBin calls dumpbin on each file in objfiles
# and finds out all symbols defined in them. 
# The list of symbols is returned.
sub CallDumpBin {
	my($objfiles) = $_[0];
	my($symbols) = $_[1];
	my($unique) = $_[2];
	my($num_symbols) = 0;
	my($num_unique) = 0;
	foreach (@{$objfiles}) {
		my($file) = $_;
		my(@results) = ();
		# if such file exists, call dumpbin
		# otherwise, print a warning
		if (-f $file) {
			@results = `dumpbin /SYMBOLS $file`;
			foreach (@results) {
				my($symbol) = ExtractFunctionSymbol($_);
				$symbol = FilterUnwantedPrefix($symbol);
				if ($symbol ne "") {
					push(@{$symbols}, ($symbol)); 
					if (!exists(${$unique}{$symbol})) {
						${$unique}{$symbol} = $symbol;
						$num_unique++;
					}
					$num_symbols++;
				}
			}	
		}
	}
}

# Returns a non-empty string if the line contains a function symbol
# Returns an empty string otherwise
# Note: a line containing a function symbol looks like this:
#       048 00000000 SECTA  notype ()    External     | _lm_DefinePkcs11
#       This function searches for entries that contains SECT,(),External
sub ExtractFunctionSymbol {
         if ($_[0] =~ m/SECT.+\(\)\s+External\s+\|\s*_(\w+)\s*$/) {
               	return $1;
        } else {
                return "";
        }
}

# GetObjFileLIst reads in a file containing LINK options
# and returns the list of .obj & .lib files in the options
sub GetObjFileList {
	my($objfiles) = $_[1];
	open (OBJS_LIST, $_[0]) or die ("Can't open $_[0].\n");
	while ($line = <OBJS_LIST>) {
		# For each line, break up into words
		$line =~ s/^\s+//;
		@words = split(/\s+/, $line);
		foreach (@words) {
			if (IsObjFile($_)) {
				push(@{$objfiles}, ($_));
			}
		}
	}
}

sub IsObjFile {
	return ($_[0] =~ m/\.(obj)|\.(lib)\s*$/i);
}

sub UsageAndExit {
	print ("Usage: perl gendisp.pl <path_to_link.cl>");
	exit(-1);
}
