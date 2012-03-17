LOCAL_PATH := $(call my-dir)

# Build the libxml2 library
include $(CLEAR_VARS)
LOCAL_MODULE := gdxml
MY_LIBXML2_PATH := libxml2-2.7.7
LOCAL_CFLAGS := -D_REENTRANT -O2 -I$(MY_LIBXML2_PATH) -I$(MY_LIBXML2_PATH)/include 
LOCAL_LDLIBS := -lz -ldl 
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,SAX.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,entities.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,encoding.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,error.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,parserInternals.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,parser.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,tree.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,hash.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,list.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,xmlIO.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,xmlmemory.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,uri.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,valid.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,xlink.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,HTMLparser.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,HTMLtree.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,debugXML.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,xpath.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,xpointer.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,xinclude.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,nanohttp.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,nanoftp.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,DOCBparser.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,catalog.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,globals.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,threads.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,c14n.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,xmlstring.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,xmlregexp.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,xmlschemas.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,xmlschemastypes.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,xmlunicode.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,xmlreader.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,relaxng.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,dict.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,SAX2.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,xmlwriter.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,legacy.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,chvalid.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,pattern.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,xmlsave.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,xmlmodule.c)
LOCAL_SRC_FILES += $(addprefix $(MY_LIBXML2_PATH)/,schematron.c)
include $(BUILD_STATIC_LIBRARY)

# Build the jpeg library
include $(CLEAR_VARS)
LOCAL_MODULE := gdjpeg
MY_JPEG_PATH := jpeg-8b
LOCAL_CFLAGS := -I$(MY_JPEG_PATH) -DHAVE_CONFIG_H 
LOCAL_LDLIBS := 
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jaricom.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jcapimin.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jcapistd.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jcarith.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jccoefct.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jccolor.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jcdctmgr.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jchuff.c) 
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jcinit.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jcmainct.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jcmarker.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jcmaster.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jcomapi.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jcparam.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jcprepct.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jcsample.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jctrans.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jdapimin.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jdapistd.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jdarith.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jdatadst.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jdatasrc.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jdcoefct.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jdcolor.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jddctmgr.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jdhuff.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jdinput.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jdmainct.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jdmarker.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jdmaster.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jdmerge.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jdpostct.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jdsample.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jdtrans.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jerror.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jfdctflt.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jfdctfst.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jfdctint.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jidctflt.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jidctfst.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jidctint.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jquant1.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jquant2.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jutils.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jmemmgr.c)
LOCAL_SRC_FILES += $(addprefix $(MY_JPEG_PATH)/,jmemnobs.c)
include $(BUILD_STATIC_LIBRARY)

# Build the freetype library
include $(CLEAR_VARS)
LOCAL_MODULE := gdfreetype
MY_FREETYPE_PATH := freetype-2.4.2
LOCAL_CFLAGS := -I$(MY_FREETYPE_PATH)/include -I$(MY_FREETYPE_PATH)/objs -I$(MY_FREETYPE_PATH)/builds/unix -DFT_CONFIG_OPTION_SYSTEM_ZLIB -DFT_CONFIG_CONFIG_H="<ftconfig.h>" -DFT2_BUILD_LIBRARY -DFT_CONFIG_MODULES_H="<ftmodule.h>" 
LOCAL_LDLIBS := 
LOCAL_SRC_FILES += $(addprefix $(MY_FREETYPE_PATH)/,builds/unix/ftsystem.c)
LOCAL_SRC_FILES += $(addprefix $(MY_FREETYPE_PATH)/,src/base/ftdebug.c)
LOCAL_SRC_FILES += $(addprefix $(MY_FREETYPE_PATH)/,src/base/ftinit.c)
LOCAL_SRC_FILES += $(addprefix $(MY_FREETYPE_PATH)/,src/base/ftbase.c)
LOCAL_SRC_FILES += $(addprefix $(MY_FREETYPE_PATH)/,src/base/ftglyph.c)
LOCAL_SRC_FILES += $(addprefix $(MY_FREETYPE_PATH)/,src/base/ftstroke.c)
LOCAL_SRC_FILES += $(addprefix $(MY_FREETYPE_PATH)/,src/base/ftbitmap.c)
LOCAL_SRC_FILES += $(addprefix $(MY_FREETYPE_PATH)/,src/truetype/truetype.c)
LOCAL_SRC_FILES += $(addprefix $(MY_FREETYPE_PATH)/,src/autofit/autofit.c)
LOCAL_SRC_FILES += $(addprefix $(MY_FREETYPE_PATH)/,src/smooth/smooth.c)
LOCAL_SRC_FILES += $(addprefix $(MY_FREETYPE_PATH)/,src/sfnt/sfnt.c)
LOCAL_SRC_FILES += $(addprefix $(MY_FREETYPE_PATH)/,src/psnames/psmodule.c)
include $(BUILD_STATIC_LIBRARY)

# Build the png library
include $(CLEAR_VARS)
LOCAL_MODULE := gdpng
MY_PNG_PATH := libpng-1.4.4
LOCAL_CFLAGS :=  -DHAVE_CONFIG_H -I$(MY_PNG_PATH) -DPNG_CONFIGURE_LIBPNG 
LOCAL_LDLIBS := 
LOCAL_SRC_FILES += $(addprefix $(MY_PNG_PATH)/,png.c)
LOCAL_SRC_FILES += $(addprefix $(MY_PNG_PATH)/,pngset.c)
LOCAL_SRC_FILES += $(addprefix $(MY_PNG_PATH)/,pngget.c)
LOCAL_SRC_FILES += $(addprefix $(MY_PNG_PATH)/,pngrutil.c)
LOCAL_SRC_FILES += $(addprefix $(MY_PNG_PATH)/,pngtrans.c)
LOCAL_SRC_FILES += $(addprefix $(MY_PNG_PATH)/,pngwutil.c)
LOCAL_SRC_FILES += $(addprefix $(MY_PNG_PATH)/,pngread.c)
LOCAL_SRC_FILES += $(addprefix $(MY_PNG_PATH)/,pngrio.c)
LOCAL_SRC_FILES += $(addprefix $(MY_PNG_PATH)/,pngwio.c)
LOCAL_SRC_FILES += $(addprefix $(MY_PNG_PATH)/,pngwrite.c)
LOCAL_SRC_FILES += $(addprefix $(MY_PNG_PATH)/,pngrtran.c)
LOCAL_SRC_FILES += $(addprefix $(MY_PNG_PATH)/,pngwtran.c)
LOCAL_SRC_FILES += $(addprefix $(MY_PNG_PATH)/,pngmem.c)
LOCAL_SRC_FILES += $(addprefix $(MY_PNG_PATH)/,pngerror.c)
LOCAL_SRC_FILES += $(addprefix $(MY_PNG_PATH)/,pngpread.c)
include $(BUILD_STATIC_LIBRARY)

# Build the application core
include $(CLEAR_VARS)
LOCAL_MODULE := gdcore
MY_GD_ROOT := GeoDiscoverer
MY_GD_ABS_ROOT := $(shell cd $(MY_GD_ROOT) && pwd)
MY_GD_PLATFORM_SRCS += $(shell find $(MY_GD_ROOT)/Source/Platform/Feature/POSIX -name '*.cpp')
MY_GD_PLATFORM_SRCS += $(shell find $(MY_GD_ROOT)/Source/Platform/Feature/OpenGLES -name '*.cpp')
MY_GD_PLATFORM_SRCS += $(shell find $(MY_GD_ROOT)/Source/Platform/Feature/libjpeg -name '*.cpp')
MY_GD_PLATFORM_SRCS += $(shell find $(MY_GD_ROOT)/Source/Platform/Feature/libxml2 -name '*.cpp')
MY_GD_PLATFORM_SRCS += $(shell find $(MY_GD_ROOT)/Source/Platform/Feature/libfreetype2 -name '*.cpp')
MY_GD_PLATFORM_SRCS += $(shell find $(MY_GD_ROOT)/Source/Platform/Feature/libpng -name '*.cpp')
MY_GD_PLATFORM_SRCS += $(shell find $(MY_GD_ROOT)/Source/Platform/Feature/Android -name '*.cpp')
MY_GD_GENERAL_SRCS += $(shell find $(MY_GD_ROOT)/Source/General -name '*.cpp')
MY_GD_INCLUDES += -I$(MY_GD_ROOT)/Source/Platform/Feature/POSIX
MY_GD_INCLUDES += -I$(MY_GD_ROOT)/Source/Platform/Feature/OpenGLES
MY_GD_INCLUDES += -I$(MY_GD_ROOT)/Source/Platform/Feature/libjpeg
MY_GD_INCLUDES += -I$(MY_GD_ROOT)/Source/Platform/Feature/libxml2
MY_GD_INCLUDES += -I$(MY_GD_ROOT)/Source/Platform/Feature/libfreetype2
MY_GD_INCLUDES += -I$(MY_GD_ROOT)/Source/Platform/Feature/libpng
MY_GD_INCLUDES += -I$(MY_GD_ROOT)/Source/Platform/Feature/Android
MY_GD_INCLUDES += -I$(MY_GD_ROOT)/Source/General/App
MY_GD_INCLUDES += -I$(MY_GD_ROOT)/Source/General/Map
MY_GD_INCLUDES += -I$(MY_GD_ROOT)/Source/General/Navigation
MY_GD_INCLUDES += -I$(MY_GD_ROOT)/Source/General/Image
MY_GD_INCLUDES += -I$(MY_GD_ROOT)/Source/General/Graphic
MY_GD_INCLUDES += -I$(MY_GD_ROOT)/Source/General/Widget
MY_GD_INCLUDES += -I$(MY_GD_ROOT)/Source/General/Math
MY_GD_INCLUDES += -I$(MY_GD_ROOT)/Source/General/Config
MY_GD_INCLUDES += -I$(MY_GD_ROOT)/Source/General/Profile
LOCAL_CFLAGS += $(MY_GD_INCLUDES) -DTARGET_ANDROID -Ilibxml2-2.7.7/include -Ifreetype-2.4.2/include -Ijpeg-8b -Ilibpng-1.4.4 -DSRC_ROOT='"$(MY_GD_ABS_ROOT)/Source"' 
LOCAL_LDLIBS += -lz -dl -llog -lGLESv1_CM
LOCAL_SRC_FILES := GDCore.cpp $(MY_GD_PLATFORM_SRCS) $(MY_GD_GENERAL_SRCS)
LOCAL_STATIC_LIBRARIES := gdjpeg gdxml gdfreetype gdpng
LOCAL_SHARED_LIBRARIES := 
include $(BUILD_SHARED_LIBRARY)
