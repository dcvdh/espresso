CC = i686-w64-mingw32-gcc
WINDRES = i686-w64-mingw32-windres

CFLAGS = -O2 -Wall
CPPFLAGS = -DUNICODE -DR_NAME=\"Espresso\" -DR_VERSION=\"2023.10.25\"
LDFLAGS = -static -s -Wl,--subsystem,windows
LDLIBS = -lcomctl32 -lshlwapi

Espresso: resources/embed.o

%.o: %.rc
	${WINDRES} --use-temp-file ${CPPFLAGS} $< $@
