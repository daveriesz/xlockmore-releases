#!/usr/bin/perl
# This is used  to add flags to flag
# What you need to run this with a banner that accepts the width option
# and tilts its output 90 degrees i.e. not the SysV banner
# banner outputs ' ' and '#' and newlines to form a letter
# tilted 90 clockwise.
# banner is run with -w 25 and then feed this into flag.pl
# This is what I used to create the flags, it could
# be easier, etc. but its good enough for me.
# An example of its use (current directory is xlockmore/flags) is:
# banner -w 25 UNIX | ../config/xlockflag.pl > flag-unix.h

# print "";
&printflag;

sub printflag {
    local ($mostcol, $row, $col);
    local ($i, $j, $found, $c, $tempx, $tempy);
    local (@array);
  

    $row = $col = 0;
    $smallest_row = $smallest_col = 32767; 
    $largest_row = $largest_col = 0; 
    while (<>) {
         if ($_ =~ /^[#\s]/)
         {
            @chars = split(//);
            $col = 0;
            foreach $c (@chars) {
                if ($c =~ /[#\s]/) {
                    $col++;
                    if ($col > $mostcol) {
                        $mostcol = $col;
                    }
                }
                if ($c =~ /[#]/) {
                  $array{$col, $row} = 1;
                  if ($smallest_row > $row) {
                      $smallest_row = $row;
                  }
                  if ($smallest_col > $col) {
                      $smallest_col = $col;
                  }
                  if ($largest_row < $row) {
                      $largest_row = $row;
                  }
                  if ($largest_col < $col) {
                      $largest_col = $col;
                  }
                }
            }
            $row++;
        }
    }
  $col = $mostcol;
# remember the letter is tilted 90 degrees
    $w = $largest_row - $smallest_row + 1;
    $h = $largest_col - $smallest_col + 1;
    print "#define flag_width $w\n";
    print "#define flag_height $h\n";
    print "static unsigned char flag_bits[] = {\n";
    for ($j = $smallest_row; $j <= $largest_row; $j++) {
        for ($i = $smallest_col; $i <= $largest_col; $i++) {
            if ($array{$i, $j}) {
                printf " 1";
            } else {
                printf " 0";
            }
            if ($i < $largest_col) {
                printf ",";
            }
        }
        if ($j < $largest_row) {
          printf ",\n";
       }
  }
  printf "\n};\n";
}
