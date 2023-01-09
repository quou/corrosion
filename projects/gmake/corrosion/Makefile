ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

.PHONY: all clean emscripten

cc = gcc
includes = -I$(VULKAN_SDK)/include -I../../../corrosion/include/corrosion -I../../../corrosion/src
target_name = cr
deps = 
srcdir = ../../../corrosion/src
defines = -Dcr_no_dx12
cflags = -std=gnu11

bir_builder = $(target_dir)/bir
bir_target = ../../../corrosion/src/bir.h

shader_targetdir = ../../../corrosion/bir/
shader_targets = $(shaders:$(srcdir)%.glsl=$(shader_targetdir)%.csh)
shader_compiler = ../shadercompiler/bin/$(config)/shadercompiler

deps += ../shadercompiler

sources =                            \
          $(srcdir)/alloc.c          \
          $(srcdir)/atlas.c          \
          $(srcdir)/bir.c            \
          $(srcdir)/core.c           \
          $(srcdir)/dtable.c         \
          $(srcdir)/font.c           \
          $(srcdir)/gizmo.c          \
          $(srcdir)/log.c            \
          $(srcdir)/render_util.c    \
          $(srcdir)/res.c            \
          $(srcdir)/res_posix.c      \
          $(srcdir)/simplerenderer.c \
          $(srcdir)/stb.c            \
          $(srcdir)/thread_posix.c   \
          $(srcdir)/timer_posix.c    \
          $(srcdir)/ui.c             \
          $(srcdir)/ui_render.c      \
          $(srcdir)/video.c          \
          $(srcdir)/video_common.c   \
          $(srcdir)/video_gl.c       \
          $(srcdir)/video_vk.c       \
          $(srcdir)/window_common.c  \
          $(srcdir)/window_x11.c     \

ifeq ($(config),debug)
  target_dir = bin/debug
  target = $(target_dir)/lib$(target_name).a
  defines += -Ddebug
  cflags += -MMD -MP -m64 -g $(includes) $(defines)
  objdir = obj/debug
endif

ifeq ($(config),release)
  target_dir = bin/release
  target = $(target_dir)/lib$(target_name).a
  defines += -Dndebug
  cflags += -MMD -MP -m64 -O3 -s $(includes) $(defines)
  objdir = obj/release
endif

all: $(target)

shaders =                       \
          $(srcdir)/line.glsl   \
          $(srcdir)/line2d.glsl \
          $(srcdir)/ui.glsl     \
          $(srcdir)/simple.glsl \
          $(srcdir)/error.glsl \

objects   = $(sources:$(srcdir)%.c=$(objdir)%.o)

$(objects):   | $(objdir) $(bir_target)

$(objects): $(objdir)%.o : $(srcdir)%.c
	@echo $(notdir $<)
	$(silent) $(cc) $(cflags) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

$(shader_targets): $(shader_targetdir)%.csh : $(srcdir)%.glsl | $(shader_targetdir) $(deps)
	@echo $(notdir $<)
	$(silent) ./$(shader_compiler) $< $@

$(shader_targetdir):
	$(silent) mkdir -p $(shader_targetdir)

$(target): $(deps) $(objects) | $(target_dir)
	@echo Linking $(target)
	$(silent) ar -rcs "$@" $(objects)

$(bir_builder): | $(target_dir)
$(bir_builder): $(srcdir)/buildbir.c
	@echo "Compiling BIR builder..."
	$(silent) $(cc) -I../../../corrosion/include/corrosion -O2 $< -L/usr/lib64 -m64 -s -o "$@"

$(bir_target): | $(bir_builder) $(shader_targets)
	@echo "Building BIR..."
	$(silent) cd ../../../ && ./projects/gmake/corrosion/$(bir_builder)

$(target_dir):
	$(silent) mkdir -p $(target_dir)

$(objdir):
	$(silent) mkdir -p $(objdir)

$(deps):
	$(silent) make --no-print-directory -C "$@" -f Makefile config=$(config)

emscripten: $(target)
	$(silent) make --no-print-directory -f emscripten.make config=$(config)

clean:
	$(silent) rm -rf obj
	$(silent) rm -rf bin
	$(silent) rm -f $(bir_target)
	$(silent) rm -f $(shader_targets)

-include $(objects:%.o=%.d)
