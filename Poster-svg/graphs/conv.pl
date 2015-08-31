#!/usr/bin/perl -T -W
print STDERR "\n\nDon't forget to use CLONE-->UNLINK CLONE on the data points\n\n";
while ( <STDIN> ) {
  my $line = $_;
  $line =~ s/(color:)(.*)(;\ * stroke:)currentColor/$1$2$3$2/;
  print $line;
}
