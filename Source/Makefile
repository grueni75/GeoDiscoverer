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
linux:	$(LINUX)/config.shipped.xsd
.PHONY: release
android:	$(ANDROID)/config.shipped.xsd
.PHONY: android

# Explicit rules:

# Install config schema into linux target
$(LINUX)/config.shipped.xsd: $(ROOT)/Source/config.xsd
	cp $< $@

# Install config schema into android target
$(ANDROID)/config.shipped.xsd: $(ROOT)/Source/config.xsd
	cp $< $@

# Debugging the makefile
showvars: 
	@echo "$(ROOT)"
	@echo "$(LINUX)"
	@echo "$(PNGS)"

