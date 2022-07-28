ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

projects = shadercompiler corrosion sbox shooter
runnables =           \
    run_sbox          \
    debug_sbox        \
    memcheck_sbox     \
    run_shooter       \
    debug_shooter     \
    memcheck_shooter  \

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

run_shooter: shooter
	$(silent) make --no-print-directory -C games/shooter run

debug_shooter: shooter
	$(silent) make --no-print-directory -C games/shooter debug

memcheck_shooter: shooter
	$(silent) make --no-print-directory -C games/shooter memcheck

clean:
	$(silent) make --no-print-directory -C corrosion clean
	$(silent) make --no-print-directory -C shadercompiler clean
	$(silent) make --no-print-directory -C sbox clean
	$(silent) make --no-print-directory -C games/shooter clean
