default:
	$(MAKE) 

install:
	$(MAKE) DESTDIR="$(BINROOTDIR)" install
	$(MAKE) DESTDIR="$(BINROOTDIR)" install.man
	chmod 4755 $(BINROOTDIR)/usr/X386/bin/xlock

clean:
	$(MAKE) clean

