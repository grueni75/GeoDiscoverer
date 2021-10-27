#
# Makefile for creating PNG from SVG icons for GeoDiscoverer
#

# Variables:

ROOT = $(shell cd ../../../.. && pwd)
ANDROID_GDC = $(ROOT)/Source/Platform/Target/Android/mobile/src/main/res/
ANDROID_GDC_QUOTED = $(subst /,\/,$(ANDROID_GDC))
ANDROID_GDB = $(ROOT)/Source/Cockpit/GeoDashboard/app/src/main/res/
ANDROID_GDB_QUOTED = $(subst /,\/,$(ANDROID_GDB))
ANDROID_GDCW = $(ROOT)/Source/Platform/Target/Android/wear/src/main/res/
ANDROID_GDCW_QUOTED = $(subst /,\/,$(ANDROID_GDCW))

SVGS = exit.svg
SVGS = $(shell find SVG -name '*.svg' -printf '%P\n')
DPIS = 160dpi_drawable-mdpi 240dpi_drawable-hdpi 320dpi_drawable-xhdpi 480dpi_drawable-xxhdpi 
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
android: $(ANDROID_GDC) 
.PHONY: android

# Implicit rules:

# Convert all *.svg into *.png
$(foreach dpi,$(DPIS),$(eval $(call SVG2PNG,$(dpi),%.png,%.svg)))

# Explicit rules:

# Install icons into android target
$(ANDROID_GDC): $(PNGS)
	@echo Installing PNGs into android target
	@find -name '*dpi_drawable*' | sed 's/\(.*\n*dpi_\(.*\)\)/cp \1\/* $(ANDROID_GDC_QUOTED)\2\//' | sh
	@find -name '*dpi_drawable*' | sed 's/\(.*\n*dpi_drawable-\(.*\)\)/mv $(ANDROID_GDC_QUOTED)\/drawable-\2\/ic_launcher.png $(ANDROID_GDC_QUOTED)\/mipmap-\2\/ic_launcher.png/' | sh
	@find -name '*dpi_drawable*' | sed 's/\(.*\n*dpi_drawable-\(.*\)\)/mv $(ANDROID_GDC_QUOTED)\/drawable-\2\/car.png $(ANDROID_GDB_QUOTED)\/drawable-\2\//' | sh
	@find -name '*dpi_drawable*' | sed 's/\(.*\n*dpi_drawable-\(.*\)\)/mv $(ANDROID_GDC_QUOTED)\/drawable-\2\/ship.png $(ANDROID_GDB_QUOTED)\/drawable-\2\//' | sh
	@find -name '*dpi_drawable*' | sed 's/\(.*\n*dpi_drawable-\(.*\)\)/mv $(ANDROID_GDC_QUOTED)\/drawable-\2\/dog.png $(ANDROID_GDB_QUOTED)\/drawable-\2\//' | sh
	@find -name '*dpi_drawable*' | sed 's/\(.*\n*dpi_drawable-\(.*\)\)/mv $(ANDROID_GDC_QUOTED)\/drawable-\2\/light.png $(ANDROID_GDB_QUOTED)\/drawable-\2\//' | sh
	@find -name '*dpi_drawable*' | sed 's/\(.*\n*dpi_drawable-\(.*\)\)/mv $(ANDROID_GDC_QUOTED)\/drawable-\2\/ok.png $(ANDROID_GDCW_QUOTED)\/drawable-\2\//' | sh
	@find -name '*dpi_drawable*' | sed 's/\(.*\n*dpi_drawable-\(.*\)\)/cp $(ANDROID_GDC_QUOTED)\/mipmap-\2\/ic_launcher.png $(ANDROID_GDCW_QUOTED)\/mipmap-\2\/ic_launcher.png/' | sh
	@touch $(ANDROID_GDC)

# Cleaning the project
clean:
	@rm -rf *dpi_drawable*

# Debugging the makefile
showvars: 
	@echo "$(SVGS)"
	@echo "$(DPIS)"
	@echo "$(PNGS)"

