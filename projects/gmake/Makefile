ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

projects = shadercompiler corrosion sbox 3d blur sand particles voxel volume ui
runnables =           \
    run_sbox          \
    debug_sbox        \
    memcheck_sbox     \
    run_3d            \
    debug_3d          \
    memcheck_3d       \
    run_blur          \
    debug_blur        \
    memcheck_blur     \
    run_sand          \
    debug_sand        \
    memcheck_sand     \
    run_particles     \
    debug_particles   \
    memcheck_particles\
    run_voxel         \
    debug_voxel       \
    memcheck_voxel    \
    run_volume        \
    debug_volume      \
    memcheck_volume   \
    run_ui            \
    debug_ui          \
    memcheck_ui       \

.PHONY: all clean $(projects) $(runnables) install

all: $(projects)

corrosion: shadercompiler
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C $@ config=$(config)

shadercompiler:
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C $@

sbox: corrosion
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C $@ config=$(config)

run_sbox: sbox
	$(silent) make --no-print-directory -C sbox run

debug_sbox: sbox
	$(silent) make --no-print-directory -C sbox debug

memcheck_sbox: sbox
	$(silent) make --no-print-directory -C sbox memcheck

3d: corrosion
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C demos/$@ config=$(config)

blur: corrosion
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C demos/$@ config=$(config)

sand: corrosion
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C demos/$@ config=$(config)

particles: corrosion
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C demos/$@ config=$(config)

voxel: corrosion
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C demos/$@ config=$(config)

volume: corrosion
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C demos/$@ config=$(config)

ui: corrosion
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C demos/$@ config=$(config)

run_3d: 3d
	$(silent) make --no-print-directory -C demos/3d run

debug_3d: 3d
	$(silent) make --no-print-directory -C demos/3d debug

memcheck_3d: 3d
	$(silent) make --no-print-directory -C demos/3d memcheck

run_blur: blur
	$(silent) make --no-print-directory -C demos/blur run

debug_blur: blur
	$(silent) make --no-print-directory -C demos/blur debug

memcheck_blur: blur
	$(silent) make --no-print-directory -C demos/blur memcheck

run_sand: sand
	$(silent) make --no-print-directory -C demos/sand run

debug_sand: sand
	$(silent) make --no-print-directory -C demos/sand debug

memcheck_sand: sand
	$(silent) make --no-print-directory -C demos/sand memcheck

run_particles: particles
	$(silent) make --no-print-directory -C demos/particles run

debug_particles: particles
	$(silent) make --no-print-directory -C demos/particles debug

memcheck_particles: particles
	$(silent) make --no-print-directory -C demos/particles memcheck

run_voxel: voxel
	$(silent) make --no-print-directory -C demos/voxel run

debug_voxel: voxel
	$(silent) make --no-print-directory -C demos/voxel debug

memcheck_voxel: voxel
	$(silent) make --no-print-directory -C demos/voxel memcheck

run_volume: volume
	$(silent) make --no-print-directory -C demos/volume run

debug_volume: volume
	$(silent) make --no-print-directory -C demos/volume debug

memcheck_volume: volume
	$(silent) make --no-print-directory -C demos/volume memcheck

run_ui: ui
	$(silent) make --no-print-directory -C demos/ui run

debug_ui: ui
	$(silent) make --no-print-directory -C demos/ui debug

memcheck_ui: ui
	$(silent) make --no-print-directory -C demos/ui memcheck

emscripten: corrosion sbox
	$(silent) make --no-print-directory -C corrosion emscripten
	$(silent) make --no-print-directory -C sbox emscripten
	$(silent) make --no-print-directory -C demos/ui emscripten

em_run_sbox: emscripten
	$(silent) emrun sbox/bin/debug/emscripten/sbox.html

clean:
	$(silent) make --no-print-directory -C corrosion clean
	$(silent) make --no-print-directory -C shadercompiler clean
	$(silent) make --no-print-directory -C sbox clean
	$(silent) make --no-print-directory -C demos/3d clean
	$(silent) make --no-print-directory -C demos/blur clean
	$(silent) make --no-print-directory -C demos/sand clean
	$(silent) make --no-print-directory -C demos/particles clean
	$(silent) make --no-print-directory -C demos/voxel clean
	$(silent) make --no-print-directory -C demos/volume clean
	$(silent) make --no-print-directory -C demos/ui clean

installheaderdir = /usr/include/corrosion
installlibdir = /usr/lib64
installbindir = /usr/bin

install:
	make config=release emscripten
	make config=debug emscripten
	install -d $(installheaderdir)
	install -d $(installlibdir)
	install -d $(installbindir)
	install -m 777 ./shadercompiler/bin/release/shadercompiler $(installbindir)/cshc
	install ./../../corrosion/include/corrosion/* $(installheaderdir)
	install ./corrosion/bin/debug/libcr.a $(installlibdir)/libcorrosiond.a
	install ./corrosion/bin/debug/emscripten/libcr.bc $(installlibdir)/libcorrosiond.bc
	install ./corrosion/bin/release/libcr.a $(installlibdir)/libcorrosion.a
	install ./corrosion/bin/release/emscripten/libcr.bc $(installlibdir)/libcorrosion.bc
	install ./corrosion.pc $(installlibdir)/pkgconfig/corrosion.pc
	install ./corrosiondebug.pc $(installlibdir)/pkgconfig/corrosiondebug.pc
