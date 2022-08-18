ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

projects = shadercompiler corrosion sbox shooter bloom particles
runnables =           \
    run_sbox          \
    debug_sbox        \
    memcheck_sbox     \
    run_shooter       \
    debug_shooter     \
    memcheck_shooter  \
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

shooter: corrosion
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C games/$@ config=$(config)

bloom: corrosion
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C games/$@ config=$(config)

particles: corrosion
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C games/$@ config=$(config)

run_shooter: shooter
	$(silent) make --no-print-directory -C games/shooter run

debug_shooter: shooter
	$(silent) make --no-print-directory -C games/shooter debug

memcheck_shooter: shooter
	$(silent) make --no-print-directory -C games/shooter memcheck

run_bloom: bloom
	$(silent) make --no-print-directory -C games/bloom run

debug_bloom: bloom
	$(silent) make --no-print-directory -C games/bloom debug

memcheck_bloom: bloom
	$(silent) make --no-print-directory -C games/bloom memcheck

run_particles: particles
	$(silent) make --no-print-directory -C games/particles run

debug_particles: particles
	$(silent) make --no-print-directory -C games/particles debug

memcheck_particles: particles
	$(silent) make --no-print-directory -C games/particles memcheck

clean:
	$(silent) make --no-print-directory -C corrosion clean
	$(silent) make --no-print-directory -C shadercompiler clean
	$(silent) make --no-print-directory -C sbox clean
	$(silent) make --no-print-directory -C games/shooter clean
	$(silent) make --no-print-directory -C games/bloom clean
	$(silent) make --no-print-directory -C games/particles clean
