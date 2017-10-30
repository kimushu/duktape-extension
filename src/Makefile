DESTDIR = ../dist
COMBINE = python ../tools/combine_src.py

TARGETS = $(addprefix $(DESTDIR)/,dukext.c dukext.h dukext.d.ts)
SOURCES = \
	dux_basis.c \
	dux_modules.c \
	dux_thrpool.c \
	dux_work.c \
	dux_promise_simplified.c \
	node/dux_node.c \
		node/dux_events.c \
		node/dux_process.c \
		node/dux_util.c \
		node/dux_path.c \
		node/dux_console.c \
		node/dux_timer.c \
			altera_hal/dux_timer_alt.c \
			linux/dux_timer_linux.c \
			win32/dux_timer_win32.c \
		node/dux_immediate.c \
	hw/dux_hardware.c \
		altera_hal/dux_hardware_io_alt.c \
		pc/dux_hardware_io_pc.c \
		hw/dux_paraio.c \
		hw/dux_i2ccon.c \
		hw/dux_spicon.c \
	peridot/dux_peridot.c \
		peridot/dux_peridot_gpio.c \
		peridot/dux_peridot_i2c.c \
		peridot/dux_peridot_spi.c \
		peridot/dux_peridot_servo.c \
	packages/delay.c \
	packages/sprintf.c \

PREAMBLE = dux_preamble.h
DEP = .dukext.dep

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS) $(DEP)

.PHONY: test
test: $(TARGETS)
	make -C ../test

.PRECIOUS: $(TARGETS)

$(DESTDIR)/dukext.c: $(PREAMBLE) $(SOURCES) $(MAKEFILE_LIST)
	mkdir -p $(dir $@)
	$(COMBINE) --line --header $< \
		-e duktape.h -e dux_config.h -o $@ --dep $(DEP) $(SOURCES) || (rm -f $@; false)

-include $(DEP)

$(DESTDIR)/dukext.h: $(PREAMBLE) dukext.h
	mkdir -p $(dir $@)
	cat $^ > $@ || (rm -f $@; false)

$(DESTDIR)/dukext.d.ts: $(wildcard $(SOURCES:.c=.d.ts))
	cat $^ > $@
