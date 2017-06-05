DESTDIR = ../dist
COMBINE = python ../tools/combine_src.py

TARGETS = $(addprefix $(DESTDIR)/dux_all.,c h)
SOURCES = \
	dux_basis.c \
	dux_modules.c \
	dux_thrpool.c \
	dux_promise_simplified.c \
	node/dux_process.c \
	node/dux_util.c \
	node/dux_console.c \
	node/dux_timer.c \
		altera_hal/dux_timer_alt.c \
		linux/dux_timer_linux.c \
		win32/dux_timer_win32.c \
	altera_hal/dux_hardware_io_alt.c \
	pc/dux_hardware_io_pc.c \
	hw/dux_paraio.c \
	hw/dux_i2ccon.c \
	hw/dux_spicon.c \
	peridot/dux_peridot.c \
		peridot/dux_peridot_gpio.c \
		peridot/dux_peridot_i2c.c \
		peridot/dux_peridot_spi.c \
		peridot/dux_peridot_servo.c

PREAMBLE = dux_preamble.h
DEP = .dux_all.dep

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS) $(DEP)

.PHONY: test
test: $(TARGETS) typings
	make -C ../test

.PHONY: typings
typings: $(wildcard $(SOURCES:.c=.d.ts))
	cp --parents --target-directory ../test/typings/globals/dux $^

.PRECIOUS: $(TARGETS)

$(DESTDIR)/dux_all.c: $(PREAMBLE) $(SOURCES) $(MAKEFILE_LIST)
	mkdir -p $(dir $@)
	$(COMBINE) --line --header $< \
		-e duktape.h -o $@ --dep $(DEP) $(SOURCES) || (rm -f $@; false)

-include $(DEP)

$(DESTDIR)/dux_all.h: $(PREAMBLE) dux_all.h
	mkdir -p $(dir $@)
	cat $^ > $@ || (rm -f $@; false)

