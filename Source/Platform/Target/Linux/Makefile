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
PLATFORM_SRCS   += $(shell find $(ROOT)/Source/Platform/Feature/stdout -name '*.cpp')
PLATFORM_SRCS   += $(shell find $(ROOT)/Source/Platform/Feature/libfreetype2 -name '*.cpp')
PLATFORM_SRCS   += $(shell find $(ROOT)/Source/Platform/Target/Linux -name '*.cpp')
GENERAL_SRCS     = $(shell find $(ROOT)/Source/General -name '*.cpp')
CPP_SRCS         = $(subst $(ROOT)/,,$(PLATFORM_SRCS) $(GENERAL_SRCS))

PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/POSIX -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/OpenGL -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/libjpeg -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/libpng -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/libxml2 -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/stdout -name '*.h')
PLATFORM_HEADERS += $(shell find $(ROOT)/Source/Platform/Feature/libfreetype2 -name '*.h')
GENERAL_HEADERS   = $(shell find $(ROOT)/Source/General -name '*.h')
HEADERS           = $(PLATFORM_HEADERS) $(GENERAL_HEADERS)

OBJDIR        = Build
CPP_OBJS      = $(patsubst %.cpp,%.o,$(CPP_SRCS))
OBJS          = $(addprefix $(OBJDIR)/,$(CPP_OBJS)) 

INCLUDES += -I$(ROOT)/Source/Platform/Feature/POSIX
INCLUDES += -I$(ROOT)/Source/Platform/Feature/OpenGL
INCLUDES += -I$(ROOT)/Source/Platform/Feature/libxml2
INCLUDES += -I$(ROOT)/Source/Platform/Feature/stdout
INCLUDES += -I$(ROOT)/Source/Platform/Feature/libfreetype2
INCLUDES += -I/usr/include/freetype2
INCLUDES += -I$(ROOT)/Source/General/App
INCLUDES += -I$(ROOT)/Source/General/Map
INCLUDES += -I$(ROOT)/Source/General/Graphic
INCLUDES += -I$(ROOT)/Source/General/Image
INCLUDES += -I$(ROOT)/Source/General/Widget
INCLUDES += -I$(ROOT)/Source/General/Navigation
INCLUDES += -I$(ROOT)/Source/General/Math
INCLUDES += -I$(ROOT)/Source/General/Config
INCLUDES += -I$(ROOT)/Source/General/Profile

LIBS += -lstdc++ -lglut -lpthread -lGLU -ljpeg -lxml2 -lfreetype -lpng

CXXFLAGS += -DSRC_ROOT='"$(ROOT)/Source"' -DTARGET_LINUX -Wnon-virtual-dtor
#debug: CXXFLAGS += -g -DDMALLOC -DDMALLOC_FUNC_CHECK  
#debug: LIBS += -ldmalloccxx
debug: CXXFLAGS += -g -O0

# General dependencies:

release: $(PRGNAME)
.PHONY: release
debug:   $(PRGNAME)
.PHONY: debug
all:    release
.PHONY: all

# Implicit rules:

$(OBJDIR)/%.o: $(ROOT)/%.cpp $(HEADERS)
	@-mkdir -p `dirname $@`
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $< -o $@

# The main program:

$(PRGNAME): $(OBJS)
	@echo "$(OBJS)"
	$(LINK_PREFIX) $(CXX) $(CXXFLAGS) $(OBJS) $(STATIC_OBJS) $(LIBS) $(LIBDIRS) -o $(PRGNAME)

# Cleaning the project

clean:
	@-rm -rf $(OBJDIR) $(DEPFILE) $(PRGNAME) 

# Debugging the makefile
showvars: 
	@echo "$(OBJS)"
	@echo "$(CPP_SRCS)"
	@echo "$(HEADERS)"
