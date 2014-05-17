#
# Makefile for distributing the config schema of GeoDiscoverer
#

# Variables:

ROOT = $(shell cd .. && pwd)
LINUX = $(ROOT)/Source/Platform/Target/Linux

# General dependencies:

linux: $(LINUX)/config.xsd
.PHONY: release
all:    linux
.PHONY: all

# Explicit rules:

# Install config schema into linux target
$(LINUX)/config.xsd: $(ROOT)/Source/config.xsd
	@cp $< $@

# Debugging the makefile
showvars: 
	@echo "$(ROOT)"
	@echo "$(LINUX)"
	@echo "$(PNGS)"

