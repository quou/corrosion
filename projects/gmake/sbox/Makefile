ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

.PHONY: all run clean

cc = gcc
cpp = g++
includes = -I../../../corrosion/include
target_name = sbox
deps =
srcdir = ../../../sbox/src
libs = -lm -lX11 -lvulkan
defines =

ifeq ($(config),debug)
  target_dir = bin/debug
  target = $(target_dir)/$(target_name)
  defines += -Ddebug
  libs += ../corrosion/bin/debug/libcr.a
  deps += ../corrosion/bin/debug/libcr.a
  lflags = -L/usr/lib64 -m64 -g
  cflags = -MMD -MP -m64 -g $(includes) $(defines)
  objdir = obj/debug
endif

ifeq ($(config),release)
  target_dir = bin/release
  target = $(target_dir)/$(target_name)
  defines += -Dndebug
  libs += ../corrosion/bin/release/libcr.a
  deps += ../corrosion/bin/release/libcr.a
  lflags = -L/usr/lib64 -m64 -s
  cflags = -MMD -MP -m64 -O2 $(includes) $(defines)
  objdir = obj/release
endif

all: $(deps) $(target)

run: | $(deps) $(target)
	@echo ==
	$(silent) cd ../../../sbox; \
	./../projects/gmake/sbox/$(target)

debug: | $(deps) $(target)
	@echo ==
	$(silent) cd ../../../sbox; \
	gdb ./../projects/gmake/sbox/$(target)

memcheck: | $(deps) $(target)
	@echo ==
	$(silent) cd ../../../sbox; \
	valgrind --leak-check=full ./../projects/gmake/sbox/$(target)

sources = $(wildcard $(srcdir)/*.c $(srcdir/*/*.c))
objects = $(sources:$(srcdir)/%.c=$(objdir)/%.o)

$(objects): | $(objdir)

$(objects): $(objdir)%.o : $(srcdir)%.c
	@echo $(notdir $<)
	$(silent) $(cc) $(cflags) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

$(target): $(objects) $(deps) | $(target_dir)
	@echo Linking $(target)
	$(silent) $(cpp) -o "$@" $(objects) $(lflags) $(libs)

$(deps):
	$(silent) make --no-print-directory -C "$@" -f Makefile config=$(config)

$(target_dir):
	$(silent) mkdir -p $(target_dir)

$(objdir):
	$(silent) mkdir -p $(objdir)

clean:
	$(silent) rm -rf obj
	$(silent) rm -rf bin

-include $(objects:%.o=%.d)
