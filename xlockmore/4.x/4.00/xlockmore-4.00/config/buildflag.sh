#!/bin/sh
banner -w 25 AIX | ../config/xlockflag.pl > flag-aix.h
banner -w 25 BSD | ../config/xlockflag.pl > flag-bsd.h
banner -w 25 HPUX | ../config/xlockflag.pl > flag-hp.h
banner -w 25 LINUX | ../config/xlockflag.pl > flag-linux.h
banner -w 25 IRIX | ../config/xlockflag.pl > flag-sgi.h
banner -w 25 SOLARIS | ../config/xlockflag.pl > flag-sol.h
banner -w 25 SUNOS | ../config/xlockflag.pl > flag-sun.h
banner -w 25 SYSV | ../config/xlockflag.pl > flag-sysv.h
banner -w 25 ULTRIX | ../config/xlockflag.pl > flag-ultrix.h
banner -w 25 UNIX | ../config/xlockflag.pl > flag-unix.h
banner -w 25 VMS | ../config/xlockflag.pl > flag-vms.h
