########################################################
# genhook.pl 
#    generate hooks of the form
#    if (xxxxx_hooks.function_xxxx != NULL) {  /* CM_HOOK */
#        return xxxxx_hooks.function_xxxxx( );
#    }
#    This script helps reduces the amount of cut and paste
#    involved in putting Cartman hooks in Communicator.
# 
########################################################

$debug = 0;

# Step 1: Read the list of functions to be hooked
UsageAndExit() if (scalar(@ARGV) != 2);
$input_file = $ARGV[0];
$output_file = $ARGV[1];
open (FUNC_FILE, "$input_file") or die("Can't open $input_file for input.\n");
open (HOOKS_FILE, ">$output_file") or die("Can't open $output_file for output.\n");

$hook_table = "unknown_hooks";

while ($line = <FUNC_FILE>) {

    print ("Processing $line...\n") if $debug;
    if ($line =~ m/^#\s+(.+)/) {

        # Step 2: For each line, if it begins with "# XXXX", 
        #         then set current hooks table name to be XXXXX
        $hook_table = lc($1);
        print ("New hook table matched = $hook_table\n") if $debug;

    } else {
        # Step 3: Otherwise, generate an 'if' statement using
        #    if (xxxxx_hooks.function_xxxx != NULL) {  /* CM_HOOK */
        #        return xxxxx_hooks.function_xxxxx( );
        #    }
        #         as the template.
        chop($line);
        $func_name = lc($line);
        print HOOKS_FILE ("    if ($hook_table.$func_name != NULL) {   /* CM_HOOK */\n");
        print HOOKS_FILE ("        return $hook_table.$func_name( );\n");
        print HOOKS_FILE ("    }\n\n");
    }
}

sub UsageAndExit {
	print ("Usage: perl genhook.pl <list_of_functions> <out_file>");
	exit(-1);
}
