#!/usr/bin/perl 

use Win32;
use strict;
use warnings;

my $res = shift;
my $dir = shift;

my @list = &read_rsrc_list($dir, "");

my $out;
open($out, ">$res") or die "Could not open output file $res\n";

my $n = 1;
foreach my $r (@list)
{
	$r =~ s|/|\\|g;
	
	print $out "$n\tMYRSRC\n{\n";

	my $fh;
	open($fh, "<$r") or die "Could not open resource file $r\n";
	while (my $line = <$fh>)
	{
		chomp($line);
		$line =~ s|"|\\"|g;
		
		print $out "\"$line\",\n";
	}
	
	print $out "}\n";
	
	++$n;
}

close($out);


sub read_rsrc_list()
{
	my ($dir, $root) = @_;
	my @result;
	
	my $dh;
	
	opendir($dh, $dir) or die "Could not open dir $dir\n";
	foreach my $e (readdir($dh))
	{
		next if ($e eq '.' or $e eq '..' or $e eq '.svn');
		
		my $p = "$dir/$e";
		
		push @result, $p if (-f $p);
		push @result, &read_rsrc_list($p, "$root\\$e") if (-d $p);
	}
	closedir($dh);
	
	return @result;
}
