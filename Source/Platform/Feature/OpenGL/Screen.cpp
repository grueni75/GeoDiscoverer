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

int frameCountBeforeTextureInvalidation=50;

// Proxy functions
void displayFunc()
{
  GEODISCOVERER::core->updateScreen(false);
}
void keyboardFunc(GLubyte key, GLint x, GLint y)
{
  switch(key) {
  case '+':
    GEODISCOVERER::core->getCommander()->execute("zoom(1.125)");
    break;
  case '-':
    GEODISCOVERER::core->getCommander()->execute("zoom(0.875)");
    break;
  case 'j':
    GEODISCOVERER::core->getCommander()->execute("pan(-5,0)");
    break;
  case 'k':
    GEODISCOVERER::core->getCommander()->execute("pan(+5,0)");
    break;
  case 'm':
    GEODISCOVERER::core->getCommander()->execute("pan(0,-5)");
    break;
  case 'i':
    GEODISCOVERER::core->getCommander()->execute("pan(0,+5)");
    break;
  case 'a':
    GEODISCOVERER::core->getCommander()->execute("rotate(-1)");
    break;
  case 's':
    GEODISCOVERER::core->getCommander()->execute("rotate(+1)");
    break;
  case 'q':
    glutLeaveMainLoop();
    break;
  default:
    break;
  }
}
void idleFunc() {
  usleep(16666);
  if (frameCountBeforeTextureInvalidation==0) {
    GEODISCOVERER::core->graphicInvalidated();
    frameCountBeforeTextureInvalidation--;
  } else {
    if (frameCountBeforeTextureInvalidation>0)
      frameCountBeforeTextureInvalidation--;
  }
  glutPostRedisplay();
}
bool mouseActive=false;
void mouseFunc(int button, int state, int x, int y)
{
  if (button==GLUT_LEFT_BUTTON) {
    std::stringstream s;
    bool sendCommand=false;
    switch(state) {
      case GLUT_DOWN:
        s << "touchDown(";
        sendCommand=true;
        mouseActive=true;
        break;
      case GLUT_UP:
        s << "touchUp(";
        sendCommand=true;
        mouseActive=false;
        break;
    }
    if (sendCommand) {
      s << x << "," << y << ")";
      GEODISCOVERER::core->getCommander()->execute(s.str());
    }

  }
}
void motionFunc(int x, int y)
{
  if (mouseActive) {
    std::stringstream s;
    s << "touchMove(" << x << "," << y << ")";
    GEODISCOVERER::core->getCommander()->execute(s.str());
  }
}

namespace GEODISCOVERER {

// Constructor: open window and init opengl
Screen::Screen(Int DPI) {
  int argc = 0;
  char **argv = NULL;

  // Set variables
  this->allowDestroying=false;
  this->DPI=DPI;
  this->wakeLock=core->getConfigStore()->getIntValue("Screen","wakeLock","Indicates if the screen backlight is kept on",0);
  setWakeLock(wakeLock);

  // Open window
  glutInit(&argc, argv);
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(getWidth(), getHeight());
  glutInitWindowPosition(600, 50);
  glutCreateWindow("GeoDiscoverer");
  glutDisplayFunc(displayFunc);
  glutKeyboardFunc(keyboardFunc);
  glutIdleFunc(idleFunc);
  glutMouseFunc(mouseFunc);
  glutMotionFunc(motionFunc);

  // Init display
  glViewport(0, 0, (GLsizei) getWidth(), (GLsizei) getHeight());
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-getWidth() / 2.0, getWidth() / 2.0, -getHeight() / 2.0, getHeight() / 2.0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );      // select modulate to mix texture with color for shading

  // Enable transparency
  glEnable (GL_BLEND);
  //glEnable (GL_LINE_SMOOTH);
  setColorModeAlpha();
}

// Main loop
void Screen::mainLoop() {
  glutMainLoop();
}

// Get the width of the screen
Int Screen::getWidth() {
  if (getOrientation()==graphicScreenOrientationProtrait)
    return 480;
  else
    return 768;
}

// Get the height of the screen
Int Screen::getHeight() {
  if (getOrientation()==graphicScreenOrientationProtrait)
    return 768;
  else
    return 480;
}

// Gets the orientation of the screen
GraphicScreenOrientation Screen::getOrientation() {
  //return graphicScreenOrientationProtrait;
  return graphicScreenOrientationLandscape;
}

// Inits the screen
void Screen::init(GraphicScreenOrientation orientation, Int width, Int height) {
}

void Screen::clear() {
  glClear(GL_COLOR_BUFFER_BIT);
}

// Starts a new object
void Screen::startObject() {
  glPushMatrix();
}

// Resets all scaling, translation and rotation parameters of the object
void Screen::resetObject() {
  glLoadIdentity();
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

// Draws a single rectangle
void Screen::drawRectangle(Int x1, Int y1, Int x2, Int y2, GraphicTextureInfo texture, bool filled) {
  double textureX1=0.0;
  double textureY1=1.0;
  double textureX2=1.0;
  double textureY2=0.0;
  if (texture!=Screen::getTextureNotDefined()) {
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, texture );
    glBegin( GL_QUADS );
    glTexCoord2d(textureX1,textureY2); glVertex2d(x1,y2);
    glTexCoord2d(textureX2,textureY2); glVertex2d(x2,y2);
    glTexCoord2d(textureX2,textureY1); glVertex2d(x2,y1);
    glTexCoord2d(textureX1,textureY1); glVertex2d(x1,y1);
    glEnd();
    glDisable( GL_TEXTURE_2D );
  } else {
    if (filled) {
      glBegin( GL_QUADS );
      glVertex2d(x1,y2);
      glVertex2d(x2,y2);
      glVertex2d(x2,y1);
      glVertex2d(x1,y1);
      glEnd();
    } else {
      glBegin(GL_LINES);
      glVertex3f(x1, y1, 0);
      glVertex3f(x2, y1, 0);
      glVertex3f(x2, y1, 0);
      glVertex3f(x2, y2, 0);
      glVertex3f(x2, y2, 0);
      glVertex3f(x1, y2, 0);
      glVertex3f(x1, y2, 0);
      glVertex3f(x1, y1, 0);
      glEnd();
    }
  }
}

// Draws multiple triangles
void Screen::drawTriangles(Int numberOfTriangles, GraphicBufferInfo pointCoordinatesBuffer, GraphicTextureInfo textureInfo, GraphicBufferInfo textureCoordinatesBuffer) {
  glEnableClientState(GL_VERTEX_ARRAY);
  glBindBuffer(GL_ARRAY_BUFFER, pointCoordinatesBuffer);
  glVertexPointer(2, GL_SHORT, 0, 0);
  if (textureInfo!=textureNotDefined) {
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, textureInfo );
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, textureCoordinatesBuffer);
    glTexCoordPointer(2, GL_SHORT, 0, 0);
  }
  glDrawArrays(GL_TRIANGLES, 0, numberOfTriangles*3);
  glDisableClientState(GL_VERTEX_ARRAY);
  if (textureInfo!=textureNotDefined) {
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable( GL_TEXTURE_2D );
  }
}

// Draws a line
void Screen::drawLine(Int x1, Int y1, Int x2, Int y2) {
  glBegin(GL_LINES);
  glVertex2f(x1, y1);
  glVertex2f(x2, y2);
  glEnd();
  glBegin(GL_POINTS);
  glVertex2f(x1, y1);
  glVertex2f(x2, y2);
  glEnd();
}

// Draws a line consisting of multiple points
void Screen::drawLines(Int numberOfPoints, Short *pointArray) {
  glBegin(GL_LINES);
  for(int i=0;i<numberOfPoints;i++) {
    glVertex2f(pointArray[2*i], pointArray[2*i+1]);
  }
  glEnd();
  glBegin(GL_POINTS);
  for(int i=0;i<numberOfPoints;i++) {
    glVertex2f(pointArray[2*i], pointArray[2*i+1]);
  }
  glEnd();
}

// Draws a ellipse
void Screen::drawEllipse(bool filled) {
  if (filled) {
    glBegin(GL_POLYGON);
  } else {
    glBegin(GL_LINE_LOOP);
  }
  for(double t = 0; t <= 2*M_PI; t += 2*M_PI/ellipseSegments)
    glVertex2f(cos(t), sin(t));
  glEnd();

}

// Finished the drawing of the scene
void Screen::endScene() {
  glFlush();
  glutSwapBuffers();
}

// Creates a new texture id
GraphicTextureInfo Screen::createTextureInfo() {
  GraphicTextureInfo i;
  glGenTextures( 1, &i );
  if (i==Screen::textureNotDefined) // if the texture id matches the special value, generate a new one
    FATAL("texture info generated that matches the special textureNotDefined info",NULL);
  GLenum error=glGetError();
  if (error!=GL_NO_ERROR)
    FATAL("can not generate texture",NULL);
  return i;
}

// Sets the image of a texture
void Screen::setTextureImage(GraphicTextureInfo texture, UShort *image, Int width, Int height, GraphicTextureFormat format) {
  glBindTexture(GL_TEXTURE_2D,texture);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
    FATAL("can not update texture image",NULL);
  }
}

// Frees a texture id
void Screen::destroyTextureInfo(GraphicTextureInfo i) {
  if (allowDestroying) {
    glDeleteTextures(1,&i);
    GLenum error=glGetError();
    if (error!=GL_NO_ERROR)
      FATAL("can not delete texture (error=0x%08x)",error);
  } else {
    FATAL("texture destroying has been disallowed",NULL);
  }
}

// Returns a new buffer id
GraphicBufferInfo Screen::createBufferInfo() {
  GraphicBufferInfo buffer;
  glGenBuffers( 1, &buffer );
  if (buffer==Screen::bufferNotDefined) // if the buffer id matches the special value, generate a new one
    FATAL("buffer info generated that matches the special textureNotDefined info",NULL);
  GLenum error=glGetError();
  if (error!=GL_NO_ERROR)
    FATAL("can not generate buffer",NULL);
  return buffer;
}

// Sets the data of an array buffer
void Screen::setArrayBufferData(GraphicBufferInfo buffer, Byte *data, Int size) {
  glBindBuffer(GL_ARRAY_BUFFER,buffer);
  glBufferData(GL_ARRAY_BUFFER,size,data,GL_STATIC_DRAW);
}

// Frees an buffer id
void Screen::destroyBufferInfo(GraphicBufferInfo buffer) {
  if (allowDestroying) {
    glDeleteBuffers(1,&buffer);
    GLenum error=glGetError();
    if (error!=GL_NO_ERROR)
      FATAL("can not delete buffer (error=0x%08x)",error);
  } else {
    FATAL("buffer destroying has been disallowed",NULL);
  }
}

// If set to one, the screen is not turned off
void Screen::setWakeLock(bool state) {
  core->getConfigStore()->setIntValue("Screen","wakeLock",state);
}

// Frees any internal textures or buffers
void Screen::graphicInvalidated() {
}

// Destructor
Screen::~Screen() {
  graphicInvalidated();
}

}