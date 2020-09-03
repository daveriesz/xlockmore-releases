#!/bin/sh
VERSION=`cat xlock/version.h | cut -d\" -f2 | cut -d\- -f2`
NAME=xlockmore
PROG=$NAME-$VERSION
AUTHOR="David Bagley"

if test "$1" = "-p"; then
	PROG=$NAME
fi
if test "$VERSION" = ""; then
	echo "run from $NAME main directory"
	exit
fi
make -f Makefile.in distclean
mkdir -p $PROG
LIST=`ls`
EXCEPT="linux solaris $PROG"
for i in $LIST; do
	FOUND=0
	for j in $EXCEPT; do
		if test "$i" = "$j"; then
			FOUND=1
			break
		fi
	done
	if test $FOUND -eq 0; then
		cp -rp $i $PROG
	fi
done	
tar cvjf $PROG.tar.bz2 $PROG
rm -rf $PROG
mv $PROG.tar.bz2 ..
