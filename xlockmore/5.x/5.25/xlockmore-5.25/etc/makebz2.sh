#!/bin/sh
VERSION=`cat xlock/version.h | cut -d\" -f2 | cut -d\- -f2`
NAME=xlockmore
PROG=$NAME-$VERSION
PROGZIP=$PROG.tar.bz2
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
tar cvf - $PROG | bzip2 > $PROGZIP
rm -rf $PROG
mv $PROGZIP ..
cd ..
gpg --print-md SHA1 $PROGZIP > $PROGZIP.sha
gpg --print-md MD5 $PROGZIP > $PROGZIP.md5
gpg --armor --output $PROGZIP.asc --detach-sig $PROGZIP
gpg --verify $PROGZIP.asc $PROGZIP
