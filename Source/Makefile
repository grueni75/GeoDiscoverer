#
# Makefile for distributing the config schema of GeoDiscoverer
#

# Variables:

ROOT = $(shell cd .. && pwd)
LINUX = $(ROOT)/Source/Platform/Target/Linux
ANDROID = $(ROOT)/Source/Platform/Target/Android/assets/GeoDiscoverer

# General dependencies:

all:    linux android
.PHONY: all
linux:	$(LINUX)/config.xsd
.PHONY: release
android:	$(ANDROID)/config.xsd
.PHONY: android

# Explicit rules:

# Install config schema into linux target
$(LINUX)/config.xsd: $(ROOT)/Source/config.xsd
	cp $< $@

# Install config schema into android target
$(ANDROID)/config.xsd: $(ROOT)/Source/config.xsd
	cp $< $@

# Debugging the makefile
showvars: 
	@echo "$(ROOT)"
	@echo "$(LINUX)"
	@echo "$(PNGS)"

