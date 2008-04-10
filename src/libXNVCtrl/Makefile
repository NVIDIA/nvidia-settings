RANLIB ?= ranlib

libXNVCtrl.a : libXNVCtrl.a(NVCtrl.o)
	$(RANLIB) $@

NVCtrl.o : NVCtrl.h nv_control.h NVCtrlLib.h
.INTERMEDIATE: NVCtrl.o

clean ::
	rm -f libXNVCtrl.a *.o
.PHONY: clean
