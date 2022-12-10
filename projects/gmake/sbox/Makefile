ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

.PHONY: all run clean

cc = gcc
includes = -I../../../corrosion/include
target_name = sbox
deps =
srcdir = ../../../sbox/src
libs = -lm -lX11 -lXi -lvulkan -lGL -lGLX -lpthread
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
  cflags = -MMD -MP -m64 -O3 $(includes) $(defines)
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
	valgrind --leak-check=no ./../projects/gmake/sbox/$(target)

sources = $(wildcard $(srcdir)/*.c $(srcdir)/*/*.c)
objects = $(sources:$(srcdir)/%.c=$(objdir)/%.o)

shaders = $(wildcard $(srcdir)/*.glsl $(srcdir)/*/*.glsl)

shader_targetdir = ../../../sbox/shaders/
shader_targets = $(shaders:$(srcdir)%.glsl=$(shader_targetdir)%.csh)
shader_compiler = ../shadercompiler/bin/$(config)/shadercompiler

$(objects): | $(shader_targets) $(objdir)

$(objects): $(objdir)%.o : $(srcdir)%.c
	@echo $(notdir $<)
	$(silent) $(cc) $(cflags) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

$(target): $(objects) $(deps) | $(target_dir)
	@echo Linking $(target)
	$(silent) $(cc) -o "$@" $(objects) $(lflags) $(libs)

$(shader_targets): $(shader_targetdir)%.csh : $(srcdir)%.glsl | $(shader_targetdir)
	@echo $(notdir $<)
	$(silent) ./$(shader_compiler) $< $@

$(deps):
	$(silent) make --no-print-directory -C "$@" -f Makefile config=$(config)

$(target_dir):
	$(silent) mkdir -p $(target_dir)

$(shader_targetdir):
	$(silent) mkdir -p $(shader_targetdir)

$(objdir):
	$(silent) mkdir -p $(objdir)

emscripten: $(target)
	$(silent) make -f emscripten.make

clean:
	$(silent) rm -rf obj
	$(silent) rm -rf bin
	$(silent) rm -f $(shader_targets)

-include $(objects:%.o=%.d)
