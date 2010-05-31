#!/usr/bin/perl 

use Win32;
use strict;
use warnings;
use File::Copy;
use Getopt::Long;

my $rcDir = "msvc/japi-vc/Rsrc/";
my $rcFile = "MJapi.rc";
my $rcRsrc = "Resources";

my $result = GetOptions(
	"rc-dir=s"	=> \$rcDir,
	"rc-file=s"	=> \$rcFile,
	"rc-rsrc=s"	=> \$rcRsrc);

die "No such directory ($rcDir)\n" unless -d $rcDir;
die "No such directory ($rcRsrc)\n" unless -d $rcRsrc;

my @list = &read_rsrc_list($rcRsrc, "");

my ($rc, $ix, $ixtext);
open($rc, ">$rcDir/$rcFile") or die "Could not open rc file: $!\n";
open($ix, ">", \$ixtext) or die "Could not open ix: $!\n";

print $rc "1\tMRSRCIX\tix.xml\n";
print $ix "<rsrc-ix>\n";

my $n = 1;
foreach my $r (@list)
{
	$r =~ s|^/||;

	copy("$rcRsrc/$r", "$rcDir/$n.xml") or die "Copy $rcRsrc/$r to $rcDir/$n.xml failed: $!\n";
	print $rc "$n\tMRSRC\t$n.xml\n";
	print $ix "  <rsrc name='$r' nr='$n'/>\n";
	++$n;
}

print $ix "</rsrc-ix>";

close($rc);
close($ix);

open($ix, ">$rcDir/ix.xml") or die "Could not open index file: $!\n";
print $ix $ixtext;
close($ix);


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
		
		push @result, "$root/$e" if (-f $p);
		push @result, &read_rsrc_list($p, "$root/$e") if (-d $p);
	}
	closedir($dh);
	
	return @result;
}
