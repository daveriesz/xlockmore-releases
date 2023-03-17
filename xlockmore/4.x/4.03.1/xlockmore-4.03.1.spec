Summary: X terminal locking program with many screensavers
Name: xlockmore
Version: 4.03.1
Release: 1
Copyright: MIT
Group: X11/Utilities
Source0: ftp://ftp.x.org/contrib/applications/xlockmore-4.03.1.tar.gz
Source1: m-redhat.xpm
Source2: m-redhat.xbm
Source3: s-redhat.xpm
Source4: s-redhat.xbm
Source5: xlock.pamd
Patch0: xlockmore-4.03-xpm.patch
Patch1: xlockmore-4.03-pam.patch
Patch2: xlockmore-4.03-fortune.patch
Patch3: xlockmore-4.03-redhat.patch
Requires: pam >= 0.56
BuildRoot: /tmp/xlockmore-root

%description
An enhanced version of the standard xlock program which allows you
to keep other users locked out of an X session while you are away
from the machine.  It runs one of many provided screensavers while
waiting for you to type your password, unlocking the session and
letting you at your X programs.

%changelog

* Tue Jul 29 1997 Timo Karjalainen <timok@iki.fi>

- Upgraded to version 4.03.1

* Wed Jul 16 1997 Timo Karjalainen <timok@iki.fi>

- Upgraded to version 4.03

* Mon Jun 9 1997 Timo Karjalainen <timok@iki.fi>

- Upgraded to version 4.02.1

%prep
%setup

%patch0 -p1 -b .xpm
%patch1 -p1 -b .nopam
%patch2 -p1 -b .fortune
%patch3 -p1 -b .redhat

cp $RPM_SOURCE_DIR/m-redhat.xpm pixmaps/m-redhat.xpm
cp $RPM_SOURCE_DIR/m-redhat.xbm bitmaps/m-redhat.xbm
cp $RPM_SOURCE_DIR/m-redhat.xpm pixmaps/l-redhat.xpm
cp $RPM_SOURCE_DIR/m-redhat.xbm bitmaps/l-redhat.xbm
cp $RPM_SOURCE_DIR/s-redhat.xpm pixmaps/s-redhat.xpm
cp $RPM_SOURCE_DIR/s-redhat.xbm bitmaps/s-redhat.xbm

%build
xmkmf -a

make

%install
rm -rf $RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT/etc/pam.d
make DESTDIR=$RPM_BUILD_ROOT install install.man
install -m 644 -o 0 -g 0 ${RPM_SOURCE_DIR}/xlock.pamd $RPM_BUILD_ROOT/etc/pam.d/xlock

%clean
rm -rf $RPM_BUILD_ROOT

%files
%config /etc/pam.d/xlock
%config /usr/X11R6/lib/X11/app-defaults/XLock
/usr/X11R6/bin/xlock
/usr/X11R6/man/man1/xlock.1x
