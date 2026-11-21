# OnyxDesktop - build the desktop daemon with OnyxCC.
#
#   make                       build build/onyxdesk.onx
#   make ONYXCC=...            custom compiler path
#   make test                  host tests (tests/native)
#   make clean
#
# onyxcc links libonyxc automatically; only src/ goes on the include path
# so the app can include its own headers. onyxdesk is built --ring1: it
# is gated on SYS_spawn/SYS_kill and owns the framebuffer session for the
# whole run (see ../OnyxOS/docs/dev/onyxdesktop-design.md).
#
# Layout rules (project-wide): max 4 files per folder, max 200 lines per
# file, one responsibility per file.

ONYXCC      ?= onyxcc
ONYXCC_FILE := $(shell command -v "$(ONYXCC)" 2>/dev/null)
SRCS        := $(sort $(wildcard src/*.c src/*/*.c src/*/*/*.c))
HDRS        := $(sort $(wildcard src/*.h src/*/*.h src/*/*/*.h))

.PHONY: all test clean

all: build/onyxdesk.onx

build/onyxdesk.onx: $(SRCS) $(HDRS) Makefile $(ONYXCC_FILE)
	@mkdir -p build
	$(ONYXCC) --ring1 -I src -o $@ $(SRCS)

test:
	$(MAKE) -C tests/native test

clean:
	rm -rf build
