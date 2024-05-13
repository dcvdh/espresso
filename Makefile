# This is free and unencumbered software released into the public domain.

WARNINGS = -Wall -Wextra -Wcast-qual -Woverlength-strings -Wpointer-arith \
           -Wvla -Wwrite-strings -Wsign-compare
CFLAGS   = -Os -fno-ident -fno-asynchronous-unwind-tables ${WARNINGS}
CPPFLAGS = -DUNICODE
LDFLAGS  = -nostdlib -s -e Start -Wl,--subsystem,windows
LDLIBS   = -lkernel32 -luser32 -lshell32 -ladvapi32

RESULT = Espresso.exe
all: ${RESULT}

${RESULT}: resources.o

Espresso.c: utilities.h resources.rc

resources.rc: manifest.xml $(wildcard icons/*.ico)

%.exe: %.c
	${LINK.c} $^ -o $@ ${LDLIBS}

%.o: %.rc
	windres ${CPPFLAGS} $< $@

clean:
	rm *.exe *.o
