ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

projects = shadercompiler corrosion sbox
runnables = run_sbox debug_sbox memcheck_sbox

.PHONY: all clean $(projects) $(runnables)

all: $(projects)

corrosion:
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

clean:
	$(silent) make --no-print-directory -C corrosion clean
	$(silent) make --no-print-directory -C shadercompiler clean
	$(silent) make --no-print-directory -C sbox clean
