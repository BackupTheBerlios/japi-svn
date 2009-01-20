#!perl

# stripped down version. Only table we need is the character break class table
# and so we dumped the previous, extended tables.

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
		'cbc'	=> 'kCBC_Other'
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
	if ($line =~ $re) {
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

# now build a table

my $table = new Table();

foreach my $pageNr (0 .. $kUC_PAGE_COUNT - 1)
{
	my $page = "\t\t{\n";
	
	foreach my $uc ($pageNr * 256 .. ($pageNr + 1) * 256 - 1) {
		$page .= sprintf("\t\t\t %s,\n", $data[$uc]->{'cbc'});
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

typedef CharBreakClass	MUnicodeInfoPage[256];

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

