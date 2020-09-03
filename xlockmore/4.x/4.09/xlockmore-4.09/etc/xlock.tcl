#!/usr/local/bin/wish -f

#charles vidal 1997 <vidalc@club-internet.fr>

#function find in demo: mkStyles.tcl
# The procedure below inserts text into a given text widget and
# applies one or more tags to that text.  The arguments are:
# 
# w             Window in which to insert
# text          Text to insert (it's inserted at the "insert" mark)
# args          One or more tags to apply to text.  If this is empty
#               then all tags are removed from the text.
set bgcolor "" 
set fgcolor "" 
set ftname ""
set mftname ""
set usernom ""
set passmot ""
set validstr ""
set invalidstr ""
set prgname ""
set geometrie ""
set icogeometrie ""
set infostring ""
proc insertWithTags {w text args} {
    set start [$w index insert]
    $w insert insert $text
    foreach tag [$w tag names $start] {
        $w tag remove $tag $start insert
    }
    foreach i $args {
        $w tag add $i $start insert
    }
}
# Function for the help
proc mkHelpCheck { w args } {
  set nbf 0
  foreach i $args {
	set nbf [ expr $nbf +1 ]
	$w insert insert  "\n"
	checkbutton $w.c$nbf -variable [lindex $i 0] -text [lindex $i 0]
	$w window create {end lineend} -window $w.c$nbf
	$w insert insert  " [lindex $i 1] "
	} 
}
proc mkHelpEntry { w args } {
  set nbf 0

  foreach i $args {
	set nbf [ expr $nbf +1 ]
	insertWithTags  $w "\n [lindex $i 0] " underline
	entry $w.e$nbf -textvariable [lindex $i 1]
	$w window create {end lineend} -window $w.e$nbf
	$w insert insert  "\n[lindex $i 2] "
	} 
}
#
proc whichcolor { which } {
global fgcolor
global bgcolor
 if {$which== "FG" } {set fgcolor [.color.frame.names get [.color.frame.names curselection]];}
  if {$which == "BG"} {set bgcolor [.color.frame.names get [.color.frame.names curselection]];}
 if {$which == "RESETFG"} {set fgcolor ""}
 if {$which == "RESETBG"} {set bgcolor ""}
}
proc mkColor { what } {
     toplevel .color
	wm title .color "Color"
    frame .color.frame -borderwidth 10
    frame .color.frame2 -borderwidth 10
    set w .color.frame
    label $w.msg0   -text "Color Options" 
    pack $w.msg0 -side top
foreach i {/usr/local/lib/X11/rgb.txt /usr/lib/X11/rgb.txt
        /X11/R5/lib/X11/rgb.txt /X11/R4/lib/rgb/rgb.txt} {
    if ![file readable $i] {
        continue;
    }
    set f [open $i]
    listbox .color.frame.names -yscrollcommand ".color.frame.scroll set" \
            -relief sunken -borderwidth 2 -exportselection false
    bind .color.frame.names <Double-1> {
            .color.test configure -bg [.color.frame.names get [.color.frame.names curselection]]
    }
    scrollbar .color.frame.scroll -orient vertical -command ".color.frame.names yview" \
            -relief sunken -borderwidth 2
    pack .color.frame.names  -side left
    pack .color.frame.scroll -side right -fill both
    pack .color.frame  -fill x
    while {[gets $f line] >= 0} {
        if {[llength $line] == 4} {
            .color.frame.names insert end [lindex $line 3]
        }
    }
    close $f
	label  .color.test -height 5 -width 20
        button .color.frame2.cancel -text Cancel -command "destroy .color"
        button .color.frame2.ok -text OK -command "whichcolor $what; destroy .color"
        button .color.frame2.reset -text Reset -command "whichcolor RESET$what; destroy .color"
	pack  .color.test 
	pack .color.frame2.ok .color.frame2.cancel .color.frame2.reset -side left -fill x
	pack .color.frame2 -fill both
    break;
}

}
# Help ...
proc Helpxlock {} {
     toplevel .help
	wm title .help "Help About Xlock"
frame .help.f 
scrollbar .help.f.s -orient vertical -command {.help.f.t yview}
pack .help.f
pack .help.f.s -side right -fill y
text .help.f.t -yscrollcommand {.help.f.s set} -wrap word -width 60 -height 30 \
        -setgrid 1
pack .help.f.t -expand y -fill both
.help.f.t tag configure big -font -Adobe-Courier-Bold-R-Normal-*-140-*
.help.f.t tag configure verybig -font -Adobe-Courier-Bold-R-Normal-*-240-*
.help.f.t tag configure color2 -foreground red
.help.f.t tag configure underline -underline on
insertWithTags .help.f.t  "Xlock Help\n"  verybig
insertWithTags .help.f.t "\n"
.help.f.t insert end { Locks the X server still the user enters their pass\
word at the keyboard.  While xlock  is  running,  all  new\
server  connections are refused.  The screen saver is dis\
abled.  The mouse cursor is turned  off.   The  screen  is\
blanked and a changing pattern is put on the screen.  If a\
key or a mouse button is pressed then the user is prompted\
for the password of the user who started xlock.
If  the  correct  password  is  typed,  then the screen is\
unlocked and the X server is restored.   When  typing  the\
password  Control-U  and  Control-H are active as kill and\
erase respectively.  To return to the locked screen, click\
in the small icon version of the changing pattern.
}
.help.f.t insert end "\n"
insertWithTags .help.f.t  "OPTIONS\n"  big
insertWithTags .help.f.t  "\n\n-display "  underline
insertWithTags .help.f.t {The option sets  the  X11  display  to  lock.\
            xlock  locks all available screens on a given server,\
            and restricts you to locking only a local server such\
            as  unix::00,,  localhost::00,,  or  ::00  unless you set the\
     -remote option.}
.help.f.t insert end "\n"
insertWithTags .help.f.t  "\n\n-name "  underline
insertWithTags .help.f.t  { is used instead of XLock  when  looking\
            for resources to configure xlock.}
.help.f.t insert end "\n"
insertWithTags .help.f.t  "\n\n-mode "  underline
insertWithTags .help.f.t {As  of  this  writing there are 48 display modes sup\
            ported (plus one more for random selection of one  of\
            the 48).}
insertWithTags .help.f.t  "\n\n-delay "  underline
insertWithTags .help.f.t {It simply sets the number  of  microseconds to  delay  between  batches  of animations.  In blank mode, it is important to set this to some small  number  of  microseconds,  because the keyboard and mouse are only checked after each delay, so you cannot set  the delay  too high, but a delay of zero would needlessly consume cpu checking for mouse and keyboard input  in a tight loop, since blank mode has no work to do.}
insertWithTags .help.f.t  "\n\n-saturation"  underline
insertWithTags .help.f.t { This option sets saturation  of  the  color
 ramp .  0 is grayscale and 1 is very rich color. 0.4 is a nice pastel.}
insertWithTags .help.f.t  "\n\n-nice"  underline
insertWithTags .help.f.t { This option sets system nicelevel  of  the  xlock .}
insertWithTags .help.f.t  "\n\n-timeout "  underline
insertWithTags .help.f.t { This option sets the number of second before the password screen will time out.}
insertWithTags .help.f.t  "\n\n-lockdelay "  underline
insertWithTags .help.f.t { This  option  sets  the  number  of second before  the  screen  needs a password to be unlocked.  Good for use with an autolocking mechanism like xautolock(1).}
insertWithTags .help.f.t  "\n\n-font "  underline
#    button .help.f.t.b1 -text "font" -command "mkFont FONT"
#    .help.f.t window create {end lineend} -window .help.f.t.b1
insertWithTags .help.f.t { Ths option  sets  the  font  to be used on the prompt screen.}
insertWithTags .help.f.t  "\n\n-fg "  underline
insertWithTags .help.f.t { This option sets the color of the text on the password screen.}
insertWithTags .help.f.t  "\n\n-bg "  underline
insertWithTags .help.f.t { This option sets the color of the background on the password screen.}
mkHelpEntry .help.f.t {-username usernom {text string to use for Name prompt}}\
	{-password passmot {text string to use for Password prompt}}\
	{-info infostring {text string to use for instructions}}\
	{-validate validstr {the message shown  while  validating  the password, defaults to \"Validating login...\"}}\
	{-invalid invalidstr {the  message  shown  when  password  is invalid, defaults to \"Invalid login.\"}}\
	{-geometry geometrie {This option sets the size and offset  of the  lock  window  (normally the entire screen).  The entire screen format is still used for  entering  the password.   The  purpose  is  to  see the screen even though it is locked.  This should be used  with  caution since many of the modes will fail if the windows are far from square or are too small  (size  must  be greater  than  0x0).   This  should also be used with esaver to protect screen from phosphor burn.}}\
{-icongeometry icongeometrie {this option sets  the  size  of  the iconic screen (normally 64x64) seen when entering the password.  This should be  used  with  caution  since many  of  the  modes will fail if the windows are far from square or are too small (size  must  be  greater than  0x0).   The  greatest  size  is 256x256.  There should be some limit  so  users  could  see  who  has locked  the  screen.  Position information of icon is ignored.}}
insertWithTags .help.f.t  "\n\n-forceLogout "  underline
insertWithTags .help.f.t { This option sets the  auto-logout.  This  might not be enforced depending how your system is configured.}
insertWithTags .help.f.t  "\n\nOPTIONS BOOLEAN\n"  big
  mkHelpCheck .help.f.t {mono {turn on/off monochrome override}}\
		{nolock {trun on/off no password required mode}}\
		{remote {turn on/off remote host access}}\
		{allowroot {turn on/off allow root password mode (ignored)}}\
		{enablesaver {turn on/off enable X server screen saver}}\
		{allowaccess {turn on/off access of the terminal X}}\
		{grabmouse {turn on/off grabbing of mouse and keyboard}}\
		{echokeys {turn on/off echo \'?\' for each password key}}\
		{usefirst {turn on/off using the first char typed in password}}\
		{verbose {turn on/off verbose mode}}\
		{inwindow {turn on/off making xlock run in a window}}\
		{inroot {turn on/off making xlock run in the root window}}\
		{timeelapsed {turn on/off clock}}\
		{install {whether to use private colormap if needed (yes/no)}}\
		{sound {whether to use sound if configured for it (yes/no}}

        button .help.ok -text OK -command "destroy .help"
	pack  .help.ok
}
# Create toplevel Author and Maintener.
proc mkAuthor {} {
     toplevel .author
    wm title .author "Author and Maintener of xlock"
    frame .author.frame -borderwidth 10
    set w .author.frame

    label $w.msg0   -text "Author and Maintener of xlock" 
    label $w.msg1   -text "Maintained by: David A. Bagley (bagleyd@bigfoot.com)"
label $w.msg2   -text "Original Author: Patrick J. Naughton (naughton@eng.sun.com)" 
label $w.msg3   -text "Mailstop 21-14 Sun Microsystems Laboratories,"
 label $w.msg4   -text "Inc.  Mountain View, CA  94043 15//336-1080"
label $w.msg5   -text "with many additional contributors"
        pack $w.msg0 $w.msg1 $w.msg2 $w.msg3 $w.msg4 $w.msg5 -side top

        label $w.msg6   -text "xlock.tcl\n created by charles VIDAL\n (author of flag mode and xmlock launcher )"
        pack $w.msg6 -side top

    button .author.ok -text OK -command "destroy .author"
    pack $w .author.ok 
}
proc mkDialog { nom titre args } {
  toplevel .$nom
  wm title .$nom "$titre"
  frame .$nom.frame -borderwidth 10
  frame .$nom.frame2 -borderwidth 10
  set w .$nom.frame
  set w2 .$nom.frame2
  set nbf 0

  label $w.msg0   -text "$titre" 
  pack $w.msg0 -side top
  foreach i $args {
	set nbf [ expr $nbf +1 ]
	frame $w.f$nbf
	set wf $w.f$nbf
	label $wf.l$nbf -text [lindex $i 0]
	entry $wf.e$nbf -textvariable [lindex $i 1]
	pack $wf.l$nbf  -side left
	pack $wf.e$nbf  -side right
	pack $wf
	} 
    button $w2.ok -text OK -command "destroy .$nom"
    button $w2.cancel -text Cancel -command "destroy .$nom"
    pack $w -side top 
    pack $w2.ok $w2.cancel -side left -fill x
	pack $w2  -side bottom
}
proc mkMessage {} {
global passmot
global validstr
global invalidstr
global infostring 
	mkDialog message {Message Options} \
	{"message password" passmot} \
	{"validate string" validstr} \
	{"invalid string" invalidstr} \
	{"info string" infostring} 
}
#no influence with xlock
proc mkGeometry {} {
	global geometrie
	global icogeometrie
	mkDialog geometry {Geometry Options} \
	{"geometry" geometrie} \
	{"icon geometry" icogeometrie}
}

proc whichfont { which } {
global ftname
global mftname
 if {$which== "FONT" } {set ftname [.font.frame.names get [.font.frame.names curselection]];}
 if {$which == "MFONT"} {set mftname [.font.frame.names get [.font.frame.names curselection]];}
 if {$which == "RESETFONT"} {set ftname ""}
 if {$which == "RESETMFONT"} {set mftname ""}
}
#this function should be erase in the newer version...
proc mkFont { What } {
     toplevel .font 
	wm title .font "Font Options"
	label  .font.label -text "ABCDEFGH\nIJKabedfg\nhijkmnopq" 
    frame .font.frame -borderwidth 10 
    frame .font.frame2 -borderwidth 10
    set w .font.frame
    label $w.msg0   -text "Font Options" 
    pack $w.msg0 -side top
    eval exec "xlsfonts \> /tmp/xlsfont.tmp"
    set f [open "/tmp/xlsfont.tmp"]
    listbox .font.frame.names -yscrollcommand ".font.frame.scroll set" \
	-xscrollcommand ".font.scroll2 set"  -setgrid 1 \
         -exportselection false 
    bind .font.frame.names <Double-1> {
            .font.test configure -font [.font.frame.names get [.font.frame.names curselection]]
    }
    scrollbar .font.frame.scroll -orient vertical -command ".font.frame.names yview" \
            -relief sunken -borderwidth 2
    scrollbar .font.scroll2 -orient horizontal -command ".font.frame.names xview" \
            -relief sunken -borderwidth 2
    while {[gets $f line] >= 0} {
            .font.frame.names insert end $line 
    }
    close $f

    eval exec "/bin/rm -f /tmp/xlsfont.tmp"
    pack .font.frame.names  -side left -expand y -fill both
    pack .font.frame.scroll -side right -fill both
    pack .font.frame  -fill x
    pack .font.scroll2 -fill both
	label  .font.test -text "ABCDEFGHIJKabedfghijkmnopq12345" 
        pack .font.test

    button .font.frame2.cancel -text Cancel -command "destroy .font"
    button .font.frame2.reset -text Reset -command "whichfont RESET$What;destroy .font"
    button .font.frame2.ok -text OK -command "whichfont $What;destroy .font"
	pack .font.frame2.ok .font.frame2.cancel .font.frame2.reset -side left -fill both
	pack .font.frame2 -fill both

    #frame $w.fontname
    #label $w.fontname.l1 -text "font name" 
    #entry $w.fontname.e1 -relief sunken 
    #frame $w.specfont
    #label $w.specfont.l2 -text "specifique font name" 
    #entry $w.specfont.e2 -relief sunken
    #pack $w.fontname $w.specfont 
    #pack $w.fontname.l1 -side left 
    #pack $w.specfont.l2 -side left 
    #pack $w.fontname.e1 $w.specfont.e2  -side top -pady 5 -fill x
    #button .font.frame2.ok -text OK -command "destroy .font"
    #button .font.frame2.cancel -text Cancel -command "destroy .font"
    #pack $w .font.frame2.ok .font.frame2.cancel -side left -fill x 
	#pack .font.frame2 -side bottom
}

proc mkEntry {} {
global usernom
global prgname
	mkDialog option {User Options} \
	{"user name" usernom} \
	{"program name" prgname}
}

proc Affopts { device } {

#options booleans
global mono
global nolock
global remote
global allowroot
global enablesaver
global allowaccess
global grabmouse
global echokeys
global usefirst
global install
global sound

global fgcolor
global bgcolor
global ftname
global mftname

global usernom 
global passmot 
global validstr 
global invalidstr 
global prgname
global geometrie 
global icogeometrie 
global infostring 

set linecommand "xlock "

if {$device == 1} {append linecommand "-inwindow "} elseif {$device == 2} {append linecommand "-inroot "}
if {$bgcolor!=""} {append linecommand "-bg $bgcolor "}
if {$fgcolor!=""} {append linecommand "-fg $fgcolor "}
if {$ftname!=""} {append linecommand "-font $ftname "}
if {$mftname!=""} {append linecommand "-mfont $mftname "}
#entry action
if {$usernom!=""} {append linecommand "-username $usernom "}
if {$passmot!=""} {append linecommand "-password $passmot "}
if {$validstr!=""} {append linecommand "-validate $validstr "}
if {$invalidstr!=""} {append linecommand "-invalid $invalidstr "}
if {$prgname!=""} {append linecommand "-program $prgname "}
if {$geometrie!=""} {append linecommand "-geometry $geometrie "}
if {$icogeometrie!=""} {append linecommand "-icongeometry $icogeometrie "}
if {$infostring!=""} {append linecommand "-info $infostring "}
#check actions
if { $mono == 1 } {append linecommand "-mono "}
if { $install == 1 } {append linecommand "+install "}
if { $sound == 1 } {append linecommand "+sound "}
if { $nolock == 1 } {append linecommand "-nolock "}
if { $remote == 1 } {append linecommand "-remote "}
if { $allowroot == 1 } {append linecommand "-allowroot "}
if { $enablesaver == 1 } {append linecommand "-enablesaver "}
if { $allowaccess == 1 } {append linecommand "-allowaccess "}
if { $grabmouse == 1 } {append linecommand "-grabmouse "}
if { $echokeys == 1 } {append linecommand "-echokeys "}
if { $usefirst == 1 } {append linecommand "-usefirst "}
append linecommand "-mode "
append linecommand [.listscrol.list get [eval .listscrol.list curselection]]
puts $linecommand
eval exec $linecommand
}

# Creation of GUI 

wm title . "xlock launcher"
. configure -cursor top_left_arrow
frame .menu -relief raised -borderwidth 1
menubutton .menu.button -text "switches" -menu .menu.button.check 
pack .menu -side top -fill x 

global mono
global sound
global install
global nolock
global remote
global allowroot
global enablesaver
global allowaccess
global grabmouses
global echokeys
global usefirst

global usernom 
global passmot
global geometrie
global icogeometrie
global infostring 

# Creation of GUI 

#Creation of  menu 

menu .menu.button.check
set CHECK .menu.button.check

#menu with les check buttons
$CHECK add check -label "mono" -variable mono
$CHECK add check -label "nolock" -variable nolock
$CHECK add check -label "remote" -variable remote
$CHECK add check -label "allowroot" -variable allowroot
$CHECK add check -label "enablesaver" -variable enablesaver
$CHECK add check -label "allowaccess" -variable allowaccess
$CHECK add check -label "grabmouse" -variable grabmouse 
$CHECK add check -label "echokeys" -variable echokeys
$CHECK add check -label "usefirst" -variable usefirst
$CHECK add check -label "install" -variable install
$CHECK add check -label "sound" -variable sound

menubutton .menu.button2 -text "options" -menu .menu.button2.options
menu .menu.button2.options
set OPTIONS .menu.button2.options
#les options
$OPTIONS add command -label "generals options" -command "mkEntry"
$OPTIONS add command -label "font to use for password prompt" -command "mkFont FONT"
$OPTIONS add command -label "font for a specific mode" -command "mkFont MFONT"


$OPTIONS add command -label "geometry options" -command "mkGeometry"
$OPTIONS add command -label "message options" -command "mkMessage"

#Color
menubutton .menu.button4 -text "color" -menu .menu.button4.color
menu .menu.button4.color
set COLOR .menu.button4.color
$COLOR add command -label "Foreground options for password" -command "mkColor FG"
$COLOR add command -label "Background options for password" -command "mkColor BG"

menubutton .menu.button3 -text "help" -menu .menu.button3.help
menu .menu.button3.help
set HELP .menu.button3.help
$HELP add command -label "About xlock" -command "Helpxlock"
$HELP add command -label "About author" -command "mkAuthor"

pack .menu.button  .menu.button2 .menu.button4 -side left
pack .menu.button3 -side right

#creation de la liste 
frame .listscrol -borderwidth 4 -relief ridge 
set LISTSCROL .listscrol
scrollbar $LISTSCROL.scroll -relief sunken -command "$LISTSCROL.list yview"
listbox $LISTSCROL.list -yscroll  "$LISTSCROL.scroll set" 
pack $LISTSCROL.scroll -side right -fill y
pack $LISTSCROL.list -side left -expand yes -fill both 
pack $LISTSCROL -fill both

$LISTSCROL.list  insert 0 \
ant ball bat blot \
bouboule bounce braid bubble bug \
cage cartoon clock coral crystal \
daisy dclock deco demon dilemma drift eyes \
fadeplot flag flame forest \
galaxy gears grav \
helix hop hyper \
ico ifs image julia kaleid \
laser life life1d life3d \
lightning lisa lissie loop \
mandelbrot marquee maze moebius morph3d mountain munch nose \
pacman penrose petal pipes puzzle pyro \
qix roll rotor rubik \
shape sierpinski slip \
sphere spiral spline sproingies stairs \
star strange superquadrics swarm swirl \
triangle tube turtle vines voters \
wator wire world worm \
blank bomb random

frame .buttons -borderwidth 4 -relief ridge
set BUTTON .buttons
button $BUTTON.launch -text "Launch"  -command "Affopts 0"
button $BUTTON.launchinW -text "Launch in Window" -command "Affopts 1"
button $BUTTON.launchinR -text "Launch in Root" -command "Affopts 2"
button $BUTTON.quit -text Quit -command "exit"
pack  $BUTTON.launch $BUTTON.launchinW $BUTTON.launchinR -side left
pack $BUTTON.quit -side right
pack $BUTTON -fill x -side bottom

