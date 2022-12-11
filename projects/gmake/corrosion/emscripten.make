ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

.phony: clean

cc = emcc
includes = -I../../../corrosion/include/corrosion -I../../../corrosion/src
targetname = cr
defines = -Dcr_no_vulkan -Wno-unreachable-code-generic-assoc
opts = -pthread
deps = 
srcdir = ../../../corrosion/src
cstd = -std=gnu11

ifeq ($(config),debug)
  targetdir = bin/debug/emscripten
  target = $(targetdir)/lib$(targetname).bc
  defines += -Ddebug
  cflags = -MMD -MP  -g $(cstd) $(includes) $(defines) $(opts)
  objdir = obj/debug/emscripten
endif

ifeq ($(config),release)
  targetdir = bin/release/emscripten
  target = $(targetdir)/lib$(targetname).bc
  defined += -Dndebug
  cflags = -MMD -MP -O2 $(cstd) $(includes) $(defines) $(opts)
  objdir = obj/release/emscripten
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
          $(srcdir)/thread_posix.c      \
          $(srcdir)/timer_posix.c       \
          $(srcdir)/ui.c                \
          $(srcdir)/ui_render.c         \
          $(srcdir)/video.c             \
          $(srcdir)/video_common.c      \
          $(srcdir)/video_gl.c          \
          $(srcdir)/video_vk.c          \
          $(srcdir)/window_common.c     \
          $(srcdir)/window_emscripten.c \

objects = $(sources:$(srcdir)/%.c=$(objdir)/%.o)

$(objects): | $(objdir)

$(objects): $(objdir)/%.o : $(srcdir)/%.c
	@echo $(notdir $<)
	$(silent) $(cc) $(cflags) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

$(target): $(objects) $(deps) | $(targetdir)
	@echo linking $(target)
	$(silent) emar -rcs "$@" $(objects)
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
