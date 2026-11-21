# Native host tests for OnyxDesktop modules.
#
# These tests compile the portable C99 modules with plain gcc - no
# OnyxOS, onyxcc or RISC-V toolchain needed. Full .onx builds and ONX1
# validation stay in CI (.github/workflows/ci.yml).
#
#   make test            build and run every test
#   make test_harness    build one test binary
#   make clean
#
# Layout: max 4 files per folder. Test binaries stay untracked (see the
# repo .gitignore).

CC      ?= gcc
CFLAGS  ?= -std=gnu11 -Wall -Wextra -Wno-unused-parameter
ROOT    := ../..

.PHONY: test clean

test: test_harness config/test_config config/test_binds ui/test_ui_panel ui/test_ui_launcher ui/test_ui_edges desk/test_desk desk/test_desk_events desk/test_desk_fb kbd/test_kbd bmp/test_bmp bmp/test_photo bar/test_bar
	./test_harness
	./config/test_config
	./config/test_binds
	./ui/test_ui_panel
	./ui/test_ui_launcher
	./ui/test_ui_edges
	./desk/test_desk
	./desk/test_desk_events
	./desk/test_desk_fb
	./kbd/test_kbd
	./bmp/test_bmp
	./bmp/test_photo
	./bar/test_bar

test_harness: test_harness.c
	$(CC) $(CFLAGS) -o $@ $<

CONFIG_SRCS := $(ROOT)/src/config/config.c $(ROOT)/src/config/bind.c

config/test_config: config/test_config.c $(CONFIG_SRCS) $(ROOT)/src/config/config.h
	$(CC) $(CFLAGS) -I $(ROOT)/src -o $@ config/test_config.c $(CONFIG_SRCS)

config/test_binds: config/test_binds.c $(CONFIG_SRCS) $(ROOT)/src/config/config.h $(ROOT)/src/config/bind.h
	$(CC) $(CFLAGS) -I $(ROOT)/src -o $@ config/test_binds.c $(CONFIG_SRCS)

UI_SRCS := $(ROOT)/src/ui.c $(ROOT)/src/font/font.c $(CONFIG_SRCS)
UI_HDRS := $(ROOT)/src/ui.h $(ROOT)/src/font/font.h $(ROOT)/src/config/config.h

ui/test_ui_panel: ui/test_ui_panel.c ui/test_ui_util.h $(UI_SRCS) $(UI_HDRS)
	$(CC) $(CFLAGS) -I $(ROOT)/src -o $@ ui/test_ui_panel.c $(UI_SRCS)

ui/test_ui_launcher: ui/test_ui_launcher.c ui/test_ui_util.h $(UI_SRCS) $(UI_HDRS)
	$(CC) $(CFLAGS) -I $(ROOT)/src -o $@ ui/test_ui_launcher.c $(UI_SRCS)

ui/test_ui_edges: ui/test_ui_edges.c $(UI_SRCS) $(UI_HDRS)
	$(CC) $(CFLAGS) -I $(ROOT)/src -o $@ ui/test_ui_edges.c $(UI_SRCS)

DESK_SRCS := $(ROOT)/src/desk/desk.c $(ROOT)/src/desk/io.c $(ROOT)/src/desk/kbd/kbd.c $(UI_SRCS)
DESK_HDRS := $(ROOT)/src/desk/desk.h $(ROOT)/src/desk/kbd/kbd.h $(UI_HDRS)

desk/test_desk: desk/test_desk.c desk/fake_desk.h $(DESK_SRCS) $(DESK_HDRS)
	$(CC) $(CFLAGS) -I $(ROOT)/src -o $@ desk/test_desk.c $(DESK_SRCS)

desk/test_desk_events: desk/test_desk_events.c desk/fake_desk.h $(DESK_SRCS) $(DESK_HDRS)
	$(CC) $(CFLAGS) -I $(ROOT)/src -o $@ desk/test_desk_events.c $(DESK_SRCS)

desk/test_desk_fb: desk/test_desk_fb.c desk/fake_desk.h $(DESK_SRCS) $(DESK_HDRS)
	$(CC) $(CFLAGS) -I $(ROOT)/src -o $@ desk/test_desk_fb.c $(DESK_SRCS)

kbd/test_kbd: kbd/test_kbd.c kbd/fake_kbd.h desk/fake_desk.h $(DESK_SRCS) $(DESK_HDRS)
	$(CC) $(CFLAGS) -I $(ROOT)/src -o $@ kbd/test_kbd.c $(DESK_SRCS)

WIDGET_HDRS := $(ROOT)/src/widgets/bmp.h $(ROOT)/src/widgets/photo.h
BMP_SRCS := $(ROOT)/src/widgets/bmp.c $(UI_SRCS)
PHOTO_SRCS := $(ROOT)/src/widgets/bmp.c $(ROOT)/src/widgets/photo.c $(UI_SRCS)

bmp/test_bmp: bmp/test_bmp.c bmp/bmp_gen.h $(BMP_SRCS) $(UI_HDRS) $(WIDGET_HDRS)
	$(CC) $(CFLAGS) -I $(ROOT)/src -o $@ bmp/test_bmp.c $(BMP_SRCS)

bmp/test_photo: bmp/test_photo.c bmp/bmp_gen.h bmp/bmp_fake.h $(PHOTO_SRCS) $(UI_HDRS) $(WIDGET_HDRS)
	$(CC) $(CFLAGS) -I $(ROOT)/src -o $@ bmp/test_photo.c $(PHOTO_SRCS)

BAR_SRCS := $(ROOT)/src/widgets/bar/bar.c

bar/test_bar: bar/test_bar.c bar/fake_bar.h $(BAR_SRCS) $(ROOT)/src/widgets/bar/bar.h $(DESK_HDRS) $(UI_HDRS)
	$(CC) $(CFLAGS) -I $(ROOT)/src -o $@ bar/test_bar.c $(BAR_SRCS) $(DESK_SRCS)

clean:
	rm -f test_harness config/test_config config/test_binds ui/test_ui_panel ui/test_ui_launcher ui/test_ui_edges desk/test_desk desk/test_desk_events desk/test_desk_fb kbd/test_kbd bmp/test_bmp bmp/test_photo bar/test_bar
