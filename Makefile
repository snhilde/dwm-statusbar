# barname version
VERSION = 1.1

# paths
# X11INC = /usr/X11R6/include
# X11LIB = /usr/X11R6/lib

# includes and libs
INCS = -I /usr/include \
       -I /usr/include/libnl3 \
       -I /usr/include/glib-2.0 \
       -I /usr/lib/glib-2.0/include \
       -I /usr/include/dbus-1.0 \
       -I /usr/lib/dbus-1.0/include
LIBS = -L /usr/lib \
       -l X11 \
       -l asound \
       -l procps \
       -l nl-3 \
       -l nl-genl-3 \
	   -l m \
	   -l netlink \
	   -l mnl \
	   -l curl \
	   -l cjson

all:
	@gcc ${INCS} -o dwm-statusbar dwm-statusbar.c ${LIBS}
	@echo build complete
