#!/usr/bin/perl -w
# This is used for moving from human readable life format ot a code format.
#
# Run like life3d2code.pl cb < glider.3dlife
#cb
#glider.3dlife
##P -2 -2 -1  Treated as a comment, program finds own center
#.**.
#*..*
#*..*
#
#....
#.**.
#.**.
#
# Run like life3d2code.pl dd < glider.3dlife
#dd
#glider.3dlife
##P 10 -1 0  Treated as a comment, program finds own center
#. .
#. .
#. *
#
# . .
# * .
# * *
#
#. .
#* .
#* .
#
# * *
# . .
# . .
#
#* *
#* .
#. .
#
# * .
# . .
# . .

local($pl) = shift(@ARGV);
if (!defined $pl || ("$pl" ne "cb" && "$pl" ne "dd" && "$pl" ne "td")) {
  print "\nUsage: $0 [cb | dd | td]\n";
  print "\tcb: cube format\n";
  print "\tdd: rhombic dodecahedron format\n";
  print "\ttd: truncated octahedron (tetradecahedron) format\n";
  exit 1;
}

local($PTS, $X, $Y, $Z);

#local($offset) = shift(@ARGV);
#if ($offset eq "") {
  $offset = 0;
#}

print "
Drop these points in Life3DForms.java array and
make up a name for name array\n\n";
#Drop these points in life.c, within the 'patterns' array.
#Note if the number of points > 48, one must increase points NUMPTS;
#also to fit most screens and especially the iconified window,
#one should have the size < 32x32.\n\n";
&search;
print "\npoints = $PTS; size = ${X}x${Y}x${Z}\n";

sub search {
 local ($col, $row, $stack);
 local ($firstcol, $firstrow, $firststack);
 local ($rowO, $stackO);
 local ($i, $j, $k);
 local ($tempx, $tempy, $tempz);
 local ($found, $found1, $c, $nlc);
 local (@array);

  $col = $row = $stack = 0;
  $firstcol = 80;
  $firstrow = -1;
  $firststack = -1;
  $rowO = 0;
  $stackO = 0;
  $PTS = $X = $Y = $Z = 0;
  $nlc = '0'; # carriage return or line feed, whichever is first
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
            $firstrow = $row;
          }
          if ($firststack < 0) {
            $firststack = $stack;
          }
          if ($col > $X) {
            $X = $col;
          }
          if ($row > $Y) {
            $Y = $row;
          }
          if ($stack > $Z) {
            $Z = $stack;
          }
          $array{$col, $row, $stack} = 1;
          $PTS++;
        } elsif ("$pl" eq "td" && $c =~ /[ ]/) {
          $col--;
#        } elsif ($c =~ /[\.]/) {
        } elsif ($c =~ /[\r\n]/) {
          if ($nlc eq '0' || $nlc eq $c) {
            $nlc = $c;
            if ($col == 1) {
              $stack++;
              $row = 0;
            }
          }
        }
      }
    }
  }
  $col = $X - $firstcol + 1;
  $row = $Y + $firstrow;
  $stack = $Z + $firststack;
  if ("$pl" eq "dd") {
    if (((-int(($stack + 2) / 2) + $offset) % 2) == 1) {
      $stackO += 1;
    }
  } elsif ("$pl" eq "td") {
    if (((-int(($stack + 2) / 2) + $offset) % 2) == 1) {
      $stackO += 1;
    }
  }
  print "\t{\n\t\t";
  for ($k = 0; $k <= $Z; $k++) {
    $found1 = 0;
    for ($j = 0; $j <= $Y; $j++) {
      $found = 0;
      for ($i = 0; $i <= $X; $i++) {
        if ($array{$i, $j, $k}) {
          if ($found) {
            printf " ";
          }
          $found = 1;
          $found1 = 1;
          if ("$pl" eq "dd") {
            $tempx = int(($i + 1) / 2) - int(($col + 2) / 4) - $firstcol + 1;
            $rowO = 1;
          } else {
            $tempx = $i - int(($col + 2) / 2) - $firstcol + 1;
          }
          $tempy = $j - int(($row + 2) / 2) + $rowO;
          $tempz = $k - int(($stack + 2) / 2) + $offset + $stackO;
          printf "$tempx, $tempy, $tempz,";
        }
      }
      if ($found) {
        print "\n";
        if ($j != $Y) {
          print "\t\t";
        }
        if ("$pl" eq "dd" && $found && ($k % 2) != 0) {
          printf " ";
        } elsif ("$pl" eq "td" && $found && ($k % 2) != 0) {
          printf " ";
        }
      }
    }
    if ($k != $Z) {
      if ($found1) {
        print "\n\t\t";
      }
      if ("$pl" eq "dd" && $found1 && ($k % 2) == 0) {
        printf " ";
      } elsif ("$pl" eq "td" && $found && ($k % 2) == 0) {
        printf " ";
      }
    }
  }
#  print "127\n\t},\n";
  print "\t},\n";
  $X = $col;
  $Z += 1;
}
