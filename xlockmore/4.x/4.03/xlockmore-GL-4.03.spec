Summary: X terminal locking program with many screensavers
Name: xlockmore-GL
Version: 4.03
Release: 2
Copyright: MIT
Group: X11/Utilities
Source0: ftp://sunsite.unc.edu/pub/Linux/X11/screensavers/xlockmore-4.03.tgz
Source1: m-redhat.xpm
Source2: m-redhat.xbm
Source3: s-redhat.xpm
Source4: s-redhat.xbm
Source5: xlock.pamd
Patch0: xlockmore-4.03-xpm.patch
Patch1: xlockmore-4.03-pam.patch
Patch2: xlockmore-4.03-fortune.patch
Patch3: xlockmore-4.03-redhat.patch
Patch4: xlockmore-4.03-manext.patch
Patch5: xlockmore-4.03-nas.patch
Requires: pam >= 0.56
BuildRoot: /tmp/xlockmore-root

%description
An enhanced version of the standard xlock program which allows you
to keep other users locked out of an X session while you are away
from the machine.  It runs one of many provided screensavers while
waiting for you to type your password, unlocking the session and
letting you at your X programs.
This version is compiled with -DUSE_SYSLOG -DALWAYS_ALLOW_ROOT -DUSE_VROOT
-DUSE_MULTIPLE_ROOT -DUSE_BOMB

%changelog

* Mon Jul 28 1997 Jon Schewe <schewe@freenet.msp.mn.us>

- Changed compile options to get rid of the libaudio dependency and to get rid
of auto logout button

* Thu Jul 17 1997 Kjetil Wiekhorst Jørgensen <jorgens@pvv.org>

- Added patch to allow using NAS with xlock.
- Added patch to allow manpages to be installed with '.1x' extension.

* Wed Jul 16 1997 Timo Karjalainen <timok@iki.fi>

- Upgraded to version 4.03

* Mon Jun 9 1997 Timo Karjalainen <timok@iki.fi>

- Upgraded to version 4.02.1

%prep
%setup -n xlockmore-4.03

%patch0 -p1
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1
%patch5 -p1

cp $RPM_SOURCE_DIR/m-redhat.xpm pixmaps/m-redhat.xpm
cp $RPM_SOURCE_DIR/m-redhat.xbm bitmaps/m-redhat.xbm
cp $RPM_SOURCE_DIR/m-redhat.xpm pixmaps/l-redhat.xpm
cp $RPM_SOURCE_DIR/m-redhat.xbm bitmaps/l-redhat.xbm
cp $RPM_SOURCE_DIR/s-redhat.xpm pixmaps/s-redhat.xpm
cp $RPM_SOURCE_DIR/s-redhat.xbm bitmaps/s-redhat.xbm

%build
CC="gcc -pipe" CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=/usr/X11R6\
	--without-motif --enable-syslog --enable-multiple-root\
make CC="gcc -pipe"

%install
rm -rf $RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT/etc/pam.d
make prefix=$RPM_BUILD_ROOT/usr/X11R6 install
install -m 644 -o 0 -g 0 ${RPM_SOURCE_DIR}/xlock.pamd $RPM_BUILD_ROOT/etc/pam.d/xlock

%clean
rm -rf $RPM_BUILD_ROOT

%files
%doc docs/*
%config /etc/pam.d/xlock
%config /usr/X11R6/lib/X11/app-defaults/XLock
/usr/X11R6/bin/xlock
/usr/X11R6/man/man1/xlock.1x
