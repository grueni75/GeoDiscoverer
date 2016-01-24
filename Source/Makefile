#
# Makefile for distributing the config schema of GeoDiscoverer
#

# Variables:

ROOT = $(shell cd .. && pwd)
LINUX = $(ROOT)/Source/Platform/Target/Linux
ANDROID_MOBILE = $(ROOT)/Source/Platform/Target/Android/mobile/src/main/assets/GeoDiscoverer
ANDROID_WEAR = $(ROOT)/Source/Platform/Target/Android/wear/src/main/assets/GeoDiscoverer
HELP = $(ROOT)/Source/Platform/Target/Help/public

# General dependencies:

all:    linux android help
.PHONY: all
linux:	$(LINUX)/config.shipped.xsd $(LINUX)/source.xsd
.PHONY: release
android:	$(ANDROID_MOBILE)/config.shipped.xsd $(ANDROID_MOBILE)/source.xsd $(ANDROID_WEAR)/config.shipped.xsd $(ANDROID_WEAR)/source.xsd
.PHONY: android
help:	$(HELP)/MapCreation/MapSource.xsd $(HELP)/MapCreation/MapContainer.xsd
.PHONY: help

# Explicit rules:

# Install config schema into linux target
$(LINUX)/config.shipped.xsd: $(ROOT)/Source/config.xsd
	cp $< $@

# Install map source schema into linux target
$(LINUX)/source.xsd: $(ROOT)/Source/source.xsd
	cp $< $@

# Install config schema into android target (mobile app)
$(ANDROID_MOBILE)/config.shipped.xsd: $(ROOT)/Source/config.xsd
	cp $< $@

# Install map source schema into android target (mobile app)
$(ANDROID_MOBILE)/source.xsd: $(ROOT)/Source/source.xsd
	cp $< $@

# Install config schema into android target (wear app)
$(ANDROID_WEAR)/config.shipped.xsd: $(ROOT)/Source/config.xsd
	cp $< $@

# Install map source schema into android target (wear app)
$(ANDROID_WEAR)/source.xsd: $(ROOT)/Source/source.xsd
	cp $< $@

# Install map source schema into help target
$(HELP)/MapCreation/MapSource.xsd: $(ROOT)/Source/source.xsd
	cp $< $@

# Install map container schema into help target
$(HELP)/MapCreation/MapContainer.xsd: $(ROOT)/Source/container.xsd
	cp $< $@

# Debugging the makefile
showvars: 
	@echo "$(ROOT)"
	@echo "$(LINUX)"
	@echo "$(ANDROID_MOBILE)"
	@echo "$(ANDROID_WEAR)"
	@echo "$(HELP)"

