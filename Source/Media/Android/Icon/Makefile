#
# Makefile for creating PNG from SVG icons for GeoDiscoverer
#

# Variables:

ROOT = $(shell cd ../../../.. && pwd)
ANDROID = $(ROOT)/Source/Platform/Target/Android/res/
ANDROID_QUOTED = $(subst /,\/,$(ANDROID))

SVGS = exit.svg
SVGS = $(shell find SVG -name '*.svg' -printf '%P\n')
DPIS = 120dpi_drawable-ldpi 160dpi_drawable-mdpi 240dpi_drawable-hdpi 320dpi_drawable-xhdpi 480dpi_drawable-xxhdpi 640dpi_drawable-xxxhdpi
PNGS = $(foreach dpi,$(DPIS),$(foreach svg,$(SVGS),$(dpi)/$(patsubst %.svg,%.png,$(svg))))

# Macros:

define SVG2PNG
$(1)/$(2): SVG/$(3)
	@echo Generating $$@
	@mkdir -p $(1)
	inkscape --export-png=$$@ --export-dpi=`echo $(1) | sed 's/\(\n*\)dpi.*/\1*90\/240/' | bc` $$<; 
endef

# General dependencies:

all:    android
.PHONY: all
android: $(ANDROID)
.PHONY: android

# Implicit rules:

# Convert all *.svg into *.png
$(foreach dpi,$(DPIS),$(eval $(call SVG2PNG,$(dpi),%.png,%.svg)))

# Explicit rules:

# Install icons into android target
$(ANDROID): $(PNGS)
	@echo Installing PNGs into android target
	@find -name '*dpi_drawable*' | sed 's/\(.*\n*dpi_\(.*\)\)/cp \1\/* $(ANDROID_QUOTED)\2\//' | sh
	@touch $(ANDROID)

# Cleaning the project
clean:
	@rm -rf *dpi_drawable*

# Debugging the makefile
showvars: 
	@echo "$(SVGS)"
	@echo "$(DPIS)"
	@echo "$(PNGS)"

