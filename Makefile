CFLAGS := -Wall -g -O3
LDFLAGS :=

PKGS := libwnck-1.0 gtk+-2.0 gmodule-2.0

CFLAGS += `pkg-config --cflags $(PKGS)`
LDFLAGS += `pkg-config --libs $(PKGS)`

sr-ts: sr-ts.c

.PHONY: clean

clean:
	-rm sr-ts
