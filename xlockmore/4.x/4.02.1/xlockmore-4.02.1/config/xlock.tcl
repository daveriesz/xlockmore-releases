#!/usr/local/bin/wish -f

# This is a start for a TCL launcher for xlock.
# This is very similar to xmlock ...
# More work needs to be done.  Help!  :)

# charles vidal 1996
#the list is multi selection I can't make it mono

proc mkMessage {} {
     toplevel .message
    frame .message.frame -borderwidth 10
    set w .message.frame

    frame $w.messagename
    label $w.messagename.l1 -text "password string" 
    entry $w.messagename.e1 -relief sunken 
    frame $w.specmessage
    label $w.specmessage.l2 -text "info string" 
    entry $w.specmessage.e2 -relief sunken
    pack $w.messagename $w.specmessage 
    pack $w.messagename.l1 -side left 
    pack $w.specmessage.l2 -side left 
    pack $w.messagename.e1 $w.specmessage.e2  -side top -pady 5 -fill x
    button .message.ok -text OK -command "destroy .message"
    pack $w .message.ok -side top -fill both

}
proc mkGeometry {} {
     toplevel .geometry
    frame .geometry.frame -borderwidth 10
    set w .geometry.frame

    frame $w.geometryname
    label $w.geometryname.l1 -text "geometry" 
    entry $w.geometryname.e1 -relief sunken 
    frame $w.specgeometry
    label $w.specgeometry.l2 -text "icon geometry" 
    entry $w.specgeometry.e2 -relief sunken
    pack $w.geometryname $w.specgeometry 
    pack $w.geometryname.l1 -side left 
    pack $w.specgeometry.l2 -side left 
    pack $w.geometryname.e1 $w.specgeometry.e2  -side top -pady 5 -fill x
    button .geometry.ok -text OK -command "destroy .geometry"
    pack $w .geometry.ok -side top -fill both

}

proc mkFont {} {
     toplevel .font
    frame .font.frame -borderwidth 10
    set w .font.frame

    frame $w.fontname
    label $w.fontname.l1 -text "font name" 
    entry $w.fontname.e1 -relief sunken 
    frame $w.specfont
    label $w.specfont.l2 -text "specifique font name" 
    entry $w.specfont.e2 -relief sunken
    pack $w.fontname $w.specfont 
    pack $w.fontname.l1 -side left 
    pack $w.specfont.l2 -side left 
    pack $w.fontname.e1 $w.specfont.e2  -side top -pady 5 -fill x
    button .font.ok -text OK -command "destroy .font"
    pack $w .font.ok -side top -fill both

}

proc mkEntry {} {
     toplevel .option
    frame .option.frame -borderwidth 10
    set w .option.frame

    frame $w.username
    label $w.username.l1 -text "user name" 
    entry $w.username.e1 -relief sunken 
    frame $w.password
    label $w.password.l2 -text "password" 
    entry $w.password.e2 -relief sunken
    frame $w.info
    label $w.info.l3 -text "program name" 
    entry $w.info.e3 -relief sunken
    pack $w.username $w.password $w.info 
    pack $w.username.l1 -side left 
    pack $w.password.l2 -side left 
    pack $w.info.l3 -side left 
    pack $w.username.e1 $w.password.e2 $w.info.e3 -side top -pady 5 -fill x
    button .option.ok -text OK -command "destroy .option"
    pack $w .option.ok -side top -fill both

}

wm title . "XLock launcher"
frame .menu -relief raised -borderwidth 1
menubutton .menu.button -text "switches" -menu .menu.button.check 
pack .menu -side top -fill x 

menu .menu.button.check
set CHECK .menu.button.check
#les check buttons
$CHECK add check -label "mono" -variable mono
$CHECK add check -label "nolock" -variable nolock
$CHECK add check -label "remote" -variable remote
$CHECK add check -label "allowroot" -variable allowroot
$CHECK add check -label "enablesaver" -variable enablesaver
$CHECK add check -label "allowaccess" -variable allowaccess
$CHECK add check -label "grabmouses" -variable grabmouses
$CHECK add check -label "echokey" -variable echokey
$CHECK add check -label "usefirst" -variable usefirst

menubutton .menu.button2 -text "options" -menu .menu.button2.options
menu .menu.button2.options
set OPTIONS .menu.button2.options
#les options
$OPTIONS add command -label "generals options" -command "mkEntry"
$OPTIONS add command -label "font options" -command "mkFont"
$OPTIONS add command -label "geometry options" -command "mkGeometry"
$OPTIONS add command -label "message options" -command "mkMessage"

pack .menu.button  .menu.button2 -side left


#creation de la liste 
frame .listscrol -borderwidth 4 -relief ridge
set LISTSCROL .listscrol
scrollbar $LISTSCROL.scroll -relief sunken -command "$LISTSCROL.list yview"
listbox $LISTSCROL.list -yscroll  "$LISTSCROL.scroll set" -relief sunken \
          
pack $LISTSCROL.scroll -side right -fill y
pack $LISTSCROL.list -side left -expand yes -fill both 
pack $LISTSCROL

$LISTSCROL.list  insert 0 \
ant ball bat blot bouboule bounce braid bug \
cartoon clock daisy dclock demon drift eyes \
flag flame forest galaxy gear geometry grav \
helix hop hyper image julia kaleid \
laser life life1d life3d lightning lissie loop \
marquee maze mountain nose \
pacman penrose petal puzzle pyro qix roll rotor \
shape slip sphere spiral spline star swarm swirl \
tri triangle turtle wator wire world worm \
blank bomb random

frame .buttons -borderwidth 4 -relief ridge
set BUTTON .buttons
button $BUTTON.launch -text "Launch"  -command "exec xlock"
button $BUTTON.launchinW -text "Launch in Window" -command "exec xlock -inwindow"
button $BUTTON.launchinR -text "Launch in Root" -command "exec xlock -inroot"
button $BUTTON.quit -text Quit -command exit
pack  $BUTTON.launch $BUTTON.launchinW $BUTTON.launchinR -side left
pack $BUTTON.quit -side right
pack $BUTTON -fill x -side bottom

