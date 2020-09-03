$ set verify
$! cc="CC/STANDAR=VAXC"
$ if p1.eqs."DEBUG" .or. p2.eqs."DEBUG"
$ then
$!   cc=="CC/DEB/NOOPT/STANDAR=VAXC"
$   cc=="CC/DEB/NOOPT/"
$   link=="LINK/DEB"
$ endif
$ if p1.nes."LINK"
$ then
$ define sys sys$library
$!
$ copy [.bitmaps]maze-x11.xbm maze.xbm
$ copy [.bitmaps]life-x11.xbm life.xbm
$ copy [.bitmaps]image-x11.xbm image.xbm
$ cc xlock.c
$ cc passwd.c
$ cc resource.c
$ cc utils.c
$ cc opt.c
$ cc bat.c
$ cc blot.c
$ cc bounce.c
$ cc clock.c
$ cc flame.c
$ cc forest.c
$ cc galaxy.c
$ cc geometry.c
$ cc grav.c
$ cc helix.c
$ cc hop.c
$ cc hyper.c
$ cc image.c
$ cc kaleid.c
$ cc laser.c
$ cc life.c
$ cc life3d.c
$ cc maze.c
$ cc mountain.c
$ cc qix.c
$ cc pyro.c
$ cc rect.c
$ cc rock.c
$ cc rotor.c
$ cc sphere.c
$ cc spiral.c
$ cc spline.c
$ cc swarm.c
$ cc wator.c
$ cc world.c
$ cc worm.c
$ cc swirl.c
$ cc image.c
$ cc blank.c
$ endif
$!
$ link/map xlock,passwd,resource,utils,opt,-
       bat,blot,bounce,clock,forest,flame,-
       galaxy,geometry,grav,helix,hop,hyper,-
       image,kaleid,laser,life,life3d,-
       maze,mountain,pyro,qix,rect,rock,rotor,-
       sphere,spiral,spline,swarm,-
       wator,world,worm,-
       swirl,blank,-
       sys$library:vaxcrtl/lib,-
       sys$library:ucx$ipc/lib,-
       sys$input/opt
sys$share:decw$dxmlibshr/share
sys$share:decw$xlibshr/share
$! sys$library:ucx$ipc_shr/share
$ set noverify
$ exit
