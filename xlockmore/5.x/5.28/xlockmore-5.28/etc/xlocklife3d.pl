#!/usr/bin/perl -T -w
# This is used  to add 3dlife to xlock's life3d.h
# This is a QUICK hack to convert life files to xlock's life3d format.
# Patterns MUST have <= 48 pts at start for life.h to use the data generated
# Below is an example of a life file without the first initial #'s
# Call the file piston.life and run it like xlocklife.pl < piston.life
#glider.3dlife
##P -2 -2 -1  Treated as a comment, program finds own center
#.**.
#*..*
#*..*
#
#....
#.**.
#.**.

local($PTS, $X, $Y, $Z);

print "
Drop these points in life.c, within the 'patterns' array.
Note if the number of points > 48, one must increase points NUMPTS;
also to fit most screens and especially the iconified window,
one should have the size < 32x32.\n\n";
&search;
print "\npoints = $PTS; size = ${X}x${Y}x${Z}\n";

sub search {
    local ($col, $row, $stack, $firstcol, $firstrow, $firststack);
    local ($i, $j, $k, $found, $c, $tempx, $tempy, $tempz);
    local (@array);

    $col = $row = $stack = 0;
    $stack = 1;
    $firstcol = 80;
    $firstrow = -1;
    $firststack = -1;
    $PTS = $X = $Y = $Z = 0;
    while (<>) {
	 if (!($_ =~ /^#/)) {
	    @chars = split(//);
	    $col = 0;
	    foreach $c (@chars) {
		$col++;
		if ($c =~ /[\*0Oo]/) {
		    if ($col < $firstcol) {
			$firstcol = $col;
		    }
		    if ($firstrow < 0) {
			$row = $firstrow = 1;
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
		} elsif ($c =~ /[\.]/) {
		    if ($firstrow < 0) {
			$row = $firstrow = 1;
		    }
		    if ($row > $Y) {
			$Y = $row;
		    }
		} elsif ($c =~ /[\n]/) {
		    if ($col == 1) {
			$stack++;
			$row = 0;
		    }
		}
	    }
	    $row++;
	}
    }
    $col = $X - $firstcol + 1;
    $row = $Y;
    $stack = $Z;
    print "\t{";
    for ($k = 0; $k <= $Z; $k++) {
	for ($j = 0; $j <= $Y; $j++) {
	    $found = 0;
	    for ($i = 0; $i <= $X; $i++) {
		if ($array{$i, $j, $k}) {
	  	    if ($found) {
			printf " ";
		    }
		    $found = 1;
		    $tempx = $i - int(($col + 2) / 2) - $firstcol + 1;
		    $tempy = $j - int(($row + 2) / 2);
		    $tempz = $k - int(($stack + 2) / 2) - 1;
		    printf "$tempx, $tempy, $tempz,";
	       }
	    }
	    if ($found) {
		print "\n";
		if ($j != $Y) {
			print "\t\t";
		}
	    }
	}
	if ($k != $Z) {
		print "\n\t\t";
	}
  }
  print "\t\t127\n\t},\n";
  $X = $col;
}
