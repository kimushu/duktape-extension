DUK_VERSION = 2.1.0

DUK_ARCHIVE = downloads/duktape-$(DUK_VERSION).tar.xz
DUK_URL = http://duktape.org/duktape-$(DUK_VERSION).tar.xz
DUK_DIR = duktape-$(DUK_VERSION)/src
DUK_SOURCE = $(DUK_DIR)/duktape.c

DUX_DIR = ../dist
DUX_SOURCE = $(DUX_DIR)/dukext.c

ifeq ($(OS),Windows_NT)
TARGET = tester_host.exe
else
TARGET = tester_host
endif

C_SOURCES = tester_host.c espresso.c

CFLAGS = -DDUX_ENABLE_PACKAGE_DELAY -DDUX_ENABLE_PACKAGE_SPRINTF
LIBS = -lm -pthread

ifeq ($(OS),Windows_NT)
LIBS += -lwinmm
endif

.PHONY: test
test: $(TARGET) tsc
	./$< $(wildcard out/*.js) $(wildcard out/**/*.js)

.PHONY: tsc
tsc:
	mkdir -p typings/globals/dukext
	cp $(DUX_DIR)/dukext.d.ts typings/globals/dukext/index.d.ts
	$(shell npm bin)/tsc -p .

.PHONY: dux
dux:
	$(MAKE) -C ../src

$(TARGET): $(DUK_SOURCE) $(DUX_SOURCE) $(C_SOURCES)
	gcc -o $@ -I. -I$(DUK_DIR) -I$(DUX_DIR) $(CFLAGS) $^ $(LIBS)
	nm $@ > $@.map

.PHONY: clean
clean:
	rm -f $(TARGET)
	rm -rf out

$(DUK_SOURCE): $(DUK_ARCHIVE)
	tar xf $< $(dir $(DUK_SOURCE))
	touch $@

$(DUK_ARCHIVE):
	mkdir -p $(@D)
	curl -o $@ $(DUK_URL)
