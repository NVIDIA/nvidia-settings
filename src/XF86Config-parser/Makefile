SRC = \
	DRI.c \
	Device.c \
	Files.c \
	Flags.c \
	Input.c \
	Keyboard.c \
	Layout.c \
	Module.c \
	Monitor.c \
	Pointer.c \
	Screen.c \
	Vendor.c \
	Video.c \
	Read.c \
	Scan.c \
	Write.c \
	Util.c \
	Extensions.c \
	Generate.c

OBJS = $(SRC:%.c=%.o)
DEPS = $(SRC:%.c=%.d)

ifndef CC
  CC = gcc
endif

CFLAGS = -Wall -g

ifdef NV_CFLAGS
  CFLAGS += $(NV_CFLAGS)
endif

ifndef RANLIB
  RANLIB = ranlib
endif

LIB = libXF86Config-parser.a
LIB_O = $(LIB:%.a=%.o)

default all: $(LIB)

.PHONY: clean clobber

$(LIB): $(OBJS)
	$(LD) -r -o $(LIB_O) $(OBJS)
	$(AR) ruv $(LIB) $(LIB_O)
	$(RANLIB) $(LIB)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.d: %.c
	@set -e; $(CC) -MM $(CPPFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
		[ -s $@ ] || rm -f $@

clean clobber:
	rm -rf *.o *~ *.d $(LIB) $(LIB_O)

-include $(DEPS)
