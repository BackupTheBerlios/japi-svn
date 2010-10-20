#!/usr/bin/perl

undef($/);
my $text = <STDIN>;
$/ = '\n';

$text =~ s/<.*?>//sgo;
print $text;
