ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

.PHONY: all run clean

cc = gcc
cpp = g++
includes = -I../../../../corrosion/include
target_name = shooter
deps =
srcdir = ../../../../games/shooter/src
libs = -lm -lX11 -lvulkan
defines =

ifeq ($(config),debug)
  target_dir = bin/debug
  target = $(target_dir)/$(target_name)
  defines += -Ddebug
  libs += ../../corrosion/bin/debug/libcr.a
  deps += ../../corrosion/bin/debug/libcr.a
  lflags = -L/usr/lib64 -m64 -g
  cflags = -MMD -MP -m64 -g $(includes) $(defines)
  objdir = obj/debug
endif

ifeq ($(config),release)
  target_dir = bin/release
  target = $(target_dir)/$(target_name)
  defines += -Dndebug
  libs += ../../corrosion/bin/release/libcr.a
  deps += ../../corrosion/bin/release/libcr.a
  lflags = -L/usr/lib64 -m64 -s
  cflags = -MMD -MP -m64 -O2 $(includes) $(defines)
  objdir = obj/release
endif

all: $(deps) $(target)

run: | $(deps) $(target)
	@echo ==
	$(silent) cd ../../../../games/shooter; \
	./../../projects/gmake/games/shooter/$(target)

debug: | $(deps) $(target)
	@echo ==
	$(silent) cd ../../../../games/shooter; \
	gdb ./../../projects/gmake/games/shooter/$(target)

memcheck: | $(deps) $(target)
	@echo ==
	$(silent) cd ../../../../games/shooter; \
	valgrind --leak-check=full ./../../projects/gmake/games/shooter/$(target)

sources = $(wildcard $(srcdir)/*.c $(srcdir/*/*.c))
objects = $(sources:$(srcdir)/%.c=$(objdir)/%.o)

shaders = $(wildcard $(srcdir)/*.glsl $(srcdir/*/*.glsl))

shader_targetdir = ../../../../games/shooter/shaders/
shader_targets = $(shaders:$(srcdir)%.glsl=$(shader_targetdir)%.csh)
shader_compiler = ../../shadercompiler/bin/$(config)/shadercompiler

$(objects): | $(shader_targets) $(objdir)

$(objects): $(objdir)%.o : $(srcdir)%.c
	@echo $(notdir $<)
	$(silent) $(cc) $(cflags) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

$(target): $(objects) $(deps) | $(target_dir)
	@echo Linking $(target)
	$(silent) $(cpp) -o "$@" $(objects) $(lflags) $(libs)

$(shader_targets): $(shader_targetdir)%.csh : $(srcdir)%.glsl | $(shader_targetdir) $(objdir)
	@echo $(notdir $<)
	$(silent) ./$(shader_compiler) $< $(objdir) $@

$(deps):
	$(silent) make --no-print-directory -C "$@" -f Makefile config=$(config)

$(target_dir):
	$(silent) mkdir -p $(target_dir)

$(shader_targetdir):
	$(silent) mkdir -p $(shader_targetdir)

$(objdir):
	$(silent) mkdir -p $(objdir)

clean:
	$(silent) rm -rf obj
	$(silent) rm -rf bin
	$(silent) rm -f $(shader_targets)

-include $(objects:%.o=%.d)
