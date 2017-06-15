#
# Makefile for creating PNG from SVG icons for GeoDiscoverer
#

# Variables:

ROOT = $(shell cd ../../.. && pwd)
LINUX = $(ROOT)/Source/Platform/Target/Linux/Icon/
ANDROID_MOBILE = $(ROOT)/Source/Platform/Target/Android/mobile/src/main/assets/GeoDiscoverer/Icon/
ANDROID_WEAR = $(ROOT)/Source/Platform/Target/Android/wear/src/main/assets/GeoDiscoverer/Icon/

SVGS = $(shell find -name '*.svg' -printf '%P\n')
DPIS = 120 160 240 320 480 640
PNGS = $(foreach dpi,$(DPIS),$(foreach svg,$(SVGS),$(dpi)dpi/$(patsubst %.svg,%.png,$(svg))) $(dpi)dpi/navigationTarget.png)

# Macros:

define SVG2PNG
$(1)dpi/$(2): $(3)
	@echo Generating $$@
	@mkdir -p $(1)dpi
	@if [ "$$<" = "tileImageNotCached.svg" ] || [ "$$<" = "tileImageNotDownloaded.svg" ] || [ "$$<" = "tileImageDownloadErrorOccured.svg" ] || [ "$$<" = "pathDirection.svg" ]; then \
		inkscape --export-png=$$@ --export-dpi=90 $$<; \
	elif [ "$(2)" = "navigationTarget.png" ]; then \
		inkscape --export-png=$$@ --export-dpi=`echo $(1)*90/240/12 | bc` $$<; \
	else \
		inkscape --export-png=$$@ --export-dpi=`echo $(1)*90/240 | bc` $$<; \
	fi >/dev/null
endef

# General dependencies:

all:    linux android
.PHONY: all
linux: $(LINUX)
.PHONY: linux
android: $(ANDROID_MOBILE) $(ANDROID_WEAR)
.PHONY: android

# Implicit rules:

# Convert all *.svg into *.png
$(foreach dpi,$(DPIS),$(eval $(call SVG2PNG,$(dpi),%.png,%.svg)))

# Explicit rules:

# Convert target.svg into navigationTarget.png
$(foreach dpi,$(DPIS),$(eval $(call SVG2PNG,$(dpi),navigationTarget.png,target.svg)))

# Install icons into linux target
$(LINUX): $(PNGS)
	@echo Installing PNGs into linux target
	@rm -rf $(LINUX)/240dpi
	@cp -r 240dpi $(LINUX)/
	@rm -rf $(LINUX)/320dpi
	@cp -r 320dpi $(LINUX)/

# Install icons into android target
$(ANDROID_MOBILE): $(PNGS)
	@echo Installing PNGs into android mobile target
	@mkdir -p $(ANDROID_MOBILE)
	@rm -rf $(ANDROID_MOBILE)/*dpi
	@cp -r *dpi $(ANDROID_MOBILE)/

# Install icons into android target
$(ANDROID_WEAR): $(PNGS)
	@echo Installing PNGs into android wear target
	@mkdir -p $(ANDROID_WEAR)
	@rm -rf $(ANDROID_WEAR)/*dpi
	@cp -r *dpi $(ANDROID_WEAR)/

# Cleaning the project
clean:
	@rm -rf $(PNGS)
	@rm -rf $(LINUX)/240dpi
	@rm -rf $(ANDROID_MOBILE)/*dpi

# Debugging the makefile
showvars: 
	@echo "$(SVGS)"
	@echo "$(DPIS)"
	@echo "$(PNGS)"

