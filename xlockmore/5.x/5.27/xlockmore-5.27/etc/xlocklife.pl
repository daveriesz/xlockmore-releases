#!/usr/bin/perl -T -w
# This is used  to add life to xlock's life.h
# This is a QUICK hack to convert life files to xlock's life format.
# Patterns MUST have <= 64 pts at start for life.h to use the data generated
# Below is an example of a life file without the first initial #'s
# Call the file piston.life and run it like xlocklife.pl < piston.life
#piston.life
##P -10 -3  Treated as a comment, program finds own center
#..........*...........
#..........****........
#**.........****.......
#**.........*..*.....**
#...........****.....**
#..........****........
#..........*...........

local($PTS, $X, $Y);

print "
Drop these points in life.c, within the 'patterns' array.
Note if the number of points > 64, one must increase points NUMPTS;
also to fit most screens and especially the iconified window,
one should have the size < 32x32.\n\n";
&search;
print "\npoints = $PTS; size = ${X}x$Y\n";

sub search {
    local ($col, $row, $firstcol, $firstrow);
    local ($i, $j, $found, $c, $tempx, $tempy);
    local ($dim);
    local (@array);

    $col = $row = 0;
    $firstcol = 80;
    $firstrow = -1;
    $PTS = $X = $Y = 0;
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
		    if ($col > $X) {
			$X = $col;
		    }
		    if ($row > $Y) {
			$Y = $row;
		    }
		    $array{$col, $row} = 1;
		    $PTS++;
		}
	    }
	    $row++;
	}
    }
    $col = $X - $firstcol + 1;
    $row = $Y;
    print "\t{\n\t\t";
    for ($j = 0; $j <= $Y; $j++) {
	$found = 0;
	for ($i = 0; $i <= $X; $i++) {
	    if ($array{$i, $j}) {
		if ($found) {
		   printf " ";
		}
		$found = 1;
		$tempx = $i - int(($col + 2) / 2) - $firstcol + 1;
		$tempy = $j - int(($row + 2) / 2);
		printf "$tempx, $tempy,";
	    }
	}
	if ($found) {
	    print "\n";
	    if ($j != $Y) {
		print "\t\t";
	    }
	}
  }
  print "127\n\t},\n";
  $X = $col;
}
