#!/bin/sh
VERSION=`grep VERSION xlock/version.h | cut -d\" -f2`
NAME="xlock95"
PROG="${NAME}-${VERSION}"
PROGZIP="${PROG}.zip"

#./makezip.sh
cd ..
gpg --print-md SHA512 $PROGZIP > $PROGZIP.sha
gpg --armor --output $PROGZIP.asc --detach-sig $PROGZIP
gpg --verify $PROGZIP.asc $PROGZIP
