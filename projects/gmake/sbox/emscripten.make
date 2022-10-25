ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

.phony: clean

cc = emcc
cxx = em++
includes = -I../../../corrosion/include
target_name = sbox
defines = -Dcr_no_vulkan
libs = ../corrosion/bin/$(config)/emscripten/libcr.bc
deps = 
opts = -pthread
srcdir = ../../../sbox/src
cstd = -std=c11
cxxstd = -std=c++20

ifeq ($(config),debug)
  targetdir = bin/debug/emscripten
  target = $(targetdir)/$(target_name).html
  lflags = -mwasm64 -g
  defines += -Ddebug
  cflags = -MMD -MP -g $(cstd) $(includes) $(defines) $(opts)
  cxxflags = -MMD -MP -g $(cxxstd) $(includes) $(defines) $(opts)
  objdir = obj
endif

ifeq ($(config),release)
  targetdir = bin/release
  target = $(targetdir)/$(target_name).html
  lflags = -mwasm64 -s
  defined += -Dndebug
  cflags = -MMD -MP -O2 $(cstd) $(includes) $(defines) $(opts)
  cxxflags = -MMD -MP -O2 $(cxxstd) $(includes) $(defines) $(opts)
  objdir = obj/release
endif

all: $(target)
	@:

sources = $(wildcard $(srcdir)/*.c $(srcdir)/*/*.c)
objects = $(sources:$(srcdir)/%.c=$(objdir)/%.o)

shaders = $(wildcard $(srcdir)/*.glsl $(srcdir)/*/*.glsl)

shader_targetdir = ../../../sbox/shaders/
shader_targets = $(shaders:$(srcdir)%.glsl=$(shader_targetdir)%.csh)
shader_compiler = ../shadercompiler/bin/$(config)/shadercompiler

$(objects): | $(objdir)

$(shader_targets): $(shader_targetdir)%.csh : $(srcdir)%.glsl | $(shader_targetdir)
	@echo $(notdir $<)
	$(silent) ./$(shader_compiler) $< $@

$(objects): $(objdir)/%.o : $(srcdir)/%.c
	@echo $(notdir $<)
	$(silent) $(cc) $(cflags) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

$(target): $(objects) $(objectsxx) $(deps) | $(targetdir)
	@echo linking $(target)
	$(silent) $(cxx) -o "$@" $(objects) $(lflags) $(libs)

$(targetdir):
	$(silent) mkdir -p $(targetdir)

$(objdir):
	$(silent) mkdir -p $(objdir)

clean:
	$(silent) rm -rf obj
	$(silent) rm -rf bin
	$(silent) rm -rf tags

-include $(objects:%.o=%.d)
-include $(objectsxx:%.opp=%.dpp)
