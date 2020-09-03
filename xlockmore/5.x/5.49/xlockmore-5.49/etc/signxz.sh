#!/bin/sh
VERSION=`grep VERSION xlock/version.h | cut -d\" -f2`
NAME="xlockmore"
PROG="${NAME}-${VERSION}"
PROGZIP="${PROG}.tar.xz"

#./makexz.sh  [no -p]
cd ..
gpg --print-md SHA512 $PROGZIP > $PROGZIP.sha
gpg --armor --output $PROGZIP.asc --detach-sig $PROGZIP
gpg --verify $PROGZIP.asc $PROGZIP
