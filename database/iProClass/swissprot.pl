#!/bin/perl
#swissprot.pl < input > output
#Input: protein db as provided by SWISSPROT
#
my $string = "";
my $indicator = "";
$sq = 0;
$ac = 0;

while(<>)
{
    #chop;
    if ( /^\/\// ) {
      print "\n";
      $sq = 0;
      $ac = 0;
      next;
    }
    if ($sq == 1) {
        @words = split;
        foreach $word (@words) {
          print "$word";
        }
        next;
    }
    if( /^AC(\s+)(\w+);/ ) {
      if ($ac == 0) {
        $indicator = $2;
        print "$indicator|";
        $sq = 0;
        $dt = 0;
        $ac = 1;
        next;
      }
    }
    if ( /^OS(\s+)(.*)\./ ) {
        $organism = $2;
        print "$organism|";
        next;
    }
    if ( /^DT(\s+)(\S+)/ ) {
        if ($dt == 0) {
           print "$2|";
           $dt = 1;
        }
    }
    if ( /^SQ(\s+)/ ) {
        $sq = "1";
        next;
    }
}
