DUK_VERSION = 2.1.0

DUK_ARCHIVE = downloads/duktape-$(DUK_VERSION).tar.xz
DUK_URL = http://duktape.org/duktape-$(DUK_VERSION).tar.xz
DUK_DIR = duktape-$(DUK_VERSION)/src
DUK_SOURCE = $(DUK_DIR)/duktape.c

DUX_DIR = ../dist
DUX_SOURCE = $(DUX_DIR)/dux_all.c

CFLAGS =
LIBS = -lm -pthread

ifeq ($(OS),Windows_NT)
LIBS += -lwinmm
endif

.PHONY: test
test: tester_host tsc
	./$< $(wildcard out/**/*.js)

.PHONY: tsc
tsc:
	$(shell npm bin)/tsc -p .

tester_host: $(DUK_SOURCE) $(DUX_SOURCE) tester_host.c espresso.c
	gcc -o $@ -I $(DUK_DIR) -I $(DUX_DIR) $(CFLAGS) $^ $(LIBS)

.PHONY: clean
clean:
	rm -f tester_host

$(DUK_SOURCE): $(DUK_ARCHIVE)
	tar xf $< $(dir $(DUK_SOURCE))
	touch $@

$(DUK_ARCHIVE):
	mkdir -p $(@D)
	curl -o $@ $(DUK_URL)

