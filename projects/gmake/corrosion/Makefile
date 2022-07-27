ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

.PHONY: all clean

cc = gcc
cxx = g++
includes = -I../../../corrosion/include/corrosion -I../../../corrosion/src
target_name = cr
deps = 
srcdir = ../../../corrosion/src
defines =
cflags = -std=gnu11
cppflags = -std=c++14

bir_builder = $(target_dir)/bir
bir_target = ../../../corrosion/src/bir.h

ifeq ($(config),debug)
  target_dir = bin/debug
  target = $(target_dir)/lib$(target_name).a
  defines += -Ddebug
  cflags += -MMD -MP -m64 -g $(includes) $(defines)
  cppflags += -MMD -MP -m64 -g $(includes) $(defines)
  objdir = obj/debug
endif

ifeq ($(config),release)
  target_dir = bin/release
  target = $(target_dir)/lib$(target_name).a
  defines += -Dndebug
  cflags += -MMD -MP -m64 -O2 $(includes) $(defines)
  cppflags += -MMD -MP -m64 -O2 $(includes) $(defines)
  objdir = obj/release
endif

all: $(target)

sources =                            \
          $(srcdir)/alloc.c          \
          $(srcdir)/atlas.c          \
          $(srcdir)/bir.c            \
          $(srcdir)/core.c           \
          $(srcdir)/dtable.c         \
          $(srcdir)/entrypoint.c     \
          $(srcdir)/font.c           \
          $(srcdir)/gizmo.c          \
          $(srcdir)/log.c            \
          $(srcdir)/res.c            \
          $(srcdir)/res_posix.c      \
          $(srcdir)/simplerenderer.c \
          $(srcdir)/stb.c            \
          $(srcdir)/timer_posix.c    \
          $(srcdir)/ui.c             \
          $(srcdir)/ui_render.c      \
          $(srcdir)/video.c          \
          $(srcdir)/video_vk.c       \
          $(srcdir)/window_x11.c     \

shaders =                       \
          $(srcdir)/line.glsl   \
          $(srcdir)/ui.glsl     \
          $(srcdir)/simple.glsl \

shader_targetdir = ../../../corrosion/bir/
shader_targets = $(shaders:$(srcdir)%.glsl=$(shader_targetdir)%.csh)
shader_compiler = ../shadercompiler/bin/$(config)/shadercompiler

sourcespp = $(srcdir)/vma.cpp

objects   = $(sources:$(srcdir)%.c=$(objdir)%.o)
objectspp = $(sourcespp:$(srcdir)%.cpp=$(objdir)%.o)

$(objects):   | $(objdir) $(bir_target)
$(objectspp): | $(objdir)

$(objects): $(objdir)%.o : $(srcdir)%.c
	@echo $(notdir $<)
	$(silent) $(cc) $(cflags) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

$(objectspp): $(objdir)%.o : $(srcdir)%.cpp
	@echo $(notdir $<)
	$(silent) $(cxx) $(cppflags) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

$(shader_targets): $(shader_targetdir)%.csh : $(srcdir)%.glsl | $(shader_targetdir) $(objdir)
	@echo $(notdir $<)
	$(silent) ./$(shader_compiler) $< $(objdir) $@

$(shader_targetdir):
	$(silent) mkdir -p $(shader_targetdir)

$(target): $(objects) $(objectspp) $(deps) | $(target_dir)
	@echo Linking $(target)
	$(silent) ar -rcs "$@" $(objects) $(objectspp)

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

clean:
	$(silent) rm -rf obj
	$(silent) rm -rf bin
	$(silent) rm -f $(bir_target)
	$(silent) rm -f $(shader_targets)

-include $(objects:%.o=%.d) $(objectspp:%.o=%.d)
