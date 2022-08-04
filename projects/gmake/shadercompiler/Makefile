ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

.PHONY: all run clean

cc = gcc
cpp = g++
includes = -I../../../corrosion/include -I../../../corrosion/include/corrosion
target_name = shadercompiler
deps =
srcdir = ../../../shadercompiler/src
libs = -lshaderc_shared
defines =

ifeq ($(config),debug)
  target_dir = bin/debug
  target = $(target_dir)/$(target_name)
  defines += -Ddebug
  libs +=
  deps +=
  lflags = -L/usr/lib64 -m64 -g
  cflags = -MMD -MP -m64 -g $(includes) $(defines)
  objdir = obj/debug
endif

ifeq ($(config),release)
  target_dir = bin/release
  target = $(target_dir)/$(target_name)
  defines += -Dndebug
  libs +=
  deps +=
  lflags = -L/usr/lib64 -m64 -s
  cflags = -MMD -MP -m64 -O2 $(includes) $(defines)
  objdir = obj/release
endif

all: $(deps) $(target)

sources = $(wildcard $(srcdir)/*.c $(srcdir/*/*.c))
objects = $(sources:$(srcdir)/%.c=$(objdir)/%.o)

sourcespp = $(wildcard $(srcdir)/*.cpp $(srcdir/*/*.cpp))
objectspp = $(sourcespp:$(srcdir)/%.cpp=$(objdir)/%.o)

$(objects): | $(objdir)
$(objectspp): | $(objdir)

$(objects): $(objdir)%.o : $(srcdir)%.c
	@echo $(notdir $<)
	$(silent) $(cc) $(cflags) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

$(objectspp): $(objdir)%.o : $(srcdir)%.cpp
	@echo $(notdir $<)
	$(silent) $(cpp) $(cflags) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

$(target): $(objects) $(objectspp) $(deps) | $(target_dir)
	@echo Linking $(target)
	$(silent) $(cpp) -o "$@" $(objects) $(objectspp) $(lflags) $(libs)

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
