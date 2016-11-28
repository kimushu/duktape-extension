SRCDIR = src-separate
DESTDIR = src
COMBINE = python tools/combine_src.py

TARGETS = $(addprefix $(DESTDIR)/dux_all.,c h)
SOURCES = \
	dux_basis.c \
	node/dux_process.c \
	node/dux_util.c \
	altera_hal/dux_timer_alt.c
PREAMBLE = $(SRCDIR)/dux_preamble.h

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS)

$(DESTDIR)/dux_all.c: $(PREAMBLE) $(addprefix $(SRCDIR)/,$(SOURCES))
	mkdir -p $(dir $@)
	$(COMBINE) -C $(SRCDIR) -l --header $(abspath $<) -e duktape.h $(SOURCES) > $@ || (rm -f $@; false)

$(DESTDIR)/dux_all.h: $(PREAMBLE) $(SRCDIR)/dux_all.h
	mkdir -p $(dir $@)
	cat $^ > $@ || (rm -f $@; false)


