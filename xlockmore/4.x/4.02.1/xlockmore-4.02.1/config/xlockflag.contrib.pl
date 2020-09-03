#!/usr/local/bin/perl

# flag.pl - create a flag.h file for use with xlockmore's flag mode
# author : billy constantine <wdconsta@cs.adelaide.edu.au>
# usage : flag.pl [ message ] [ > ./flag.h ]

# this program requires that Mark Horton's banner-06(6) be in your
# path - it is included in the banners-1.0 and banners-1.1
# distributions, which can be found at most (if not all)
# comp.sources.unix archives

=head1 NAME

B<flag.pl> - create a F<flag.h> file for use with B<xlockmore>'s flag mode

=head1 SYNOPSIS

B<flag.pl> [ I<message> ] [ I<E<gt> ./flag.h> ]

=head1 DESCRIPTION

B<Flag.pl> writes a F<flag.h> file, suitable for use with
B<xlockmore>'s flag mode to standard output. If command line arguments
are given, they are joined to form a space-separated string. If none
are given, then the user is prompted to enter a message for the
flag. If still no message is given, then the login name of the user is
used. In all cases, the message is uppercased to avoid letters with
tails from messing up the flag.

The message is then sent to Mark Horton's B<banner-06> program, which
creates the initial form of the flag. The flag created is then
manipulated and padded to get all lines the same length, and finally
extra details, such as the height and width, are added to the
"bitmap". A comment, containing details of who created the flag and
when, and what the message is, is also added to the top.

=head1 SEE ALSO

xlock(1), banner-06(6)

=head1 AUTHORS

B<xlock>     : Many, many people

B<banner-06> : Mark Horton (address unknown)

B<flag.pl>   : Billy Constantine I<E<lt>wdconsta@cs.adelaide.edu.auE<gt>>

=cut

#'
# to stop font-lock-mode from messing up :)

$banner = "banner-06 -w25 ";
# the command to generate the banner, from which the flag is created

$flag_height = 17;
# this is how high the letters are... if you change the "-w25" bit
# above, you may need to change this too... the flag.h's that come
# with xlock were created with "-w25", so it _might_ mess things up a
# bit if you use a different setting...

$user = (getpwuid($<))[0];
# this is who you are :)

unless(@ARGV) {
    # if no comm-line args...
    print STDERR "What do you want the banner to say? ";
    # prompt to stderr, in case of redirection
    chop($mesg = <STDIN>);
    # get rid of the newline
    $mesg = $user unless $mesg;
    # if no message (ie only a carriage return was given), use the
    # username... $ENV{HOSTTYPE} is also a good alternative
} else {
    # comm-line args were given
    $mesg = join(" ", @ARGV);
    # join them with spaces
}

$\ = "\n";
#  after each print(), do a newline...

$, = "\n";
# ... and also when doing print() on a list - newline after each
# element

@flag = ();
# empty the @flag list - unecessary, but sometimes perl may complain
# about uninitialised data (like when perl -w is used :)

$banner .= uc($mesg);
# uppercase the message, add it to the banner command
open(BANNER, "$banner|");
# create a pipe from the banner command
while(length($_ = <BANNER>)) {
    # for each line of it...
    undef(substr($_, 0, 6));
    # the first 6 characters are spaces, so get rid of them - again,
    # if you change the "-w25" bit, this may need playing with
    s/\s+$//;
    # get rid of trailing whitespace
    $_ .= " " while length($_) < $flag_height;
    # and then append spaces to bring the length up to 17 characters
    tr/ \#/01/;
    # swap spaces for 0's and #'s for 1's... the backslash is to stop
    # font-lock-mode from screwing up
    s/(.)/ $1,/g;
    # replace each put a space before each character, and a comma
    # after
    $_ = " $_";
    # add a space at the start - looks like emacs indenting now :)
    push(@flag, $_);
    # add it to the flag list
}
close(BANNER);
# close the banner command pipe
die "Couldn't run banner\n" unless @flag;
# quit if the flag array is empty - it means the banner command
# couldn't be run. strangely enough, it doesn't seem to work to do
# open(BANNER,...)||die "..." which is very strange... oh well...
$flag_width = scalar(@flag);
# get the width of the flag here... $#flag + 1 would also work
chop($flag[$#flag]);
# get rid of the last comma of the bottom-most entry
unshift(@flag, ("/*",
                " * flag.h - generated on ".localtime($^T)." by $user", 
                " * message : $mesg",
                " */",
                "#define flag_width $flag_width",
                "#define flag_height $flag_height",
                "static unsigned char flag_bits[] = {"));
# add all that to the start - a helpful comment, and the necessary
# #define statements
push(@flag, "}");
# whack a closing brace on the end
print @flag;
# write it to standard output - the user may not want ./flag.h to be
# clobbered :)

# and that's all :)

