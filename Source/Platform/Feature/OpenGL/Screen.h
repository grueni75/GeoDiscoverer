//============================================================================
// Name        : Screen.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2016 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// GeoDiscoverer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GeoDiscoverer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GeoDiscoverer.  If not, see <http://www.gnu.org/licenses/>.
//
//============================================================================

#ifndef SCREEN_H_
#define SCREEN_H_

#ifdef TARGET_ANDROID
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif
#ifdef TARGET_LINUX
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/freeglut.h>
#endif
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#ifdef TARGET_ANDROID
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "OpenGLES3Stub.h"
#endif

namespace GEODISCOVERER {

// Data types
typedef GLuint GraphicTextureInfo;
typedef GLuint GraphicBufferInfo;
enum GraphicTextureFormat { GraphicTextureFormatRGB565, GraphicTextureFormatRGBA5551, GraphicTextureFormatRGBA4444, GraphicTextureFormatRGB888, GraphicTextureFormatRGBA8888  };
enum GraphicScreenOrientation { GraphicScreenOrientationProtrait = 0, GraphicScreenOrientationLandscape = 1  };
typedef struct {
  GraphicTextureInfo textureInfo;
  std::string source;
} TextureDebugInfo;

// Manages access to the screen
class Screen {

protected:

  // Pointer to the device this screen belongs to
  Device *device;

  // Value indicating that no texture is available
  static const GraphicTextureInfo textureNotDefined = 0;

  // Value indicating that no buffer is available
  const static GraphicBufferInfo bufferNotDefined = 0;

  // Number of segments to use for ellipse drawing
  static const int ellipseSegments=32;

  // Holds the coordinates for drawing the ellipse
  GraphicBufferInfo ellipseCoordinatesBuffer;

  // List of cached texture infos
  std::list<TextureDebugInfo> unusedTextureInfos;

  // List of cached buffer infos
  std::list<GraphicBufferInfo> unusedBufferInfos;

  // Current width and height of the screen
  Int width;
  Int height;

  // Indicates if wake lock is on or off
  bool wakeLock;

  // Orientation of the screen
  GraphicScreenOrientation orientation;

  // Decides if resource destroying is allowed
  bool allowDestroying;

  // Decides if resource allocation is allowed
  bool allowAllocation;

  // Indicates that this screen is drawing into a buffer
  bool separateFramebuffer;

  // Framebuffer ID
  GLuint framebuffer;

  // Color renderbuffer ID
  GLuint colorRenderbuffer;

  // Pixel buffer ID
  GLuint pixelBuffer;

  // Size of the pixel buffer
  GLsizeiptr pixelBufferSize;

  // Screen shot pixel buffer
  GLvoid *screenShotPixel;

  // Matrixes for OpenGL transformations
  glm::mat4x4 viewMatrix;
  glm::mat4x4 projectionMatrix;
  glm::mat4x4 modelMatrix;
  glm::mat4x4 vpMatrix;
  glm::mat4x4 mvpMatrix;

  // Stack of matrixes that are used for drawing
  std::list<glm::mat4x4 > modelMatrixStack;

  // Programs for the vertex and fragment shaders
  static const char *vertexShaderProgram;
  static const char *fragmentShaderProgram;

  // Handles for the shaders
  GLuint vertexShaderHandle;
  GLuint fragmentShaderHandle;
  GLuint shaderProgramHandle;
  GLint mvpMatrixHandle;
  GLint colorInHandle;
  GLint positionInHandle;
  GLint textureCoordinateInHandle;
  GLint textureImageInHandle;
  GLint textureEnabledHandle;

  // Compiles a shader program
  void compileShaderProgram(GLuint handle, const char *program);

  // Width of the line
  Int lineWidth;

  // Drawing color
  glm::vec4 drawingColor;

  // Checks if an extension is available
  bool queryExtension(const char *extName);

  // Indicates if RGBA8888 image format can be used
  GraphicTextureInfo testTextureInfo;
  bool textureFormatRGBA8888Supported;
  bool textureFormatRGB888Supported;

  // Indicates if OpenGL ES 3.0 is available
  bool openglES30Supported;

#ifdef TARGET_ANDROID
  // EGL context
  static EGLConfig eglConfig;
  static EGLSurface eglSurface;
  static EGLContext eglContext;
  static EGLDisplay eglDisplay;
#endif

public:

  // Constructor
  Screen(Device *device);

  // Destructor
  virtual ~Screen();

  // Setups the EGL context
  static bool setupContext();

  // Destroys the EGL context
  static void shutdownContext();

  // Inits the screen
  void init(GraphicScreenOrientation orientation, Int width, Int height);

  // Activates the screen for drawing
  void startScene();

  // Creates a screen shot
  bool createScreenShot();

  // Writes the screen content as a png
  void writePNG(std::string path);

  // Clears the screen
  void clear();

  // Starts a new object
  void startObject();

  // Sets the line width for drawing operations
  void setLineWidth(Int width);

  // Ends the current object
  void endObject();

  // Rotates the scene
  void rotate(double angle, Int x, Int y, Int z);

  // Scales the scene
  void scale(double x, double y, double z);

  // Translates the scene
  void translate(Int x, Int y, Int z);

  // Sets the drawing color
  void setColor(UByte r, UByte g, UByte b, UByte a);

  // Sets color mode such that alpha channel of primitive determines its transparency
  void setColorModeAlpha();

  // Sets color mode such that primitive color is multiplied with background color
  void setColorModeMultiply();

  // Draws a rectangle
  void drawRectangle(Int x1,Int y1, Int x2, Int y2, GraphicTextureInfo texture, bool filled);

  // Draws multiple triangles
  void drawTriangles(Int numberOfTriangles, GraphicBufferInfo pointCoordinatesBuffer, GraphicTextureInfo textureInfo=textureNotDefined, GraphicBufferInfo textureCoordinatesBuffer=bufferNotDefined);

  // Draws a ellipse
  void drawEllipse(bool filled);

  // Finished the drawing of the scene
  void endScene();

  // Returns a new texture id
  GraphicTextureInfo createTextureInfo();

  // Sets the image of a texture
  bool setTextureImage(GraphicTextureInfo texture, UByte *image, Int width, Int height, GraphicTextureFormat format=GraphicTextureFormatRGB565);

  // Frees a texture id
  void destroyTextureInfo(GraphicTextureInfo i, std::string source);

  // Returns a new buffer id
  GraphicBufferInfo createBufferInfo();

  // Sets the data of an array buffer
  void setArrayBufferData(GraphicBufferInfo buffer, Byte *data, Int size);

  // Frees an buffer id
  void destroyBufferInfo(GraphicBufferInfo buffer);

  // If set to one, the screen is not turned off
  void setWakeLock(bool state, const char *file, int line, bool persistent=true);

  // Frees any internal textures or buffers
  void graphicInvalidated(bool contextLost);

  // Creates the graphic
  void createGraphic();

  // Destroys the graphic
  void destroyGraphic();

  // Main loop that handles events
  void mainLoop();

  // Getters and setters
  static const GraphicBufferInfo getBufferNotDefined()
  {
      return bufferNotDefined;
  }

  static const GraphicTextureInfo getTextureNotDefined()
  {
      return textureNotDefined;
  }

  Int getHeight() const
  {
      return height;
  }

  Int getWidth() const
  {
      return width;
  }

  GraphicScreenOrientation getOrientation() const
  {
      return orientation;
  }

  bool getWakeLock() const
  {
      return wakeLock;
  }

  void setAllowDestroying(bool allowDestroying)
  {
      this->allowDestroying=allowDestroying;
  }

  void setAllowAllocation(bool allowAllocation)
  {
      this->allowAllocation=allowAllocation;
  }

  bool isTextureFormatRGB888Supported() const {
    return textureFormatRGB888Supported;
  }

  bool isTextureFormatRGBA8888Supported() const {
    return textureFormatRGBA8888Supported;
  }

  double getDiagonal() const;

  Int getDPI() const;

};

}

#endif /* SCREEN_H_ */
