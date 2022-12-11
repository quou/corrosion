ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

.phony: clean

cc = emcc
includes = -I../../../../corrosion/include
target_name = ui
defines = -Dcr_no_vulkan -Wno-unreachable-code-generic-assoc
libs = ../../corrosion/bin/$(config)/emscripten/libcr.bc -sMAX_WEBGL_VERSION=2 --shell-file ../../../../corrosion/ems_shell.html --post-js ../../../../corrosion/post_run.js
deps = ../../corrosion/bin/$(config)/emscripten/libcr.bc
opts = -pthread
srcdir = ../../../../demos/ui/src
cstd = -std=c11
package_files = --preload-file ../../../../demos/ui/stylesheet.dt@stylesheet.dt

ifeq ($(config),debug)
  targetdir = bin/debug/emscripten
  target = $(targetdir)/$(target_name).html
  lflags = -mwasm64 -g
  defines += -Ddebug
  cflags = -MMD -MP -g $(cstd) $(includes) $(defines) $(opts)
  objdir = obj/debug/emscripten
endif

ifeq ($(config),release)
  targetdir = bin/release/emscripten
  target = $(targetdir)/$(target_name).html
  lflags = -mwasm64 -s
  defined += -Dndebug
  cflags = -MMD -MP -O2 $(cstd) $(includes) $(defines) $(opts)
  objdir = obj/release/emscripten
endif

all: $(target)
	@:

sources = $(wildcard $(srcdir)/*.c $(srcdir)/*/*.c)
objects = $(sources:$(srcdir)/%.c=$(objdir)/%.o)

$(objects): | $(objdir)

$(objects): $(objdir)/%.o : $(srcdir)/%.c
	@echo $(notdir $<)
	$(silent) $(cc) $(cflags) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

$(target): $(objects) $(deps) | $(targetdir)
	@echo linking $(target)
	$(silent) $(cc) -o "$@" $(objects) $(lflags) $(libs) $(package_files)

$(targetdir):
	$(silent) mkdir -p $(targetdir)

$(objdir):
	$(silent) mkdir -p $(objdir)

clean:
	$(silent) rm -rf obj
	$(silent) rm -rf bin
	$(silent) rm -rf tags

-include $(objects:%.o=%.d)
