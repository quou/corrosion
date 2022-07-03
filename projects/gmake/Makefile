ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

projects = corrosion shaders sbox
runnables = run_sbox

.PHONY: all clean $(projects) $(runnables)

all: $(projects)

corrosion:
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C $@ config=$(config)

shaders:
	@echo == Building $@ ==
	$(silent) make --no-print-directory -C $@

sbox: corrosion shaders
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
	$(silent) make --no-print-directory -C shaders clean
	$(silent) make --no-print-directory -C sbox clean
