#!perl

use strict;
use warnings;
use Data::Dumper;

$| = 1;

my $kUC_COUNT = 1114112;
my $kUC_PAGE_COUNT = 4352;

my (%codes, @data);

print STDERR "Initializing array...";

for (my $uc = 0; $uc < $kUC_COUNT; ++$uc) {
	my %ucd = (
		'prop'	=> 'kOTHER',
		'cbc'	=> 'kCBC_Other',
		'upper' => 0,
		'lower'	=> 0
	);
	
	push @data, \%ucd;
}

print STDERR " done\n";

# Character Break Classes

my $re = qr/^([0-9A-F]{4,5})(\.\.([0-9A-F]{4,5}))?\s+;\s+(\S+)/;

my %grapheme_const = (
	'CR'		=> 'kCBC_CR',
	'LF'		=> 'kCBC_LF',
	'Control'	=> 'kCBC_Control',
	'Extend'	=> 'kCBC_Extend',
	'L'			=> 'kCBC_L',
	'T'			=> 'kCBC_T',
	'V'			=> 'kCBC_V',
	'LV'		=> 'kCBC_LV',
	'LVT'		=> 'kCBC_LVT'
);

print STDERR "Reading GraphemeBreakProperty.txt...";

open IN, "<GraphemeBreakProperty.txt" or die "Could not open GraphemeBreakProperty\n";
while (my $line = <IN>)
{
	if ($line =~ $re) { #$
		my $first = hex($1);
		my $last = $first;
		$last = hex($3) if defined $3;
		
		my $value = $grapheme_const{$4};
		die "unknown value: $4\n" unless defined $value;
		
		foreach my $uc ($first .. $last) {
			$data[$uc]->{'cbc'} = $value;
		}
	}
}
close IN;

print STDERR " done\n";

# Character Property Table

print STDERR "Reading UnicodeData.txt ...";

open IN, "<UnicodeData.txt" or die "Could not open UnicodeData.txt\n";
while (my $line = <IN>)
{
	chomp($line);
	my @v = split(m/;/, $line);
	
	next unless scalar(@v) > 3;
	
	my $uc = hex($v[0]);

	my $c1 = substr($v[2], 0, 1);
	my $c2 = substr($v[2], 1, 1);
	
	# character class

	if ($c1 eq 'L') {
		$data[$uc]->{'prop'} = 'kLETTER';
	}
	elsif ($c1 eq 'N') {
		$data[$uc]->{'prop'} = 'kNUMBER';
	}
	elsif ($c1 eq 'M') {
		$data[$uc]->{'prop'} = 'kCOMBININGMARK';
	}
	elsif ($c1 eq 'P') {
		$data[$uc]->{'prop'} = 'kPUNCTUATION';
	}
	elsif ($c1 eq 'S') {
		$data[$uc]->{'prop'} = 'kSYMBOL';
	}
	elsif ($c1 eq 'Z') {
		$data[$uc]->{'prop'} = 'kSEPARATOR';
	}
	elsif ($c1 eq 'C') {
		$data[$uc]->{'prop'} = 'kCONTROL';
	}
	
	# upper case/lower case mapping
	
	$data[$uc]->{'upper'} = hex($v[13]) if defined $v[13];
	$data[$uc]->{'lower'} = hex($v[14]) if defined $v[14];
}
close IN;

print STDERR " done\n";

for (my $uc = hex('0x3400'); $uc <= hex('0x4DB5'); ++$uc) {
	$data[$uc]->{'prop'} = 'kLETTER';
}

for (my $uc = hex('0x4E00'); $uc <= hex('0x09FA5'); ++$uc) {
	$data[$uc]->{'prop'} = 'kLETTER';
}

for (my $uc = hex('0x0Ac00'); $uc <= hex('0x0D7A3'); ++$uc) {
	$data[$uc]->{'prop'} = 'kLETTER';
}

for (my $uc = hex('0x20000'); $uc <= hex('0x2A6D6'); ++$uc) {
	$data[$uc]->{'prop'} = 'kLETTER';
}

# we treat an underscore as a letter
$data[95]->{'prop'} = 'kLETTER';

# now build a table

my $table = new Table();

foreach my $pageNr (0 .. $kUC_PAGE_COUNT - 1)
{
	my $page = "\t\t{\n";
	
	foreach my $uc ($pageNr * 256 .. ($pageNr + 1) * 256 - 1) {
		$page .= sprintf("\t\t\t{ 0x%5.5x, 0x%5.5x, %s, %s },\n",
			$data[$uc]->{'upper'}, $data[$uc]->{'lower'},
			$data[$uc]->{'cbc'}, $data[$uc]->{'prop'});
	}
	
	$page .= "\t\t},\n";
	
	$table->addPage($pageNr, $page);
}

$table->print_out();
exit;

package Table;

sub new
{
	my $invocant = shift;
	my $self = {
		indexCount	=> 0,
		pageIndex	=> [],
		pages		=> [],
		@_
	};
	
	return bless $self, "Table";
}

sub addPage
{
	my ($self, $pageNr, $page) = @_;
	
	my $pageIx = -1;
	
	for (my $pn = 0; $pn < scalar($self->{'pages'}); ++$pn)
	{
		last unless defined @{$self->{'pages'}}[$pn];

		if (@{$self->{'pages'}}[$pn] eq $page)
		{
			$pageIx = $pn;
			last;
		}
	}
	
	if ($pageIx == -1)
	{
		$pageIx = push @{$self->{'pages'}}, $page;
		$pageIx -= 1;
	}
	
	push @{$self->{'pageIndex'}}, $pageIx;
}

sub print_out
{
	my $self = shift;
	
	my $pageIndexSize = scalar(@{$self->{'pageIndex'}});
	my $pageCount = scalar(@{$self->{'pages'}});
	
	print<<EOF;

struct MUnicodeInfoAtom {
	uint32			upper;
	uint32			lower;
	CharBreakClass	cbc;
	uint8			prop;
};

typedef MUnicodeInfoAtom	MUnicodeInfoPage[256];

struct MUnicodeInfo {
	int16				page_index[$pageIndexSize];
	MUnicodeInfoPage	data[$pageCount];
} kUnicodeInfo = {
EOF

	print "\t{";
	
	# the index
	
	for (my $ix = 0; $ix < $pageIndexSize; ++$ix)
	{
		print "\n\t\t" if ($ix % 16) == 0;
		print @{$self->{'pageIndex'}}[$ix], ", ";
	}
	
	print "\n\t},\n\t{\n";

	# the data pages

	foreach my $page (@{$self->{'pages'}}) {
		print $page;
	}

	print "\n\t}\n};\n";
}

