//============================================================================
// Name        : Screen.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
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

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/freeglut.h>

namespace GEODISCOVERER {

// Data types
typedef GLuint GraphicBufferInfo;
typedef GLuint GraphicTextureInfo;
enum GraphicTextureFormat { graphicTextureFormatRGB, graphicTextureFormatRGBA1, graphicTextureFormatRGBA4  };
enum GraphicScreenOrientation { graphicScreenOrientationProtrait, graphicScreenOrientationLandscape  };

// Manages access to the screen
class Screen {

protected:

  // Density of the screen
  Int DPI;

  // Indicates if wake lock is on or off
  bool wakeLock;

  // Number of steps to approximate an ellipse
  const static Int ellipseSegments = 32;

  // Value indicating that no texture is available
  const static GraphicTextureInfo textureNotDefined = 0;

  // Value indicating that no buffer is available
  const static GraphicBufferInfo bufferNotDefined = 0;

  // Decides if texture/buffer destroying is allowed
  bool allowDestroying;

public:

  // Constructor: Init screen (show window)
  Screen(Int DPI);

  // Inits the screen
  void init(GraphicScreenOrientation orientation, Int width, Int height);

  // Gets the width of the screen
  Int getWidth();

  // Gets the height of the screen
  Int getHeight();

  // Gets the orientation of the screen
  GraphicScreenOrientation getOrientation();

  // Main loop that handles events
  void mainLoop();

  // Destructor: clean up everything (close window)
  virtual ~Screen();

  // Clears the screen
  void clear();

  // Starts a new object
  void startObject();

  // Resets all scaling, translation and rotation parameters of the object
  void resetObject();

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

  // Sets color mode such that alpha channel of primitive determines its transparency
  void setColorModeAlpha();

  // Sets color mode such that primitive color is multiplied with background color
  void setColorModeMultiply();

  // Sets the drawing color
  void setColor(UByte r, UByte g, UByte b, UByte a);

  // Draws a rectangle
  void drawRectangle(Int x1,Int y1, Int x2, Int y2, GraphicTextureInfo texture, bool filled);

  // Draws multiple triangles
  void drawTriangles(Int numberOfTriangles, GraphicBufferInfo pointCoordinatesBuffer, GraphicTextureInfo textureInfo=textureNotDefined, GraphicBufferInfo textureCoordinatesBuffer=bufferNotDefined);

  // Draws a line
  void drawLine(Int x1, Int y1, Int x2, Int y2);

  // Draws a line consisting of multiple points
  void drawLines(Int numberOfPoints, Short *pointArray);

  // Draws a ellipse
  void drawEllipse(bool filled);

  // Finished the drawing of the scene
  void endScene();

  // Returns a new texture id
  GraphicTextureInfo createTextureInfo();

  // Sets the image of a texture
  void setTextureImage(GraphicTextureInfo texture, UShort *image, Int width, Int height, GraphicTextureFormat format=graphicTextureFormatRGB);

  // Frees a texture id
  void destroyTextureInfo(GraphicTextureInfo i);

  // Returns a new buffer id
  GraphicBufferInfo createBufferInfo();

  // Sets the data of an array buffer
  void setArrayBufferData(GraphicBufferInfo buffer, Byte *data, Int size);

  // Frees an buffer id
  void destroyBufferInfo(GraphicBufferInfo buffer);

  // If set to one, the screen is not turned off
  void setWakeLock(bool state);

  // Frees any internal textures or buffers
  void graphicInvalidated();

  // Getters and setters
  const GraphicTextureInfo getTextureNotDefined()
  {
      return textureNotDefined;
  }

  const GraphicBufferInfo getBufferNotDefined()
  {
      return bufferNotDefined;
  }

  Int getDPI() const
  {
      return DPI;
  }

  bool getWakeLock() const
  {
      return wakeLock;
  }

  void setAllowDestroying(bool allowDestroying)
  {
      this->allowDestroying=allowDestroying;
  }

};

}

#endif /* SCREEN_H_ */
