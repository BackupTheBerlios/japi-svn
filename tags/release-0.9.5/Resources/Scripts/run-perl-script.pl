#!/usr/bin/perl
#
# Read in the current Perl script

undef($/);
my $script = <STDIN>;
$/ = '\n';

# re-open STDOUT, so output is now redirected to a new japi window
open(STDOUT, "|japi -")
	or die "Could not open a new japi window";

# evaluate the script.
eval "$script";

# and print out any errors that might have occurred
if ($@) { print $@; }
