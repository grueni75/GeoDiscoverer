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

#include <GLES/gl.h>
#include <GLES/glext.h>

namespace GEODISCOVERER {

// Data types
typedef GLuint GraphicTextureInfo;
typedef GLuint GraphicBufferInfo;
enum GraphicTextureFormat { graphicTextureFormatRGB, graphicTextureFormatRGBA1, graphicTextureFormatRGBA4  };
enum GraphicScreenOrientation { graphicScreenOrientationProtrait, graphicScreenOrientationLandscape  };
typedef struct {
  GraphicTextureInfo textureInfo;
  std::string source;
} TextureDebugInfo;

// Manages access to the screen
class Screen {

protected:

  // Value indicating that no texture is available
  static const  GraphicTextureInfo textureNotDefined = 0;

  // Value indicating that no buffer is available
  const static GraphicBufferInfo bufferNotDefined = 0;

  // Number of segments to use for ellipse drawing
  static const int ellipseSegments=32;

  // Holds the coordinates for drawing the ellipse
  GraphicBufferInfo ellipseCoordinatesBuffer;

  // List of cached texture infos
  static std::list<TextureDebugInfo> unusedTextureInfos;

  // List of cached buffer infos
  static std::list<GraphicBufferInfo> unusedBufferInfos;

  // Current width and height of the screen
  Int width;
  Int height;

  // Density of the screen
  Int DPI;

  // Indicates if wake lock is on or off
  bool wakeLock;

  // Orientation of the screen
  GraphicScreenOrientation orientation;

  // Decides if resource destroying is allowed
  bool allowDestroying;

  // Decides if resource allocation is allowed
  bool allowAllocation;

public:

  // Constructor
  Screen(Int DPI);

  // Destructor
  virtual ~Screen();

  // Inits the screen
  void init(GraphicScreenOrientation orientation, Int width, Int height);

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
  void graphicInvalidated();

  // Creates the graphic
  void createGraphic();

  // Destroys the graphic
  void destroyGraphic();

  // Getters and setters
  const GraphicBufferInfo getBufferNotDefined()
  {
      return bufferNotDefined;
  }

  const GraphicTextureInfo getTextureNotDefined()
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

  Int getDPI() const
  {
      return DPI;
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

};

}

#endif /* SCREEN_H_ */
