# Makefile - compile libcurl.a for the Nintendo Switch target.
#
# Build with devkitPro (devkitA64) in WSL:
#   make -j20
#
# Produces out/libcurl.a and the public headers under out/include/ (curl + nn).

DEVKITPRO ?= /opt/devkitpro
DEVKITA64 := $(DEVKITPRO)/devkitA64
PREFIX    := $(DEVKITA64)/bin/aarch64-none-elf-

CC  := $(PREFIX)gcc
CXX := $(PREFIX)g++
AR  := $(PREFIX)ar

ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIC -fvisibility=hidden

INCLUDES := -Iinclude -Ilib -Ilib/vtls
DEFINES  := -DHAVE_CONFIG_H -DBUILDING_LIBCURL -DCURL_STATICLIB

OPT  := -O2 -ffunction-sections -fdata-sections
WARN := -Wall -Wno-implicit-fallthrough -Wno-unused-parameter
DEP  := -MMD -MP

CFLAGS   := $(ARCH) $(OPT) $(WARN) $(DEP) $(DEFINES) $(INCLUDES) -std=gnu11
CXXFLAGS := $(ARCH) $(OPT) $(WARN) $(DEP) $(DEFINES) $(INCLUDES) -std=gnu++17 \
            -fno-rtti -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables

CSRC := $(wildcard lib/*.c) \
        $(wildcard lib/vauth/*.c) \
        $(wildcard lib/vtls/*.c) \
        $(wildcard lib/vquic/*.c) \
        $(wildcard lib/vssh/*.c)

CXXSRC := lib/nntime.cpp

OUT    := out
BUILD  := $(OUT)/obj
COBJ   := $(patsubst lib/%.c,$(BUILD)/%.o,$(CSRC))
CXXOBJ := $(patsubst lib/%.cpp,$(BUILD)/%.o,$(CXXSRC))
OBJ    := $(COBJ) $(CXXOBJ)

TARGET := $(OUT)/libcurl.a
INCDIR := $(OUT)/include

# Public headers shipped alongside libcurl.a: the curl API plus the minimal nn
# shims. The BSD-compat headers (sys/, netinet/, arpa/, ...) are build-only and
# intentionally not published.
PUBHDRS := curl nn

all: $(TARGET) headers

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $(OBJ)
	@echo "==> Built $@"

$(BUILD)/%.o: lib/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Stage the public headers into $(INCDIR). Remove first so re-runs don't nest
# (cp -r of a dir into an existing dir would create dir/dir).
headers:
	@mkdir -p $(INCDIR)
	@rm -rf $(foreach d,$(PUBHDRS),$(INCDIR)/$(d))
	@cp -r $(foreach d,$(PUBHDRS),include/$(d)) $(INCDIR)/
	@echo "Copied public headers to $(INCDIR)"

clean:
	rm -rf $(OUT)

-include $(OBJ:.o=.d)

.PHONY: all clean headers
