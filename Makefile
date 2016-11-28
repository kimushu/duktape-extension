SRCDIR = src-separate
DESTDIR = src
COMBINE = python tools/combine_src.py

TARGETS = $(addprefix $(DESTDIR)/dux_all.,c h)
SOURCES = $(addprefix $(SRCDIR)/,\
	dux_basis.c \
	dux_thrpool.c \
	node/dux_process.c \
	node/dux_util.c \
	altera_hal/dux_timer_alt.c \
	)
PREAMBLE = $(SRCDIR)/dux_preamble.h
DEP = .dux_all.dep

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS) $(DEP)

$(DESTDIR)/dux_all.c: $(PREAMBLE) $(SOURCES)
	mkdir -p $(dir $@)
	$(COMBINE) --line --header $< \
		-e duktape.h -o $@ --dep $(DEP) \
		$(SOURCES) || (rm -f $@; false)

-include $(DEP)

$(DESTDIR)/dux_all.h: $(PREAMBLE) $(SRCDIR)/dux_all.h
	mkdir -p $(dir $@)
	cat $^ > $@ || (rm -f $@; false)

