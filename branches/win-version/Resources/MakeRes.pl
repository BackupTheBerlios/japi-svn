#!/usr/bin/perl 

use strict;
use warnings;

my $res = shift;

print "Hello, world!\n";

my $out;
open($out, ">$res") or die "Could not open output file $res\n";
print $out "Hallo!\n";
close($out);
