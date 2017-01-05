all: bin/flidget

clean:
	rm -f *.o *~ *.pyc
	rm -rf bin/flidget debian/usr

bin/flidget: src/flidget_parseconf.c src/flidget_console.c src/flidget_monitor.c src/flidget.c src/flidget.h
	gcc -g -O3 -Wall -pthread `pkg-config --cflags gtk+-3.0` -o bin/flidget src/flidget_parseconf.c src/flidget_console.c src/flidget_monitor.c src/flidget.c `pkg-config --libs gtk+-3.0` -lm

local_install: all
	mkdir -p /usr/local/bin
	mkdir -p /usr/local/man/man1/
	cp -f bin/* /usr/local/bin
	cp -f man/man1/* /usr/local/man/man1/

uninstall:
	rm -f /usr/local/bin/flidget*
	rm -f /usr/local/man/man1/flidget*

debian: all
	rm -rf debian/usr
	mkdir -p debian/usr/bin
	mkdir -p debian/usr/share/man/man1
	mkdir -p debian/usr/share/doc/flidget
	#bin
	strip --strip-unneeded bin/flidget
	cp -f bin/* debian/usr/bin
	chmod 755 debian/usr/bin/*
	#man
	cp -f man/man1/* debian/usr/share/man/man1
	chmod 644 debian/usr/share/man/man1/*
	gzip -f -n --best debian/usr/share/man/man1/*
	#doc
	cp -f changelog debian/usr/share/doc/flidget/changelog.Debian
	cp -f doc/* debian/usr/share/doc/flidget
	gzip -f -n --best debian/usr/share/doc/flidget/*
	cp -f copyright debian/usr/share/doc/flidget
	chmod 644 debian/usr/share/doc/flidget/*
	#build
	find debian -type d | xargs chmod 755
	fakeroot dpkg-deb --build debian
	mv debian.deb ../flidget_1.0-1_amd64.deb
	lintian ../flidget_1.0-1_amd64.deb
