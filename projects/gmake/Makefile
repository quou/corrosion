ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

projects = shadercompiler corrosion sbox 3d bloom particles
runnables =           \
    run_sbox          \
    debug_sbox        \
    memcheck_sbox     \
    run_3d       \
    debug_3d     \
    memcheck_3d  \
    run_bloom         \
    debug_bloom       \
    memcheck_bloom    \
    run_particles     \
    debug_particles   \
    memcheck_particles\

.PHONY: all clean $(projects) $(runnables)

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

bloom: corrosion
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C demos/$@ config=$(config)

particles: corrosion
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C demos/$@ config=$(config)

run_3d: 3d
	$(silent) make --no-print-directory -C demos/3d run

debug_3d: 3d
	$(silent) make --no-print-directory -C demos/3d debug

memcheck_3d: 3d
	$(silent) make --no-print-directory -C demos/3d memcheck

run_bloom: bloom
	$(silent) make --no-print-directory -C demos/bloom run

debug_bloom: bloom
	$(silent) make --no-print-directory -C demos/bloom debug

memcheck_bloom: bloom
	$(silent) make --no-print-directory -C demos/bloom memcheck

run_particles: particles
	$(silent) make --no-print-directory -C demos/particles run

debug_particles: particles
	$(silent) make --no-print-directory -C demos/particles debug

memcheck_particles: particles
	$(silent) make --no-print-directory -C demos/particles memcheck

clean:
	$(silent) make --no-print-directory -C corrosion clean
	$(silent) make --no-print-directory -C shadercompiler clean
	$(silent) make --no-print-directory -C sbox clean
	$(silent) make --no-print-directory -C demos/3d clean
	$(silent) make --no-print-directory -C demos/bloom clean
	$(silent) make --no-print-directory -C demos/particles clean
