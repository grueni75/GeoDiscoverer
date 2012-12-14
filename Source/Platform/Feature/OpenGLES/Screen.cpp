//============================================================================
// Name        : Screen.cpp
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

#include <Core.h>

namespace GEODISCOVERER {

// List of cached texture infos
std::list<TextureDebugInfo> Screen::unusedTextureInfos;

// List of cached buffer infos
std::list<GraphicBufferInfo> Screen::unusedBufferInfos;

// Constructor: open window and init opengl
Screen::Screen(Int DPI) {
  this->allowDestroying=false;
  this->allowAllocation=false;
  this->DPI=DPI;
  this->wakeLock=core->getConfigStore()->getIntValue("General","wakeLock");
  this->ellipseCoordinatesBuffer=bufferNotDefined;
}

// Inits the screen
void Screen::init(GraphicScreenOrientation orientation, Int width, Int height) {

  // Update variables
  this->width=width;
  this->height=height;
  this->orientation=orientation;
  DEBUG("dpi=%d width=%d height=%d",DPI,width,height);

  // Init display
  glClearColor(0.0f,0.0f,0.0f,1.0f);
  glViewport(0, 0, (GLsizei) getWidth(), (GLsizei) getHeight());
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrthof(-getWidth() / 2.0, getWidth() / 2.0, -getHeight() / 2.0, getHeight() / 2.0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Set texture parameter
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );      // select modulate to mix texture with color for shading

  // Enable transparency
  glEnable (GL_BLEND);
  setColorModeAlpha();

  // Compute the maximum tiles to show
  core->getMapEngine()->setMaxTiles();

  /*for (int index=0;index<ellipseSegments;index++) {
    DEBUG("ellipseSegmentPoints[%d]=%f",index,this->ellipseSegmentPoints[index]);
  }*/
}

// Clears the scene
void Screen::clear() {
  glClear(GL_COLOR_BUFFER_BIT);
}

// Starts a new object
void Screen::startObject() {
  glPushMatrix();
}

// Sets the line width for drawing operations
void Screen::setLineWidth(Int width) {
  glLineWidth(width);
  glPointSize(width);
}

// Ends the current object
void Screen::endObject() {
  glPopMatrix();
}

// Rotates the scene
void Screen::rotate(double angle, Int x, Int y, Int z) {
  glRotatef(angle, x, y, z);
}

// Scales the scene
void Screen::scale(double x, double y, double z) {
  glScalef(x, y, z);
}

// Translates the scene
void Screen::translate(Int x, Int y, Int z) {
  glTranslatef(x, y, z);
}

// Sets the drawing color
void Screen::setColor(UByte r, UByte g, UByte b, UByte a) {
  glColor4f((GLfloat) (r) / 255.0, (GLfloat) (g) / 255.0, (GLfloat) (b) / 255.0, (GLfloat) (a) / 255.0);
}

// Sets color mode such that alpha channel of primitive determines its transparency
void Screen::setColorModeAlpha() {
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Sets color mode such that primitive color is multiplied with background color
void Screen::setColorModeMultiply() {
  glBlendFunc (GL_ZERO, GL_SRC_COLOR);
}

// Draws a rectangle
void Screen::drawRectangle(Int x1, Int y1, Int x2, Int y2, GraphicTextureInfo texture, bool filled) {
  GLfloat box[] = {x1,y1, x2,y1, x1,y2, x2,y2 };
  if (texture!=Screen::getTextureNotDefined()) {
    GLfloat tex[] = {0.0,1.0, 1.0,1.0, 0.0,0.0, 1.0,0.0};
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, texture );
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0,box);
    glTexCoordPointer(2, GL_FLOAT, 0, tex);
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable( GL_TEXTURE_2D );
  } else {
    if (filled) {
      glEnableClientState(GL_VERTEX_ARRAY);
      glVertexPointer(2,GL_FLOAT,0,box);
      glDrawArrays(GL_TRIANGLE_STRIP,0,4);
      glDisableClientState(GL_VERTEX_ARRAY);
    } else {
      drawLine(x1,y1,x2,y1);
      drawLine(x2,y1,x2,y2);
      drawLine(x2,y2,x1,y2);
      drawLine(x1,y2,x1,y1);
    }
  }
}

// Draws a line
void Screen::drawLine(Int x1, Int y1, Int x2, Int y2) {
  GLfloat line[] = {
      x1, y1,
      x2, y2
  };
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, line);
  glDrawArrays(GL_LINES, 0, 2);
  glDrawArrays(GL_POINTS, 0, 2);
  glDisableClientState(GL_VERTEX_ARRAY);
}

// Draws a line consisting of multiple points
void Screen::drawLines(Int numberOfPoints, Short *pointArray) {
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_SHORT, 0, pointArray);
  glDrawArrays(GL_LINES, 0, numberOfPoints);
  glDrawArrays(GL_POINTS, 0, numberOfPoints);
  glDisableClientState(GL_VERTEX_ARRAY);
}

// Draws multiple triangles
void Screen::drawTriangles(Int numberOfTriangles, GraphicBufferInfo pointCoordinatesBuffer, GraphicTextureInfo textureInfo, GraphicBufferInfo textureCoordinatesBuffer) {
  glEnableClientState(GL_VERTEX_ARRAY);
  glBindBuffer(GL_ARRAY_BUFFER, pointCoordinatesBuffer);
  glVertexPointer(2, GL_SHORT, 0, 0);
  if (textureInfo!=textureNotDefined) {
    glEnable( GL_TEXTURE_2D );
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, textureCoordinatesBuffer);
    glTexCoordPointer(2, GL_SHORT, 0, 0);
    glBindTexture( GL_TEXTURE_2D, textureInfo );
  }
  glDrawArrays(GL_TRIANGLES, 0, numberOfTriangles*3);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  if (textureInfo!=textureNotDefined) {
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable( GL_TEXTURE_2D );
  }
  glDisableClientState(GL_VERTEX_ARRAY);
}

// Draws a ellipse
void Screen::drawEllipse(bool filled) {

  // Points prepared?
  if (ellipseCoordinatesBuffer==bufferNotDefined) {

    // No, create them
    ellipseCoordinatesBuffer=createBufferInfo();
    GLfloat *ellipseSegmentPoints=NULL;
    if (!(ellipseSegmentPoints=(GLfloat*)malloc(2*sizeof(*ellipseSegmentPoints)*ellipseSegments))) {
      FATAL("can not create ellipse segment points array",NULL);
      return;
    }
    Int index=0;
    for(double t = 0; t <= 2*M_PI; t += 2*M_PI/ellipseSegments) {
      ellipseSegmentPoints[index++]=cos(t);
      ellipseSegmentPoints[index++]=sin(t);
    }
    setArrayBufferData(ellipseCoordinatesBuffer,(Byte*)ellipseSegmentPoints,2*sizeof(*ellipseSegmentPoints)*ellipseSegments);
  }

  // Draw the ellipse
  glEnableClientState(GL_VERTEX_ARRAY);
  glBindBuffer(GL_ARRAY_BUFFER, ellipseCoordinatesBuffer);
  glVertexPointer(2, GL_FLOAT, 0, 0);
  if (filled) {
    glDrawArrays(GL_TRIANGLE_FAN,0,ellipseSegments);
  } else {
    glDrawArrays(GL_LINE_LOOP,0,ellipseSegments);
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDisableClientState(GL_VERTEX_ARRAY);
}

// Finished the drawing of the scene
void Screen::endScene() {
  glFlush();
}

// Creates a new texture id
GraphicTextureInfo Screen::createTextureInfo() {
  if (allowAllocation) {
    GraphicTextureInfo i;
    if (unusedTextureInfos.size()>0) {
      i=unusedTextureInfos.front().textureInfo;
      unusedTextureInfos.pop_front();
      //DEBUG("reusing existing texture info",NULL);
    } else {
      //DEBUG("creating new texture info",NULL);
      glGenTextures( 1, &i );
      if (i==Screen::textureNotDefined) // if the texture id matches the special value, generate a new one
        glGenTextures( 1, &i );
      GLenum error=glGetError();
      if (error!=GL_NO_ERROR)
        FATAL("can not generate texture (error=0x%08x)",error);
    }
    return i;
  } else {
    FATAL("texture allocation has been disallowed",NULL);
    return textureNotDefined;
  }
}

// Sets the image of a texture
void Screen::setTextureImage(GraphicTextureInfo texture, UShort *image, Int width, Int height, GraphicTextureFormat format) {

  //glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,texture);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  GLenum imageFormat;
  GLenum imageContents;
  GLenum imageDataType;
  switch(format) {
    case graphicTextureFormatRGB:
      imageFormat=GL_RGB;
      imageContents=GL_RGB;
      imageDataType=GL_UNSIGNED_SHORT_5_6_5;
      break;
    case graphicTextureFormatRGBA4:
      imageFormat=GL_RGBA;
      imageContents=GL_RGBA;
      imageDataType=GL_UNSIGNED_SHORT_4_4_4_4;
      break;
    case graphicTextureFormatRGBA1:
      imageFormat=GL_RGBA;
      imageContents=GL_RGBA;
      imageDataType=GL_UNSIGNED_SHORT_5_5_5_1;
      break;
    default:
      FATAL("unknown format",NULL);
      return;
  }
  glTexImage2D(GL_TEXTURE_2D, 0, imageFormat, width, height, 0, imageContents, imageDataType, image);
  GLenum error=glGetError();
  if (error!=GL_NO_ERROR) {
    FATAL("can not update texture image (error=0x%08x)",error);
  }
}

// Frees a texture id
void Screen::destroyTextureInfo(GraphicTextureInfo i, std::string source) {
  if (allowDestroying) {
    for(std::list<TextureDebugInfo>::iterator j=unusedTextureInfos.begin();j!=unusedTextureInfos.end();j++) {
      if (i==(*j).textureInfo) {
        FATAL("texture 0x%08x already destroyed (first destroy by %s, second destroy by %s)",i,(*j).source.c_str(),source.c_str());
      }
    }
    TextureDebugInfo t;
    t.textureInfo=i;
    t.source=source;
    unusedTextureInfos.push_back(t);
  } else {
    FATAL("texture destroying has been disallowed",NULL);
  }
}

// Returns a new buffer id
GraphicBufferInfo Screen::createBufferInfo() {
  if (allowAllocation) {
    GraphicBufferInfo buffer;
    if (unusedBufferInfos.size()>0) {
      buffer=unusedBufferInfos.front();
      unusedBufferInfos.pop_front();
      //DEBUG("reusing existing buffer info",NULL);
    } else {
      //DEBUG("creating new buffer info",NULL);
      glGenBuffers( 1, &buffer );
      if (buffer==Screen::bufferNotDefined) // if the buffer id matches the special value, generate a new one
        glGenBuffers( 1, &buffer );
      GLenum error=glGetError();
      if (error!=GL_NO_ERROR)
        FATAL("can not generate buffer",NULL);
    }
    //DEBUG("new buffer %d created",buffer);
    return buffer;
  } else {
    FATAL("buffer allocation has been disallowed",NULL);
    return bufferNotDefined;
  }
}

// Sets the data of an array buffer
void Screen::setArrayBufferData(GraphicBufferInfo buffer, Byte *data, Int size) {
  glBindBuffer(GL_ARRAY_BUFFER,buffer);
  glBufferData(GL_ARRAY_BUFFER,size,data,GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER,0);
}

// Frees an buffer id
void Screen::destroyBufferInfo(GraphicBufferInfo buffer) {
  if (allowDestroying) {
    for(std::list<GraphicBufferInfo>::iterator i=unusedBufferInfos.begin();i!=unusedBufferInfos.end();i++) {
      if (buffer==*i) {
        FATAL("buffer 0x%08x already destroyed!",buffer);
      }
    }
    unusedBufferInfos.push_back(buffer);
  } else {
    FATAL("buffer destroying has been disallowed",NULL);
  }
}

// Frees any internal textures or buffers
void Screen::graphicInvalidated() {
  if (allowDestroying) {
    //DEBUG("unusedTextureInfos.size()=%d",unusedTextureInfos.size());
    for(std::list<TextureDebugInfo>::iterator i=unusedTextureInfos.begin();i!=unusedTextureInfos.end();i++) {
      GraphicTextureInfo textureInfo=(*i).textureInfo;
      glDeleteTextures(1,&textureInfo);
    }
    unusedTextureInfos.clear();
    //DEBUG("unusedBufferInfos.size()=%d",unusedBufferInfos.size());
    for(std::list<GraphicBufferInfo>::iterator i=unusedBufferInfos.begin();i!=unusedBufferInfos.end();i++) {
      GraphicBufferInfo bufferInfo=*i;
      glDeleteBuffers(1,&bufferInfo);
    }
    unusedBufferInfos.clear();
  } else {
    FATAL("texture and buffer destroying has been disallowed",NULL);
  }
}

// Creates the graphic
void Screen::createGraphic() {
}

// Destroys the graphic
void Screen::destroyGraphic() {
  if (ellipseCoordinatesBuffer!=bufferNotDefined) {
    destroyBufferInfo(ellipseCoordinatesBuffer);
    ellipseCoordinatesBuffer=bufferNotDefined;
  }
}

// Destructor
Screen::~Screen() {
}

}
