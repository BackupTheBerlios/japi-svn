#!perl

use strict;
use warnings;

my (%codes, %data);

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
			my $key = sprintf("%4.4x", $uc);
			$data{$key} = $value;
		}
	}
}
close IN;

print "const uint8 kCharBreakClass[983039] = {\n";
for (my $uc = 0; $uc < 983039; ++$uc) {
	my $value = $data{sprintf("%4.4x", $uc)};
	$value = "kCBC_Other" unless defined $value;
	
	print "\t$value, /* $uc */\n";
}
print "};\n\n";

# Character Property Table

undef(%data);

sub add
{
	my ($key, $value) = @_;
	
	$key = sprintf("%5.5x", $key);
	
	if (defined $data{$key}) {
		$data{$key} .= " | $value";
	}
	else {
		$data{$key} = $value;
	}
}

open IN, "<CharClass.csv" or die "Could not open CharClass\n";
while (my $line = <IN>)
{
	if ($line =~ m/(.+);(.)(.)/)
	{
		my $uc = hex($1);
		my $c1 = $2;
		my $c2 = $3;
		
		my $v;
		
		if ($c1 eq 'L') {
			if ($c2 eq 'o') {
				$v = 'kOTHER';
			}
			else {
				$v = 'kLETTER';
			}
		}
		elsif ($c1 eq 'N') {
			$v = 'kNUMBER';
		}
		elsif ($c1 eq 'M') {
			$v = 'kCOMBININGMARK';
		}
		elsif ($c1 eq 'P') {
			$v = 'kPUNCTUATION';
		}
		elsif ($c1 eq 'S') {
			$v = 'kSYMBOL';
		}
		elsif ($c1 eq 'Z') {
			$v = 'kSEPARATOR';
		}
		elsif ($c1 eq 'C') {
			$v = 'kCONTROL';
		}
		
		if (defined $v) {
			add($uc, $v);
		}
	}
}
close IN;

for (my $uc = hex('0x3400'); $uc <= hex('0x4DB5'); ++$uc) {
	add($uc, 'kLETTER');
}

for (my $uc = hex('0x4E00'); $uc <= hex('0x09FA5'); ++$uc) {
	add($uc, 'kLETTER');
}

for (my $uc = hex('0x0Ac00'); $uc <= hex('0x0D7A3'); ++$uc) {
	add($uc, 'kLETTER');
}

for (my $uc = hex('0x20000'); $uc <= hex('0x2A6D6'); ++$uc) {
	add($uc, 'kLETTER');
}

add(95, 'kLETTER');

my $defs=<<EOF;

enum {
	kLETTER			= 1,
	kNUMBER			= (kLETTER << 1),
	kCOMBININGMARK	= (kNUMBER << 1),
	kPUNCTUATION	= (kCOMBININGMARK << 1),
	kSYMBOL			= (kPUNCTUATION << 1),
	kSEPARATOR		= (kSYMBOL << 1),
	kCONTROL		= (kSEPARATOR << 1),
	kOTHER			= (kCONTROL << 1)
};

EOF

print $defs;

print "const uint8 kCharClass[983039] = {\n";
for (my $uc = 0; $uc < 983039; ++$uc) {
	my $value = $data{sprintf("%5.5x", $uc)};
	$value = 'kOTHER' unless defined $value;
	
	printf "\t$value, /* $uc */\n";
}
print "};\n\n";
