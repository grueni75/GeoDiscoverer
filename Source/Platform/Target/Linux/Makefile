#
# Makefile for GeoDiscoverer
#

# Variables:

PRGNAME = GeoDiscoverer
ROOT = $(shell cd ../../../.. && pwd)

PLATFORM_SRCS   += $(shell find $(ROOT)/Source/Platform/Feature/POSIX -name '*.cpp')
PLATFORM_SRCS   += $(shell find $(ROOT)/Source/Platform/Feature/OpenGL -name '*.cpp')
PLATFORM_SRCS   += $(shell find $(ROOT)/Source/Platform/Feature/libjpeg -name '*.cpp')
PLATFORM_SRCS   += $(shell find $(ROOT)/Source/Platform/Feature/libpng -name '*.cpp')
PLATFORM_SRCS   += $(shell find $(ROOT)/Source/Platform/Feature/libxml2 -name '*.cpp')
PLATFORM_SRCS   += $(shell find $(ROOT)/Source/Platform/Feature/libfreetype2 -name '*.cpp')
PLATFORM_SRCS   += $(shell find $(ROOT)/Source/Platform/Feature/libcurl -name '*.cpp')
PLATFORM_SRCS   += $(shell find $(ROOT)/Source/Platform/Feature/libzip -name '*.cpp')
PLATFORM_SRCS   += $(shell find $(ROOT)/Source/Platform/Feature/libproj -name '*.cpp')
PLATFORM_SRCS   += $(shell find $(ROOT)/Source/Platform/Feature/libgdal -name '*.cpp')
PLATFORM_SRCS   += $(shell find $(ROOT)/Source/Platform/Feature/openssl -name '*.cpp')
PLATFORM_SRCS   += $(shell find $(ROOT)/Source/Platform/Feature/Linux -name '*.cpp')
PLATFORM_SRCS   += $(shell find $(ROOT)/Source/Platform/Target/Linux -name '*.cpp')
GENERAL_SRCS     = $(shell find $(ROOT)/Source/General -name '*.cpp')
CPP_SRCS         = $(subst $(ROOT)/,,$(PLATFORM_SRCS) $(GENERAL_SRCS))

PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/POSIX -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/OpenGL -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/libjpeg -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/libpng -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/libxml2 -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/libfreetype2 -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/libcurl -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/libzip -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/libproj -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/libgdal -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/openssl -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/Linux -name '*.h')
GENERAL_HEADERS   = $(shell find $(ROOT)/Source/General -name '*.h')
HEADERS           = $(PLATFORM_HEADERS) $(GENERAL_HEADERS)

OBJDIR        = Build
CPP_OBJS      = $(patsubst %.cpp,%.o,$(CPP_SRCS))
OBJS          = $(addprefix $(OBJDIR)/,$(CPP_OBJS)) 

INCLUDES += -I$(ROOT)/Source/Platform/Feature/POSIX
INCLUDES += -I$(ROOT)/Source/Platform/Feature/OpenGL
INCLUDES += -I$(ROOT)/Source/Platform/Feature/libxml2
INCLUDES += -I$(ROOT)/Source/Platform/Feature/libfreetype2
INCLUDES += -I$(ROOT)/Source/Platform/Feature/libcurl
INCLUDES += -I$(ROOT)/Source/Platform/Feature/libzip
INCLUDES += -I$(ROOT)/Source/Platform/Feature/libproj
INCLUDES += -I$(ROOT)/Source/Platform/Feature/libgdal
INCLUDES += -I$(ROOT)/Source/Platform/Feature/openssl
INCLUDES += -I$(ROOT)/Source/Platform/Feature/Linux
INCLUDES += -I$(ROOT)/Source/General/App
INCLUDES += -I$(ROOT)/Source/General/Elevation
INCLUDES += -I$(ROOT)/Source/General/Map
INCLUDES += -I$(ROOT)/Source/General/Graphic
INCLUDES += -I$(ROOT)/Source/General/Image
INCLUDES += -I$(ROOT)/Source/General/Widget
INCLUDES += -I$(ROOT)/Source/General/Navigation
INCLUDES += -I$(ROOT)/Source/General/Math
INCLUDES += -I$(ROOT)/Source/General/Config
INCLUDES += -I$(ROOT)/Source/General/Profile
INCLUDES += -I/usr/include/freetype2
INCLUDES += -I/usr/include/libxml2
INCLUDES += -I/usr/include/gdal

LIBS += -lstdc++ -lglut -lpthread -lGLU -ljpeg -lxml2 -lfreetype -lpng -lcurl -lzip -lproj -lGL -lm -lcrypto -lgdal

DEFINES = -DSRC_ROOT='"$(ROOT)/Source"' -DTARGET_LINUX
CXXFLAGS += -Wnon-virtual-dtor -fsanitize=address
# normal debug
debug: CXXFLAGS += -g -O0 
# Use this to get more warnings on the source code
#CXX = clang
# for detecting various heap & stack problems (requires CXX=clang)
#debug: CXXFLAGS += -g -O0 -fsanitize=address
# for detecting uninitialized memory use (requires CXX=clang)
#debug: CXXFLAGS += -g -O0 -fsanitize=memory -fPIE -pie

# General dependencies:

release: $(PRGNAME) config.shipped.xsd icons
.PHONY: release
debug:   $(PRGNAME) config.shipped.xsd icons
.PHONY: debug
all:    release
.PHONY: all

# Implicit rules:

$(OBJDIR)/%.o: $(ROOT)/%.cpp 
	@-mkdir -p `dirname $@`
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $< -o $@

# Compile rules based on header dependencies

depend: $(OBJDIR)/.depend

$(OBJDIR)/.depend: $(PLATFORM_SRCS) $(GENERAL_SRCS)
	@-mkdir -p Build
	@cd $(ROOT); makedepend -f- $(DEFINES) $(INCLUDES) -pBuild/ $(CPP_SRCS) >.depend 2>/dev/null
	@grep -v /usr/ $(ROOT)/.depend >$(OBJDIR)/.depend
	@rm $(ROOT)/.depend

include $(OBJDIR)/.depend

# The main program:

$(PRGNAME): $(OBJS)
	@echo "$(OBJS)"
	$(LINK_PREFIX) $(CXX) $(CXXFLAGS) $(OBJS) $(STATIC_OBJS) $(LIBS) $(LIBDIRS) -o $(PRGNAME)

# XML schema for configuration
# Update if source XSD has changed or widget engine
config.shipped.xsd: $(ROOT)/Source/config.xsd $(ROOT)/Source/General/Widget/WidgetEngine.cpp 
	cd $(ROOT)/Source && make linux
	sed -i 's/<xsd:documentation>GeoDiscoverer config schema.*<\/xsd:documentation>/<xsd:documentation>GeoDiscoverer config schema '`date +"%Y%m%d-%H%M%S"`'<\/xsd:documentation>/' config.shipped.xsd 

# Icons
icons:
	cd $(ROOT)/Source/Media/Icon && make linux

# Cleaning the project

clean:
	@-rm -rf $(OBJDIR) $(DEPFILE) $(PRGNAME) 

# Debugging the makefile
showvars: 
	@echo "$(OBJS)"
	@echo "$(CPP_SRCS)"
	@echo "$(HEADERS)"
