CFLAGS := -Wall -g -O3
LDFLAGS :=

PKGS := libwnck-1.0 gtk+-2.0 gmodule-2.0 x11

CFLAGS += `pkg-config --cflags $(PKGS)`
LDFLAGS += `pkg-config --libs $(PKGS)`

sr-ts: sr-ts.c sr-ts-glade.c

sr-ts-glade.c: sr-ts.glade convert-glade
	./convert-glade $< $@

.PHONY: clean

clean:
	-rm sr-ts
