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
  defines += -Drelease
  clfags += -MMD -MP -m64 -02 $(includes) $(defines)
  cppflags += -MMD -MP -m64 -02 $(includes) $(defines)
  objdir = obj/release
endif

all: $(target)

sources =                            \
          $(srcdir)/atlas.c          \
          $(srcdir)/core.c           \
          $(srcdir)/dtable.c         \
          $(srcdir)/entrypoint.c     \
          $(srcdir)/font.c           \
          $(srcdir)/log.c            \
          $(srcdir)/res.c            \
          $(srcdir)/res_posix.c      \
          $(srcdir)/serialise.c      \
          $(srcdir)/simplerenderer.c \
          $(srcdir)/stb.c            \
          $(srcdir)/timer_posix.c    \
          $(srcdir)/ui.c             \
          $(srcdir)/ui_render.c      \
          $(srcdir)/video.c          \
          $(srcdir)/video_vk.c       \
          $(srcdir)/window_x11.c     \

sourcespp = $(srcdir)/vma.cpp

objects   = $(sources:$(srcdir)%.c=$(objdir)%.o)
objectspp = $(sourcespp:$(srcdir)%.cpp=$(objdir)%.o)

$(objects):   | $(objdir)
$(objectspp): | $(objdir)

$(objects): $(objdir)%.o : $(srcdir)%.c
	@echo $(notdir $<)
	$(silent) $(cc) $(cflags) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

$(objectspp): $(objdir)%.o : $(srcdir)%.cpp
	@echo $(notdir $<)
	$(silent) $(cxx) $(cppflags) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

$(target): $(objects) $(objectspp) $(deps) | $(target_dir)
	@echo Linking $(target)
	$(silent) ar -rcs "$@" $(objects) $(objectspp)

$(target_dir):
	$(silent) mkdir -p $(target_dir)

$(objdir):
	$(silent) mkdir -p $(objdir)

clean:
	$(silent) rm -rf obj
	$(silent) rm -rf bin

-include $(objects:%.o=%.d) $(objectspp:%.o=%.d)
