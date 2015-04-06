//============================================================================
// Name        : Screen.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#include <Core.h>

int frameCountBeforeTextureInvalidation=30;
bool graphicInitialized=false;

// Proxy functions
void displayFunc()
{
  if (!graphicInitialized) {
    GEODISCOVERER::core->updateGraphic(false);
    graphicInitialized=true;
  }
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
    GEODISCOVERER::core->updateGraphic(false);
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

// Background thread that stores images of the screen
void *writeScreenShotThread(void *args) {
  ((Screen*)args)->writeScreenShot();
  return NULL;
}

// Constructor: open window and init opengl
Screen::Screen(Device *device) {

  // Set variables
  this->device=device;
  this->wakeLock=core->getConfigStore()->getIntValue("General","wakeLock", __FILE__, __LINE__);
  this->separateFramebuffer=separateFramebuffer;
  this->lineWidth=0;
  framebuffer=0;
  colorRenderbuffer=0;
  setWakeLock(wakeLock, __FILE__, __LINE__);
  for (Int i=0;i<2;i++) {
    screenShotPixels[i]=NULL;
  }
  quitWriteScreenShotThread=false;
  nextScreenShotPixelsIndex=0;
  if (separateFramebuffer) {
    nextScreenShotPixelsMutex=core->getThread()->createMutex("current screen shot pixels mutex");
    writeScreenShotSignal=core->getThread()->createSignal();
    writeScreenShotThreadInfo=core->getThread()->createThread("screen shot write thread",writeScreenShotThread,this);
  }
}

// Main loop
void Screen::mainLoop() {
  glutMainLoop();
}

// Get the width of the screen
Int Screen::getWidth() {
  return width;
}

// Get the height of the screen
Int Screen::getHeight() {
  return height;
}

// Gets the orientation of the screen
GraphicScreenOrientation Screen::getOrientation() {
  return orientation;
}

// Checks if an extension is available
bool Screen::queryExtension(const char *extName) {
  /*
  ** Search for extName in the extensions string. Use of strstr()
  ** is not sufficient because extension names can be prefixes of
  ** other extension names. Could use strtok() but the constant
  ** string returned by glGetString might be in read-only memory.
  */
  char *p;
  char *end;
  int extNameLen;

  extNameLen = strlen(extName);

  p = (char *)glGetString(GL_EXTENSIONS);
  //DEBUG("%s",p);
  if (NULL == p) {
    return false;
  }

  end = p + strlen(p);

  while (p < end) {
    int n = strcspn(p, " ");
    if ((extNameLen == n) && (strncmp(extName, p, n) == 0)) {
      return true;
    }
    p += (n + 1);
  }
  return false;
}

// Inits the screen
void Screen::init(GraphicScreenOrientation orientation, Int width, Int height) {

  int argc = 0;
  char **argv = NULL;

  // Update variables
  this->width=width;
  this->height=height;
  this->orientation=orientation;
  this->textureFormatRGB888Supported=false;
  this->textureFormatRGBA8888Supported=false;
  DEBUG("dpi=%d width=%d height=%d",getDPI(),width,height);

  // Shall we do off-screen rendering?
  if (!separateFramebuffer) {

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

  } else {

    // Create a suitable frame buffer
    glGenFramebuffers(1,&framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    glGenRenderbuffers(1,&colorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER,colorRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, getWidth(), getHeight());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
      FATAL("can not create off-screen frame buffer",NULL);

    // Reserve memory for the screen shot buffers
    core->getThread()->lockMutex(nextScreenShotPixelsMutex,__FILE__,__LINE__);
    for (Int i=0;i<2;i++) {
      if (screenShotPixels[i])
        free(screenShotPixels[i]);
      if (!(screenShotPixels[i]=malloc(width*height*Image::getRGBPixelSize()))) {
        FATAL("can not reserve memory for screen shot pixels",NULL);
        return;
      }
    }
    nextScreenShotPixelsIndex=0;
    core->getThread()->unlockMutex(nextScreenShotPixelsMutex);

  }

  // Compute the maximum tiles to show
  if (!separateFramebuffer)
    core->getMapEngine()->setMaxTiles();

  // Check for error
  GLenum error=glGetError();
  if (error!=GL_NO_ERROR) {
    FATAL("something went wrong during init (error=0x%08x)",error);
  }
}

// Activates the screen for drawing
void Screen::startScene() {

  // Set frame buffer
  if (separateFramebuffer)
    glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
  else
    glBindFramebuffer(GL_FRAMEBUFFER,0);

  // Init display
  if (device->getWhiteBackground())
    glClearColor(1.0f,1.0f,1.0f,0.0f);
  else
    glClearColor(0.0f,0.0f,0.0f,0.0f);
  glViewport(0, 0, (GLsizei) getWidth(), (GLsizei) getHeight());
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-((GLfloat)getWidth()) / 2.0, ((GLfloat)getWidth()) / 2.0, -((GLfloat)getHeight()) / 2.0, ((GLfloat)getHeight()) / 2.0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );      // select modulate to mix texture with color for shading

  // Enable transparency
  glEnable(GL_BLEND);
  //glEnable (GL_LINE_SMOOTH);
  setColorModeAlpha();

  // Check for error
  GLenum error=glGetError();
  if (error!=GL_NO_ERROR) {
    FATAL("something went wrong during startScene (error=0x%08x)",error);
  }
}

// Creates a screen shot
bool Screen::createScreenShot() {

  // Get the screen pixels
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  core->getThread()->lockMutex(nextScreenShotPixelsMutex,__FILE__,__LINE__);
  GLvoid *nextScreenShotPixels=screenShotPixels[nextScreenShotPixelsIndex];
  if (nextScreenShotPixels!=NULL)
    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,nextScreenShotPixels);
  core->getThread()->unlockMutex(nextScreenShotPixelsMutex);
  if (nextScreenShotPixels!=NULL)
    core->getThread()->issueSignal(writeScreenShotSignal);

  // Check for error
  GLenum error=glGetError();
  if (error!=GL_NO_ERROR) {
    FATAL("something went wrong during createScreenShot (error=0x%08x)",error);
  }

  return true;
}

// Writes the screen shot as a png
void Screen::writeScreenShot() {

  GLvoid *pixels;

  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundLow);

  // This thread can be cancelled at any time
  core->getThread()->setThreadCancable();

  // Do an endless loop
  while (true) {

    // Wait for the trigger
    core->getThread()->waitForSignal(writeScreenShotSignal);

    // Shall we quit?
    if (quitWriteScreenShotThread)
      return;

    // Copy the screen shot pixels
    core->getThread()->lockMutex(nextScreenShotPixelsMutex,__FILE__,__LINE__);
    pixels=screenShotPixels[nextScreenShotPixelsIndex];
    nextScreenShotPixelsIndex=(nextScreenShotPixelsIndex+1)%2;
    core->getThread()->unlockMutex(nextScreenShotPixelsMutex);

    // Write the PNG
    //core->getImage()->writePNG((ImagePixel*)pixels,screenShotPath,width,height,core->getImage()->getRGBPixelSize(),true);
  }
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
  this->lineWidth=width;
  glLineWidth(width);
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
    glBindTexture( GL_TEXTURE_2D, 0 );
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
      Int negHalveLineWidth=lineWidth/2;
      Int posHalveLineWidth=lineWidth/2+lineWidth%2;
      drawRectangle(x1-negHalveLineWidth,y1-negHalveLineWidth,x1+posHalveLineWidth,y2+posHalveLineWidth,texture,true);
      drawRectangle(x1-negHalveLineWidth,y2-negHalveLineWidth,x2+posHalveLineWidth,y2+posHalveLineWidth,texture,true);
      drawRectangle(x2-negHalveLineWidth,y2+posHalveLineWidth,x2+posHalveLineWidth,y1-negHalveLineWidth,texture,true);
      drawRectangle(x2+negHalveLineWidth,y1+posHalveLineWidth,x1-negHalveLineWidth,y1-negHalveLineWidth,texture,true);
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
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDisableClientState(GL_VERTEX_ARRAY);
  if (textureInfo!=textureNotDefined) {
    glBindTexture( GL_TEXTURE_2D, 0 );
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable( GL_TEXTURE_2D );
  }
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

  // Check for error
  GLenum error=glGetError();
  if (error!=GL_NO_ERROR) {
    FATAL("something went wrong during drawing (error=0x%08x)",error);
  }
}

// Creates a new texture id
GraphicTextureInfo Screen::createTextureInfo() {
  if (allowAllocation) {
    GraphicTextureInfo i;
    if (unusedTextureInfos.size()>0) {
      i=unusedTextureInfos.front().textureInfo;
      unusedTextureInfos.pop_front();
      //DEBUG("reusing existing texture info (i==%d)",i);
    } else {
      glGenTextures( 1, &i );
      //DEBUG("creating new texture info (i==%d)",i);
      if (i==Screen::textureNotDefined) // if the texture id matches the special value, generate a new one
        glGenTextures( 1, &i );
      GLenum error=glGetError();
      if (error!=GL_NO_ERROR) {
        FATAL("can not generate texture (error=0x%08x)",error);
      }
    }
    return i;
  } else {
    FATAL("texture allocation has been disallowed",NULL);
    return textureNotDefined;
  }
}

// Sets the image of a texture
bool Screen::setTextureImage(GraphicTextureInfo texture, UByte *image, Int width, Int height, GraphicTextureFormat format) {
  glBindTexture(GL_TEXTURE_2D,texture);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  GLenum imageFormat;
  GLenum imageContents;
  GLenum imageDataType;
  switch(format) {
    case GraphicTextureFormatRGB565:
      imageFormat=GL_RGB;
      imageContents=GL_RGB;
      imageDataType=GL_UNSIGNED_SHORT_5_6_5;
      break;
    case GraphicTextureFormatRGBA4444:
      imageFormat=GL_RGBA;
      imageContents=GL_RGBA;
      imageDataType=GL_UNSIGNED_SHORT_4_4_4_4;
      break;
    case GraphicTextureFormatRGBA5551:
      imageFormat=GL_RGBA;
      imageContents=GL_RGBA;
      imageDataType=GL_UNSIGNED_SHORT_5_5_5_1;
      break;
    case GraphicTextureFormatRGB888:
      imageFormat=GL_RGB;
      imageContents=GL_RGB;
      imageDataType=GL_UNSIGNED_BYTE;
      break;
    case GraphicTextureFormatRGBA8888:
      imageFormat=GL_RGBA;
      imageContents=GL_RGBA;
      imageDataType=GL_UNSIGNED_BYTE;
      break;
    default:
      FATAL("unknown format",NULL);
      return false;
  }
  glTexImage2D(GL_TEXTURE_2D, 0, imageFormat, width, height, 0, imageContents, imageDataType, image);
  GLenum error=glGetError();
  glBindTexture(GL_TEXTURE_2D,0);
  if (error!=GL_NO_ERROR) {
    DEBUG("glGetError()=0x%x",error);
    return false;
  } else {
    return true;
  }
}

// Frees a texture id
void Screen::destroyTextureInfo(GraphicTextureInfo i, std::string source) {
  for(std::list<TextureDebugInfo>::iterator j=unusedTextureInfos.begin();j!=unusedTextureInfos.end();j++) {
    if (i==(*j).textureInfo) {
      FATAL("texture 0x%08x already destroyed (first destroy by %s, second destroy by %s)",i,(*j).source.c_str(),source.c_str());
    }
  }
  TextureDebugInfo t;
  t.textureInfo=i;
  t.source=source;
  unusedTextureInfos.push_back(t);
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
  for(std::list<GraphicBufferInfo>::iterator i=unusedBufferInfos.begin();i!=unusedBufferInfos.end();i++) {
    if (buffer==*i) {
      FATAL("buffer 0x%08x already destroyed!",buffer);
    }
  }
  unusedBufferInfos.push_back(buffer);
}

// If set to one, the screen is not turned off
void Screen::setWakeLock(bool state, const char *file, int line, bool persistent) {
  if (persistent)
    core->getConfigStore()->setIntValue("General","wakeLock",state,file,line);
  wakeLock=state;
}

// Frees any internal textures or buffers
void Screen::graphicInvalidated(bool contextLost) {
  if (allowDestroying) {
    DEBUG("graphic invalidation called",NULL);
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
}

// Destructor
Screen::~Screen() {
  if (separateFramebuffer) {
    quitWriteScreenShotThread=true;
    core->getThread()->issueSignal(writeScreenShotSignal);
    core->getThread()->waitForThread(writeScreenShotThreadInfo);
    core->getThread()->destroyThread(writeScreenShotThreadInfo);
    core->getThread()->destroyMutex(nextScreenShotPixelsMutex);
    core->getThread()->destroySignal(writeScreenShotSignal);
  }
  for (Int i=0;i<2;i++)
    if (screenShotPixels[i])
      free(screenShotPixels[i]);
}

// Setups the EGL context
bool Screen::setupContext() {
  FATAL("not supported",NULL);
  return false;
}

// Destroys the EGL context
void Screen::shutdownContext() {
  FATAL("not supported",NULL);
}

// Returns the diagonal of the screen
double Screen::getDiagonal() const {
  return device->getDiagonal();
}

// Returns the DPI of the screen
Int Screen::getDPI() const
{
    return device->getDPI();
}

}
