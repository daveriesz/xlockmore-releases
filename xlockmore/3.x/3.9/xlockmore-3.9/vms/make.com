$ SAVE_VERIFY='F$VERIFY(0)
$! set ver 
$!
$!
$!      Compile and link xlock
$!
$! USAGE:
$! @make [link debug clobber clean]
$!       link : linking only
$!       debug : compile with degugger switch
$!       clean : clean all except executable
$!       clobber : clean all
$!
$! If you have
$!              XPM library
$!              XVMSUTILS library (VMS6.2 or lower)
$! insert the correct directory instead of X11:
$ xvmsf="X11:XVMSUTILS.OLB"
$ xpmf="X11:LIBXPM.OLB"
$!
$!
$! NOTHING SHOULD BE MODIFIED BELOW
$!
$ if p1 .Eqs. "CLEAN" then goto clean
$ if p1 .Eqs. "CLOBBER" then goto clobber
$!
$ xpm = f$search("''xpmf'").nes.""
$ axp = f$getsyi("HW_MODEL").ge.1024
$ sys_ver = f$edit(f$getsyi("version"),"compress")
$ if f$extract(0,1,sys_ver) .nes. "V"
$ then
$ type sys$input
You appear to be running a Field Test version of VMS. This script will 
assume that the operating system version is at least V7.0. 

$ sys_ver = "V7.0"
$ endif
$ sys_maj = 0+f$extract(1,1,sys_ver)
$ if sys_maj .lt. 7 then  xvms = f$search("''xvmsf'").nes.""
$!
$!
$! Create .opt file
$ open/write optf xlock.opt
$!
$ defs=="VMS"
$ if xpm then defs=="''defs',HAS_XPM"
$ if axp then defs=="''defs',VMS_PLAY"
$ if sys_maj .lt. 7
$ then
$ if xvms then defs=="''defs',XVMSUTILS"
$ endif
$!
$! If your system does not have bitmaps, you should
$! decw$bitmaps:==[-.bitmaps]
$! Many systems are distributed with a nulls at the end of these include
$! files which causes shapes.c not to compile.
$!
$! Establish the Compiling Environment
$!
$!
$! Set compiler command
$if axp
$ then
$   cc=="CC/list/STANDAR=VAXC/DEFINE=(''defs')"
$   else
$   cc=="CC/DEFINE=(''defs')"
$endif
$if p1.eqs."DEBUG" .or. p2.eqs."DEBUG" .or. p3.eqs."DEBUG"
$ then
$ if axp
$ then
$   cc=="CC/DEB/NOOPT/STANDAR=VAXC/DEFINE=(''defs')/list"
$   else
$     cc=="CC/DEB/NOOPT/define=''defs'"
$  endif
$   link=="LINK/DEB"
$ endif
$!
$ if p1.nes."LINK"
$ then
$ define/nolog sys sys$library
$!
$ write sys$output "Copying Bitmaps (and Pixmaps)"
$ call make image.xpm	"copy [.pixmaps]image-x11.xpm image.xpm"	[.pixmaps]image-x11.xpm
$ call make maze.xbm	"copy [.bitmaps]maze-x11.xbm maze.xbm"	[.bitmaps]maze-x11.xbm
$ call make life.xbm	"copy [.bitmaps]life-x11.xbm life.xbm"	[.bitmaps]life-x11.xbm
$ call make image.xbm	"copy [.bitmaps]image-x11.xbm image.xbm"	[.bitmaps]life-x11.xbm
$ call make flag.h	"copy [.flags]flag-vms.h flag.h"	[.flags]flag-vms.h
$!
$ write sys$output "Compiling XLock"
$ call make xlock.obj	"cc xlock.c"	xlock.c xlock.h mode.h vroot.h
$ call make passwd.obj	"cc passwd.c"	passwd.c xlock.h
$ call make resource.obj	"cc resource.c"	resource.c xlock.h mode.h
$ call make utils.obj	"cc utils.c"	utils.c xlock.h
$ call make logout.obj	"cc logout.c"	logout.c xlock.h
$ call make mode.obj	"cc mode.c"	mode.c xlock.h mode.h
$ call make ras.obj	"cc ras.c"	ras.c xlock.h
$ call make xbm.obj	"cc xbm.c"	xbm.c xlock.h
$ call make color.obj	"cc color.c"	color.c xlock.h
$ call make ant.obj	"cc ant.c"	ant.c xlock.h mode.h
$ call make bat.obj	"cc bat.c"	bat.c xlock.h mode.h
$ call make blot.obj	"cc blot.c"	blot.c xlock.h mode.h
$ call make bouboule.obj	"cc bouboule.c"	bouboule.c xlock.h mode.h
$ call make bounce.obj	"cc bounce.c"	bounce.c xlock.h mode.h
$ call make braid.obj	"cc braid.c"	braid.c xlock.h mode.h
$ call make bug.obj	"cc bug.c"	bug.c xlock.h mode.h
$ call make clock.obj	"cc clock.c"	clock.c xlock.h mode.h
$ call make demon.obj	"cc demon.c"	demon.c xlock.h mode.h
$ call make eyes.obj	"cc eyes.c"	eyes.c xlock.h mode.h
$ call make flag.obj	"cc flag.c"	flag.c xlock.h mode.h
$ call make flame.obj	"cc flame.c"	flame.c xlock.h mode.h
$ call make forest.obj	"cc forest.c"	forest.c xlock.h mode.h
$ call make galaxy.obj	"cc galaxy.c"	galaxy.c xlock.h mode.h
$ call make geometry.obj	"cc geometry.c"	geometry.c xlock.h mode.h
$ call make grav.obj	"cc grav.c"	grav.c xlock.h mode.h
$ call make helix.obj	"cc helix.c"	helix.c xlock.h mode.h
$ call make hop.obj	"cc hop.c"	hop.c xlock.h mode.h
$ call make hyper.obj	"cc hyper.c"	hyper.c xlock.h mode.h
$ call make image.obj	"cc image.c"	image.c xlock.h mode.h ras.h
$ call make kaleid.obj	"cc kaleid.c"	kaleid.c xlock.h mode.h
$ call make laser.obj	"cc laser.c"	laser.c xlock.h mode.h
$ call make life.obj	"cc life.c"	life.c xlock.h mode.h
$ call make life1d.obj	"cc life1d.c"	life1d.c xlock.h mode.h
$ call make life3d.obj	"cc life3d.c"	life3d.c xlock.h mode.h
$ call make lissie.obj	"cc lissie.c"	lissie.c xlock.h mode.h
$ call make marquee.obj	"cc marquee.c"	marquee.c xlock.h mode.h
$ call make maze.obj	"cc maze.c"	maze.c xlock.h mode.h
$ call make mountain.obj	"cc mountain.c"	mountain.c xlock.h mode.h
$ call make nose.obj	"cc nose.c"	nose.c xlock.h mode.h
$ call make qix.obj	"cc qix.c"	qix.c xlock.h mode.h
$ call make petal.obj	"cc petal.c"	petal.c xlock.h mode.h
$ call make puzzle.obj	"cc puzzle.c"	puzzle.c xlock.h mode.h ras.h
$ call make pyro.obj	"cc pyro.c"	pyro.c xlock.h mode.h
$ call make rock.obj	"cc rock.c"	rock.c xlock.h mode.h
$ call make rotor.obj	"cc rotor.c"	rotor.c xlock.h mode.h
$ call make shape.obj	"cc shape.c"	shape.c xlock.h mode.h
$ call make slip.obj	"cc slip.c"	slip.c xlock.h mode.h
$ call make sphere.obj	"cc sphere.c"	sphere.c xlock.h mode.h
$ call make spiral.obj	"cc spiral.c"	spiral.c xlock.h mode.h
$ call make spline.obj	"cc spline.c"	spline.c xlock.h mode.h
$ call make swarm.obj	"cc swarm.c"	swarm.c xlock.h mode.h
$ call make swirl.obj	"cc swirl.c"	swirl.c xlock.h mode.h
$ call make triangle.obj	"cc triangle.c"	triangle.c xlock.h mode.h
$ call make wator.obj	"cc wator.c"	wator.c xlock.h mode.h
$ call make world.obj	"cc world.c"	world.c xlock.h mode.h
$ call make worm.obj	"cc worm.c"	worm.c xlock.h mode.h
$ call make blank.obj	"cc blank.c"	blank.c xlock.h mode.h
$ call make random.obj	"cc random.c"	random.c xlock.h mode.h
$! AMD and VMS_PLAY for SOUND uncomment VMS_PLAY in xlock.h
$ if axp
$then
$ call make amd.obj	"cc amd.c"	amd.c amd.h
$ call make vms_play.obj	"cc vms_play.c"	vms_play.c amd.h
$ endif
$ endif
$!
$! Get libraries
$ if xpm then write optf "''xpmf'/lib"
$ if sys_maj .lt. 7
$ then
$   if xvms then write optf "''xvmsf'/lib"
$ endif
$ write optf "sys$library:vaxcrtl/lib"
$ if axp then write optf "sys$library:ucx$ipc_shr/share"
$ if axp then write optf "sys$share:decw$xextlibshr/share"
$ if axp then write optf "sys$share:decw$xtlibshrr5/share"
$ if .not. axp then write optf "sys$library:ucx$ipc/lib"
$ write optf "sys$share:decw$dxmlibshr/share"
$ write optf "sys$share:decw$xlibshr/share"
$ close optf
$!
$! LINK
$ write sys$output "Linking XLock"
$ link/map xlock/opt
$set noverify
$ exit
$!
$ Clobber:      ! Delete executables, Purge directory and clean up object files
$!                and listings
$ Delete/noconfirm/log xlock.exe;*
$!
$ Clean:        ! Purge directory, clean up object files and listings
$ Purge
$ Delete/noconfirm/log *.lis;*
$ Delete/noconfirm/log *.obj;*
$ Delete/noconfirm/log *.opt;*
$ Delete/noconfirm/log *.map;*
$!
$ exit
$!
$MAKE: SUBROUTINE   !SUBROUTINE TO CHECK DEPENDENCIES
$ V = 'F$Verify(0)
$! P1 = What we are trying to make
$! P2 = Command to make it
$! P3 - P8  What it depends on
$
$ if ( f$extract( 0 , 3 , P2 ) .eqs. "cc " ) then write optf "''P1'"
$ if ( f$extract( 0 , 3 , P2 ) .eqs. "CC " ) then write optf "''P1'"
$
$ If F$Search(P1) .Eqs. "" Then Goto Makeit
$ Time = F$CvTime(F$File(P1,"RDT"))
$arg=3
$Loop:
$	Argument = P'arg
$	If Argument .Eqs. "" Then Goto Exit
$	El=0
$Loop2:
$	File = F$Element(El," ",Argument)
$	If File .Eqs. " " Then Goto Endl
$	AFile = ""
$Loop3:
$	OFile = AFile
$	AFile = F$Search(File)
$	If AFile .Eqs. "" .Or. AFile .Eqs. OFile Then Goto NextEl
$	If F$CvTime(F$File(AFile,"RDT")) .Ges. Time Then Goto Makeit
$	Goto Loop3
$NextEL:
$	El = El + 1
$	Goto Loop2
$EndL:
$ arg=arg+1
$ If arg .Le. 8 Then Goto Loop
$ Goto Exit
$
$Makeit:
$ Set Verify
$ 'P2
$ VV='F$Verify(0)
$Exit:
$ If V Then Set Verify
$ENDSUBROUTINE
