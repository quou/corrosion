ifndef config
  config=debug
endif

ifndef verbose
  silent = @
endif

.PHONY: all clean .FORCE

cc = gcc
shader_compiler = compiler
shader_compiler_src = ../../../shaders/compiler.c
srcdir = ../../../shaders/src
objdir = obj
bindir = bin

# Copy the final binary to these directories.
copydirs = ../../../sbox/shaderbin

sources = $(wildcard $(srcdir)/*.glsl $(srcdir/*/*.glsl))
objects = $(sources:$(srcdir)/%.glsl=$(bindir)/%.csh)

all: $(objects) $(copydirs)

$(objects): | $(objdir) $(bindir) $(shader_compiler)

$(objects): $(bindir)%.csh : $(srcdir)%.glsl
	@echo $(notdir $<)
	$(silent) ./$(shader_compiler) $< $(objdir) $@

$(objdir):
	$(silent) mkdir -p $(objdir)

$(copydirs): .FORCE
	$(silent) mkdir -p $@
	@echo "Copying shader binaries to $@..."
	$(silent) cp $(bindir)/* $@/

.FORCE:

$(shader_compiler): $(shader_compiler_src)
	@echo "Compiling shader compiler..."
	$(silent) $(cc)  -I../../../corrosion/include -O2 $< -L/usr/lib64 -m64 -s -o "$@"

$(bindir):
	$(silent) mkdir -p $(bindir)

clean:
	$(silent) rm -rf $(objdir)
	$(silent) rm -rf $(bindir)
	$(silent) rm $(shader_compiler)
