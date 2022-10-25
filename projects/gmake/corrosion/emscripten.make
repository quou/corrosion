ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

.phony: clean

cc = emcc
cxx = em++
includes = -I../../../corrosion/include/corrosion -I../../../corrosion/src
targetname = cr
defines = -Dcr_no_vulkan
libs = 
deps = 
srcdir = ../../../corrosion/src
cstd = -std=c11
cxxstd = -std=c++20

ifeq ($(config),debug)
  targetdir = bin/debug/emscripten
  target = $(targetdir)/lib$(targetname).bc
  defines += -Ddebug
  cflags = -MMD -MP  -g $(cstd) $(includes) $(defines)
  cxxflags = -MMD -MP -g $(cxxstd) $(includes) $(defines)
  objdir = obj
endif

ifeq ($(config),release)
  targetdir = bin/release
  target = $(targetdir)/lib$(targetname).bc
  defined += -Dndebug
  cflags = -MMD -MP -O2 $(cstd) $(includes) $(defines)
  cxxflags = -MMD -MP -O2 $(cxxstd) $(includes) $(defines)
  objdir = obj/release
endif

all: $(target)
	@:

sources =                               \
          $(srcdir)/alloc.c             \
          $(srcdir)/atlas.c             \
          $(srcdir)/bir.c               \
          $(srcdir)/core.c              \
          $(srcdir)/dtable.c            \
          $(srcdir)/font.c              \
          $(srcdir)/gizmo.c             \
          $(srcdir)/log.c               \
          $(srcdir)/render_util.c       \
          $(srcdir)/res.c               \
          $(srcdir)/res_emscripten.c    \
          $(srcdir)/simplerenderer.c    \
          $(srcdir)/stb.c               \
          $(srcdir)/thread_emscripten.c \
          $(srcdir)/timer_emscripten.c  \
          $(srcdir)/ui.c                \
          $(srcdir)/ui_render.c         \
          $(srcdir)/video.c             \
          $(srcdir)/video_common.c      \
          $(srcdir)/video_gl.c          \
          $(srcdir)/video_vk.c          \
          $(srcdir)/window_common.c     \
          $(srcdir)/window_emscripten.c \

sourcesxx = \
	$(srcdir)/vma.cpp       \

objects = $(sources:$(srcdir)/%.c=$(objdir)/%.o)
objectsxx = $(sourcesxx:$(srcdir)/%.cpp=$(objdir)/%.opp)

$(objects): | $(objdir)
$(objectsxx): | $(objdir)

$(objects): $(objdir)/%.o : $(srcdir)/%.c
	@echo $(notdir $<)
	$(silent) $(cc) $(cflags) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

$(objectsxx): $(objdir)/%.opp : $(srcdir)/%.cpp
	@echo $(notdir $<)
	$(silent) $(cxx) $(cxxflags) -o "$@" -MF "$(@:%.opp=%.dpp)" -c "$<"

$(target): $(objects) $(objectsxx) $(deps) | $(targetdir)
	@echo linking $(target)
	$(silent) emar -rcs "$@" $(objects) $(objectsxx)
	ctags -R

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
