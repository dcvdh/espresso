CC = i686-w64-mingw32-gcc
WINDRES = i686-w64-mingw32-windres

CFLAGS = -O2 -Wall
CPPFLAGS = -DUNICODE
LDFLAGS = -static -s -Wl,--subsystem,windows
LDLIBS = -lcomctl32 -lshlwapi

Espresso.exe: resources/embed.o

%.exe: %.c
	${LINK.c} $^ -o $@ ${LDLIBS}
%.o: %.rc
	${WINDRES} --use-temp-file ${CPPFLAGS} $< $@

clean:
	git clean -dXf
