#
# Makefile for creating PNG from SVG icons for GeoDiscoverer
#

# Variables:

ROOT = $(shell cd ../../.. && pwd)
LINUX = $(ROOT)/Source/Platform/Target/Linux/Icon/240dpi

SVGS = $(shell find -name '*.svg' -printf '%P\n')
DPIS = 120 160 240 320 480 640
PNGS = $(foreach dpi,$(DPIS),$(foreach svg,$(SVGS),$(dpi)dpi/$(patsubst %.svg,%.png,$(svg))) $(dpi)dpi/navigationTarget.png)


# Macros:

define SVG2PNG
$(1)dpi/$(2): $(3)
	@echo Generating $$@
	@if [ "$$<" == "tileImageNotCached.svg" ] || [ "$$<" == "tileImageNotDownloaded.svg" ] || [ "$$<" == "pathDirection.svg" ]; then \
		inkscape --export-png=$$@ --export-dpi=90 $$<; \
	elif [ "$(2)" == "navigationTarget.png" ]; then \
		inkscape --export-png=$$@ --export-dpi=`echo $(1)*90/240/12 | bc` $$<; \
	else \
		inkscape --export-png=$$@ --export-dpi=`echo $(1)*90/240 | bc` $$<; \
	fi >/dev/null
endef

# General dependencies:

linux: $(LINUX)
.PHONY: linux
all:    linux
.PHONY: all

# Implicit rules:

# Convert all *.svg into *.png
$(foreach dpi,$(DPIS),$(eval $(call SVG2PNG,$(dpi),%.png,%.svg)))

# Explicit rules:

# Convert target.svg into navigationTarget.png
$(foreach dpi,$(DPIS),$(eval $(call SVG2PNG,$(dpi),navigationTarget.png,target.svg)))

# Install icons into linux target
$(LINUX): $(PNGS)
	@echo Installing PNGs into linux target
	@rm -rf $(LINUX)
	@cp -r 240dpi $(LINUX)/

# Cleaning the project

clean:
	@-rm -rf $(PNGS)  

# Debugging the makefile
showvars: 
	@echo "$(SVGS)"
	@echo "$(DPIS)"
	@echo "$(PNGS)"

