#!/usr/bin/perl -T -w
# This is used for moving from human readable life format ot a code format.
#
# Run like life2code.pl sq < piston.life
#sq
#piston.life
##P -10 -3  Treated as a comment, program finds own center
#..........*...........
#..........****........
#**.........****.......
#**.........*..*.....**
#...........****.....**
#..........****........
#..........*...........
#
# Run like life2code.pl hx < glider.hlife
#glider.hlife
##P -2 -2   Treated as a comment, program finds own center
#       . . O .
#      . . . O
#     O . . O
#    . . . O
#   . . . O

local($pl) = shift(@ARGV);
if (!defined $pl || ("$pl" ne "tr" && "$pl" ne "sq" && "$pl" ne "pn" && "$pl" ne "hx")) {
  print "\nUsage: $0 [tr | sq | pn | hx]\n";
  print "\ttr: triangle format (same as sq)\n";
  print "\tsq: square format\n";
  print "\tpn: pentagon (cairo tiling) format (same as sq)\n";
  print "\thx: hexagon format\n";
  exit 1;
}

local($PTS, $X, $Y);

#local($offset) = shift(@ARGV);
#if ($offset eq "") {
  $offset = 0;
#}

print "
Drop these points in Life2DForms.java array and
make up a name for name array\n\n";
#Drop these points in life.c, within the 'patterns' array.
#Note if the number of points > 64, one must increase points NUMPTS;
#also to fit most screens and especially the iconified window,
#one should have the size < 32x32.\n\n";
&search;
print "\npoints = $PTS; size = ${X}x$Y\n";

sub search {
  local ($col, $row);
  local ($firstcol, $firstrow);
  local ($i, $j);
  local ($tempx, $tempy);
  local ($found, $c);
  local (@array);

  $col = $row = 0;
  $firstcol = 80;
  $firstrow = -1;
  $PTS = $X = $Y = 0;
  while (<>) {
    if (!($_ =~ /^#/)) {
      @chars = split(//);
      $col = 0;
      $row++;
      foreach $c (@chars) {
        $col++;
        if ($c =~ /[\*0OoAVJD]/) {
          if ($col < $firstcol) {
            $firstcol = $col;
          }
          if ($firstrow < 0) {
            $row = $firstrow = 1;
          }
          if ($col > $X) {
            $X = $col;
          }
          if ($row > $Y) {
            $Y = $row;
          }
          $array{$col, $row} = 1;
          $PTS++;
          if ("$pl" eq "hx") {
            $col++;
          }
          } if ("$pl" eq "hx" && $c =~ /[ ]/) {
            $col--;
//        } if ($c =~ /[\.]/) {
        }
      }
    }
  }
  $col = $X - $firstcol + 1;
  $row = $Y;
  print "\t{\n\t\t";
  for ($j = 0; $j <= $Y; $j++) {
    $found = 0;
    for ($i = 0; $i <= $X; $i++) {
      if ($array{$i, $j}) {
        if ("$pl" ne "hx" && $found) {
          printf " ";
        }
        $found = 1;
        if ("$pl" eq "hx") {
          $tempx = $i - int(($col + 2) / 2) - $firstcol + 1 + $offset;
        } else {
          $tempx = $i - int(($col + 2) / 2) - $firstcol + 1;
        }
        $tempy = $j - int(($row + 2) / 2);
        printf "$tempx, $tempy,";
        if ("$pl" eq "hx") {
          printf " ";
        }
      }
    }
    if ($found) {
      if ("$pl" eq "hx") {
        print "\n\t\t";
      } else {
        print "\n";
        if ($j != $Y) {
          print "\t\t";
        }
      }
    }
  }
#  print "127\n\t},\n";
  print "\t},\n";
  $X = $col;
}
