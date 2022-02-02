//============================================================================
// Name        : Screen.cpp
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

#include <Core.h>
#include <Screen.h>
#include <Commander.h>
#include <MapEngine.h>
#include <Device.h>
#include <Image.h>
#include <FloatingPoint.h>

#ifdef TARGET_LINUX

int frameCountBeforeTextureInvalidation=30;
bool graphicInitialized=false;

// Proxy functions
void displayFunc()
{
  if (!graphicInitialized) {
    GEODISCOVERER::core->updateGraphic(false,false);
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
  case 'f':
    GEODISCOVERER::core->getCommander()->execute("toggleFingerMenu()");
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
    GEODISCOVERER::core->updateGraphic(false,false);
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

#endif

namespace GEODISCOVERER {

// Program for the vertex shader
const char *Screen::vertexShaderProgram =
#ifdef TARGET_LINUX
"   #version 130                                                                    \n"
#endif
    // Functions
"   vec3 rgb2hsv(vec3 c)                                                            \n"
"   {                                                                               \n"
"     vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);                              \n"
"     vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));             \n"
"     vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));             \n"
"     float d = q.x - min(q.w, q.y);                                                \n"
"     float e = 1.0e-10;                                                            \n"
"     return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);      \n"
"   }                                                                               \n"
"   vec3 hsv2rgb(vec3 c)                                                            \n"
"   {                                                                               \n"
"     vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);                                \n"
"     vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);                             \n"
"     return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);                     \n"
"   }                                                                               \n"
    // Constants
"   #define M_PI 3.1415926535897932384626433832795                                  \n"
"   #define WHITE_BLEND_START 0.2                                                   \n"
"   #define WHITE_BLEND_END 0.8                                                     \n"
"   #define WHITE_BLEND_DURATION 0.1                                                \n"
    // Inputs
"   uniform bool timeColoringEnabled;                                               \n"
"   uniform mat4 mvpMatrix;                                                         \n"
"   uniform vec4 colorIn;                                                           \n"
"   uniform float timeOffset;                                                       \n"
"   attribute vec4 positionIn;                                                      \n"
"   attribute vec2 textureCoordinateIn;                                             \n"
"   attribute float colorOffsetIn;                                                  \n"
    // Variables shared with the fragment shader
"   varying vec4 colorOut;                                                          \n"
"   varying vec2 textureCoordinateOut;                                              \n"
"                                                                                   \n"
"   void main()                                                                     \n"
"   {                                                                               \n"
"     if (timeColoringEnabled) {                                                    \n"
"       float t=timeOffset-colorOffsetIn;                                           \n"
"       if (t <= 0.0)                                                               \n"
"         t = t + 1.0;                                                              \n"
"       if (t >= 1.0)                                                               \n"
"         t = t - 1.0;                                                              \n"
"       float b;                                                                    \n"
"       if (t < WHITE_BLEND_START)                                                  \n"
"         b = 0.0;                                                                  \n"
"       else if (t < WHITE_BLEND_START + WHITE_BLEND_DURATION)                      \n"
"         b = sin((t-WHITE_BLEND_START)/WHITE_BLEND_DURATION*M_PI/2.0);             \n"
"       else if (t < WHITE_BLEND_END)                                               \n"
"         b = 1.0;                                                                  \n"
"       else if (t < WHITE_BLEND_END + WHITE_BLEND_DURATION)                        \n"
"         b = cos((t-WHITE_BLEND_END)/WHITE_BLEND_DURATION*M_PI/2.0);               \n"
"       else                                                                        \n"
"         b = 0.0;                                                                  \n"
"       colorOut = vec4(1.0*b+colorIn.x*(1.0-b),1.0*b+colorIn.y*(1.0-b),1.0*b       \n"
"                  +colorIn.z*(1.0-b),colorIn.w);                                   \n"
"     } else {                                                                      \n"
"       colorOut = colorIn;                                                         \n"
"     }                                                                             \n"
"     gl_Position = mvpMatrix * positionIn;                                         \n"
"     textureCoordinateOut = textureCoordinateIn;                                   \n"
"   }                                                                               \n";

// Program for the fragment shader
const char *Screen::fragmentShaderProgram =
#ifdef TARGET_LINUX
"   #version 130                                                                    \n"
#endif
    // Inputs
"   uniform bool textureEnabled;                                                    \n"
"   uniform sampler2D textureImageIn;                                               \n"
    // Variables shared with the vertex shader
"   varying lowp vec4 colorOut;                                                     \n"
"   varying lowp vec2 textureCoordinateOut;                                         \n"
"                                                                                   \n"
"   void main()                                                                     \n"
"   {                                                                               \n"
"     if (textureEnabled)                                                           \n"
"       gl_FragColor = colorOut * texture2D(textureImageIn, textureCoordinateOut);  \n"
"     else                                                                          \n"
"       gl_FragColor = colorOut;                                                    \n"
"   }                                                                               \n";

// Constructor: open window and init opengl
Screen::Screen(Device *device) {
  this->device=device;
  this->allowDestroying=false;
  this->allowAllocation=false;
  this->orientation=GraphicScreenOrientationProtrait;
  this->wakeLock=core->getConfigStore()->getIntValue("General","wakeLock",__FILE__, __LINE__);
  this->ellipseCoordinatesBuffer=bufferNotDefined;
  this->halfEllipseCoordinatesBuffer=bufferNotDefined;
  this->separateFramebuffer=(device!=core->getDefaultDevice());
  this->testTextureInfo=getTextureNotDefined();
  this->alphaScale=1.0;
  framebuffer=0;
  colorRenderbuffer=0;
  screenShotPixel=NULL;
  vertexShaderHandle=0;
  fragmentShaderHandle=0;
  shaderProgramHandle=0;
  width=0;
  height=0;
  mvpMatrixHandle=-1;
  positionInHandle=-1;
  colorInHandle=-1;
  timeOffsetHandle=-1;
  textureCoordinateInHandle=-1;
  textureImageInHandle=-1;
  textureEnabledHandle=-1;
  timeColoringEnabledHandle=-1;
  colorOffsetInHandle=-1;
  lineWidth=0;
  openglES30Supported=false;
  pixelBuffer=0;
  pixelBufferSize=0;
}

// Main loop
void Screen::mainLoop() {
#ifdef TARGET_LINUX
  int argc = 0;
  char **argv = NULL;
  glutInit(&argc, argv);
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(getWidth(), getHeight());
  glutInitWindowPosition(600, 50);
  Int winid=glutCreateWindow("GeoDiscoverer");
  glutDisplayFunc(displayFunc);
  glutKeyboardFunc(keyboardFunc);
  glutIdleFunc(idleFunc);
  glutMouseFunc(mouseFunc);
  glutMotionFunc(motionFunc);
  glutMainLoop();
  core->updateGraphic(false,true);
  core->getDefaultScreen()->setAllowDestroying(true);
  graphicInvalidated(false);
#endif
}

// Compiles a shader program
void Screen::compileShaderProgram(GLuint handle, const char *program) {
  GLint len = strlen(program);
  glShaderSource(handle,1,&program,&len);
  glCompileShader(handle);
  GLint compileSuccess;
  glGetShaderiv(handle, GL_COMPILE_STATUS, &compileSuccess);
  if (compileSuccess == GL_FALSE) {
      GLchar messages[256];
      glGetShaderInfoLog(handle, sizeof(messages), 0, &messages[0]);
      FATAL("can not compile shader program: %s",messages);
      return;
  }
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

  // Update variables
  this->width=width;
  this->height=height;
  this->orientation=orientation;
  DEBUG("dpi=%d width=%d height=%d",getDPI(),width,height);

  // Compute the maximum tiles to show
  if (!separateFramebuffer)
    core->getMapEngine()->setMaxTiles();

  // Init the projection matrix
  projectionMatrix = glm::frustum<float>(-((float)getWidth()) / 2.0, ((float)getWidth()) / 2.0, -((float)getHeight()) / 2.0, ((float)getHeight()) / 2.0, -1.0, 1.0);
  vpMatrix=projectionMatrix*viewMatrix;
  mvpMatrix=vpMatrix*modelMatrix;

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

  // Set texture parameter
  //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );      // select modulate to mix texture with color for shading
  //glActiveTexture(GL_TEXTURE0);
  //glUniform1i(textureImageInHandle, 0);

  // Enable transparency
  glEnable (GL_BLEND);
  setColorModeAlpha();

  // Activate texture slot
  glActiveTexture(GL_TEXTURE0);
  glUniform1i(textureImageInHandle, 0);

  // Hand over pointers to static variables to the shader
  glEnableVertexAttribArray(positionInHandle);
  glEnableVertexAttribArray(textureCoordinateInHandle);
  glUniform1i(textureEnabledHandle,1);

  // Clear the matrix stack
  if (modelMatrixStack.size()!=0) {
    FATAL("model matrix stack has entries left from previous drawing",NULL);
  }
  modelMatrix = glm::mat4x4(1.0);
  mvpMatrix=vpMatrix*modelMatrix;
  glUniformMatrix4fv(mvpMatrixHandle,1,false,glm::value_ptr(mvpMatrix));

  // Check for error
  GLenum error=glGetError();
  if (error!=GL_NO_ERROR) {
    FATAL("something went wrong during startScene (error=0x%08x)",error);
  }
}

// Creates a screen shot
bool Screen::createScreenShot() {

  bool result = true;

  // Get the screen pixels
  GLvoid *pixels;
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  if (pixelBufferSize!=0) {

    // Grab the image
    if (openglES30Supported) {
      glBindBuffer(GL_PIXEL_PACK_BUFFER, pixelBuffer);
      glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,0);
      pixels = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, pixelBufferSize, GL_MAP_READ_BIT);
    } else {
      glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,screenShotPixel);
      pixels=screenShotPixel;
    }

    // Write the PNG
    result = core->getImage()->writePNG((ImagePixel*)pixels,device,width,height,core->getImage()->getRGBPixelSize(),true);

    // Clean up
    if (openglES30Supported) {
      glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
      glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }
  }

  // Check for error
  GLenum error=glGetError();
  if (error!=GL_NO_ERROR) {
    FATAL("something went wrong during createScreenShot (error=0x%08x)",error);
  }

  return result;
}

// Clears the scene
void Screen::clear() {
  glClear(GL_COLOR_BUFFER_BIT);
}

// Starts a new object
void Screen::startObject() {
  modelMatrixStack.push_back(modelMatrix);
}

// Sets the line width for drawing operations
void Screen::setLineWidth(Int width) {
  lineWidth=width;
  glLineWidth(width);
}

// Ends the current object
void Screen::endObject() {
  modelMatrix=modelMatrixStack.back();
  modelMatrixStack.pop_back();
  mvpMatrix=vpMatrix*modelMatrix;
  glUniformMatrix4fv(mvpMatrixHandle,1,false,glm::value_ptr(mvpMatrix));
}

// Rotates the scene
void Screen::rotate(double angle, Int x, Int y, Int z) {
  glm::vec3 axis = glm::vec3(x,y,z);
  modelMatrix=glm::rotate(modelMatrix,(float)FloatingPoint::degree2rad(angle),axis);
  mvpMatrix=vpMatrix*modelMatrix;
  glUniformMatrix4fv(mvpMatrixHandle,1,false,glm::value_ptr(mvpMatrix));
}

// Scales the scene
void Screen::scale(double x, double y, double z) {
  glm::vec3 factors = glm::vec3(x,y,z);
  modelMatrix=glm::scale(modelMatrix,factors);
  mvpMatrix=vpMatrix*modelMatrix;
  glUniformMatrix4fv(mvpMatrixHandle,1,false,glm::value_ptr(mvpMatrix));
}

// Translates the scene
void Screen::translate(Int x, Int y, Int z) {
  glm::vec3 translation = glm::vec3(x,y,z);
  modelMatrix=glm::translate(modelMatrix,translation);
  mvpMatrix=vpMatrix*modelMatrix;
  glUniformMatrix4fv(mvpMatrixHandle,1,false,glm::value_ptr(mvpMatrix));
}

// Sets the drawing color
void Screen::setColor(UByte r, UByte g, UByte b, UByte a) {
  drawingColor = glm::vec4((float) (r) / 255.0, (float) (g) / 255.0, (float) (b) / 255.0, (float) (a) * alphaScale / 255.0);
  glUniform4fv(colorInHandle,1,glm::value_ptr(drawingColor));
}

// Sets the time offset
void Screen::setTimeOffset(double timeOffset) {
  GLfloat value = timeOffset;
  //DEBUG("timeOffset=%f",value);
  glUniform1f(timeOffsetHandle,value);
}

// Sets color mode such that alpha channel of primitive determines its transparency
void Screen::setColorModeAlpha() {
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Sets color mode such that primitive color is multiplied with background color
void Screen::setColorModeMultiply() {
  glBlendFunc(GL_ZERO, GL_SRC_COLOR);
}

// Draws a rectangle
void Screen::drawRectangle(Int x1, Int y1, Int x2, Int y2, GraphicTextureInfo texture, bool filled) {
  float box[] = {(float)x1, (float)y1, (float)x2,(float)y1, (float)x1,(float)y2, (float)x1,(float)y2, (float)x2,(float)y1, (float)x2,(float)y2};
  //DEBUG("x1=%d y1=%d x2=%d y2=%d",x1,y1,x2,y2);
  if (texture!=Screen::getTextureNotDefined()) {
    float tex[] = {0.0,1.0, 1.0,1.0, 0.0,0.0, 0.0,0.0, 1.0,1.0, 1.0,0.0};
    glBindTexture( GL_TEXTURE_2D, texture );
    glVertexAttribPointer(positionInHandle, 2, GL_FLOAT, false, 0, &box[0]);
    glVertexAttribPointer(textureCoordinateInHandle, 2, GL_FLOAT, false, 0, &tex[0]);
    glDrawArrays(GL_TRIANGLES,0,6);
  } else {
    if (filled) {
      glDisableVertexAttribArray(textureCoordinateInHandle);
      glUniform1i(textureEnabledHandle,0);
      glVertexAttribPointer(positionInHandle, 2, GL_FLOAT, false, 0, &box[0]);
      glDrawArrays(GL_TRIANGLES,0,6);
      glUniform1i(textureEnabledHandle,1);
      glEnableVertexAttribArray(textureCoordinateInHandle);
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
void Screen::drawTriangles(Int numberOfTriangles, GraphicBufferInfo pointCoordinatesBuffer, GraphicTextureInfo textureInfo, GraphicBufferInfo textureCoordinatesBuffer, boolean normalizeTextureCoordinates)  {

  if (textureInfo!=textureNotDefined) {
    glBindBuffer(GL_ARRAY_BUFFER, textureCoordinatesBuffer);
    glVertexAttribPointer(textureCoordinateInHandle, 2, GL_SHORT, normalizeTextureCoordinates, 0, 0);
    glBindTexture( GL_TEXTURE_2D, textureInfo );
  } else {
    glDisableVertexAttribArray(textureCoordinateInHandle);
    glUniform1i(textureEnabledHandle,0);
  }
  glBindBuffer(GL_ARRAY_BUFFER, pointCoordinatesBuffer);
  glVertexAttribPointer(positionInHandle, 2, GL_SHORT, false, 0, 0);
  glDrawArrays(GL_TRIANGLES, 0, numberOfTriangles*3);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  if (textureInfo==textureNotDefined) {
    glEnableVertexAttribArray(textureCoordinateInHandle);
    glUniform1i(textureEnabledHandle,1);
  }
}

// Draws a ellipse
void Screen::drawEllipse(bool filled) {

  // Points prepared?
  if (ellipseCoordinatesBuffer==bufferNotDefined) {

    // No, create them
    ellipseCoordinatesBuffer=createBufferInfo();
    float *ellipseSegmentPoints=NULL;
    if (!(ellipseSegmentPoints=(float*)malloc(2*sizeof(*ellipseSegmentPoints)*ellipseSegments))) {
      FATAL("can not create ellipse segment points array",NULL);
      return;
    }
    Int index=0;
    for(double t = 0; t <= 2*M_PI; t += 2*M_PI/(ellipseSegments-1)) {
      ellipseSegmentPoints[index++]=cos(t);
      ellipseSegmentPoints[index++]=sin(t);
    }
    setArrayBufferData(ellipseCoordinatesBuffer,(Byte*)ellipseSegmentPoints,2*sizeof(*ellipseSegmentPoints)*ellipseSegments);
    free(ellipseSegmentPoints);
  }

  // Draw the ellipse
  glDisableVertexAttribArray(textureCoordinateInHandle);
  glUniform1i(textureEnabledHandle,0);
  glBindBuffer(GL_ARRAY_BUFFER, ellipseCoordinatesBuffer);
  glVertexAttribPointer(positionInHandle, 2, GL_FLOAT, false, 0, 0);
  if (filled) {
    glDrawArrays(GL_TRIANGLE_FAN,0,ellipseSegments);
  } else {
    glDrawArrays(GL_LINE_LOOP,0,ellipseSegments);
  }
  glUniform1i(textureEnabledHandle,1);
  glEnableVertexAttribArray(textureCoordinateInHandle);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Draws a half ellipse
void Screen::drawHalfEllipse(bool filled) {

  // Points prepared?
  if (halfEllipseCoordinatesBuffer==bufferNotDefined) {

    // No, create them
    halfEllipseCoordinatesBuffer=createBufferInfo();
    float *ellipseSegmentPoints=NULL;
    if (!(ellipseSegmentPoints=(float*)malloc(2*sizeof(*ellipseSegmentPoints)*ellipseSegments/2))) {
      FATAL("can not create ellipse segment points array",NULL);
      return;
    }
    Int index=0;
    for(double t = 0; t <= M_PI; t += M_PI/(ellipseSegments/2-1)) {
      ellipseSegmentPoints[index++]=cos(t);
      ellipseSegmentPoints[index++]=sin(t);
      //DEBUG("t=%f index=%d size=%d",FloatingPoint::rad2degree(t),index*sizeof(*ellipseSegmentPoints),sizeof(*ellipseSegmentPoints)*ellipseSegments);
    }
    setArrayBufferData(halfEllipseCoordinatesBuffer,(Byte*)ellipseSegmentPoints,sizeof(*ellipseSegmentPoints)*ellipseSegments);
    free(ellipseSegmentPoints);
  }

  // Draw the ellipse
  glDisableVertexAttribArray(textureCoordinateInHandle);
  glUniform1i(textureEnabledHandle,0);
  glBindBuffer(GL_ARRAY_BUFFER, halfEllipseCoordinatesBuffer);
  glVertexAttribPointer(positionInHandle, 2, GL_FLOAT, false, 0, 0);
  if (filled) {
    glDrawArrays(GL_TRIANGLE_FAN,0,ellipseSegments/2);
  } else {
    glDrawArrays(GL_LINE_LOOP,0,ellipseSegments/2);
  }
  glUniform1i(textureEnabledHandle,1);
  glEnableVertexAttribArray(textureCoordinateInHandle);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Finished the drawing of the scene
void Screen::endScene() {

#ifdef TARGET_LINUX
  glFlush();
  glutSwapBuffers();
#endif

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
      //DEBUG("reusing existing texture info",NULL);
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
  //DEBUG("imageFormat=%d width=%d height=%d imageContents=%d imageDataType=%d ",imageFormat,width,height,imageContents,imageDataType);
  glTexImage2D(GL_TEXTURE_2D, 0, imageFormat, width, height, 0, imageContents, imageDataType, image);
  GLenum error=glGetError();
  if (error!=GL_NO_ERROR) {
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

// Frees any internal textures or buffers
void Screen::graphicInvalidated(bool contextLost) {
  if (allowDestroying) {
    DEBUG("graphic invalidation called",NULL);
    //DEBUG("unusedTextureInfos.size()=%d",unusedTextureInfos.size());
    for(std::list<TextureDebugInfo>::iterator i=unusedTextureInfos.begin();i!=unusedTextureInfos.end();i++) {
      GraphicTextureInfo textureInfo=(*i).textureInfo;
      if (!contextLost) {
        glDeleteTextures(1,&textureInfo);
      }
    }
    unusedTextureInfos.clear();
    //DEBUG("unusedBufferInfos.size()=%d",unusedBufferInfos.size());
    for(std::list<GraphicBufferInfo>::iterator i=unusedBufferInfos.begin();i!=unusedBufferInfos.end();i++) {
      GraphicBufferInfo bufferInfo=*i;
      if (!contextLost)
        glDeleteBuffers(1,&bufferInfo);
    }
    unusedBufferInfos.clear();
    if (shaderProgramHandle) {
      if (!contextLost)
        glDeleteProgram(shaderProgramHandle);
      shaderProgramHandle=0;
    }
    if (fragmentShaderHandle) {
      if (!contextLost)
        glDeleteShader(fragmentShaderHandle);
      fragmentShaderHandle=0;
    }
    if (vertexShaderHandle) {
      if (!contextLost)
        glDeleteShader(vertexShaderHandle);
      vertexShaderHandle=0;
    }
    if (separateFramebuffer) {
      if (colorRenderbuffer) {
        if (!contextLost)
          glDeleteRenderbuffers(1,&colorRenderbuffer);
        colorRenderbuffer=0;
      }
      if (framebuffer) {
        if (!contextLost)
          glDeleteFramebuffers(1,&framebuffer);
        framebuffer=0;
      }
    }
  } else {
    FATAL("texture and buffer destroying has been disallowed",NULL);
  }

  // Check for error
  GLenum error=glGetError();
  if (error!=GL_NO_ERROR) {
    FATAL("something went wrong during graphicInvalidated (error=0x%08x)",error);
  }
}

// Creates the graphic
void Screen::createGraphic() {

#ifdef TARGET_ANDROID
  // Check if OpenGLES 3.0 is available
  const char* versionStr = (const char*)glGetString(GL_VERSION);
  if (strstr(versionStr, "OpenGL ES 3.")) {
    if (!openglES30Supported) {
      if (gl3stubInit()) {
        openglES30Supported=true;
        DEBUG("OpenGL ES 3.0 supported",NULL);
      }
    }
  }
#endif

  // Shall we do off-screen rendering?
  if (separateFramebuffer) {

    // Create a suitable frame buffer
    glGenFramebuffers(1,&framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    glGenRenderbuffers(1,&colorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER,colorRenderbuffer);
#ifdef TARGET_LINUX
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, getWidth(), getHeight());
#endif
#ifdef TARGET_ANDROID
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8_OES, getWidth(), getHeight());
#endif
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      glDeleteFramebuffers(1,&framebuffer);
      framebuffer=0;
      ERROR("can not create off-screen frame buffer (status=0x%04x)",status);
    }
    glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    pixelBufferSize=getWidth()*getHeight()*Image::getRGBPixelSize();

    // Create a suitable pixel buffer object for fast reads
    if (openglES30Supported) {
      glGenBuffers(1, &pixelBuffer);
      glBindBuffer(GL_PIXEL_PACK_BUFFER, pixelBuffer);
      glBufferData(GL_PIXEL_PACK_BUFFER, pixelBufferSize, 0, GL_DYNAMIC_READ);
      glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }

    // Reserve memory for the screen shot buffers
    if (!openglES30Supported) {
      if (screenShotPixel)
        free(screenShotPixel);
      if (!(screenShotPixel=malloc(pixelBufferSize))) {
        FATAL("can not reserve memory for screen shot pixels",NULL);
        return;
      }
    }
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER,0);
  }

  // Check if the more accurate texture formats are supported
  UByte texture[] = { 0,0,0,0 };
  testTextureInfo=createTextureInfo();
  if (setTextureImage(testTextureInfo,&texture[0],1,1,GraphicTextureFormatRGBA8888)) {
    DEBUG("RGBA8888 supported!",NULL);
    textureFormatRGBA8888Supported=true;
  }
  if (setTextureImage(testTextureInfo,&texture[0],1,1,GraphicTextureFormatRGB888)) {
    DEBUG("RGB888 supported!",NULL);
    textureFormatRGB888Supported=true;
  }

  // Compile and link the vertex shader program
  vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
  if (!vertexShaderHandle) {
    FATAL("can not create vertex shader handle",NULL);
    return;
  }
  compileShaderProgram(vertexShaderHandle,vertexShaderProgram);
  fragmentShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
  if (!vertexShaderHandle) {
    FATAL("can not create vertex shader handle",NULL);
    return;
  }
  compileShaderProgram(fragmentShaderHandle,fragmentShaderProgram);
  shaderProgramHandle=glCreateProgram();
  glAttachShader(shaderProgramHandle, vertexShaderHandle);
  glAttachShader(shaderProgramHandle, fragmentShaderHandle);
  glLinkProgram(shaderProgramHandle);
  GLint linkSuccess;
  glGetProgramiv(shaderProgramHandle, GL_LINK_STATUS, &linkSuccess);
  if (linkSuccess == GL_FALSE) {
    GLchar messages[256];
    glGetProgramInfoLog(shaderProgramHandle, sizeof(messages), 0, &messages[0]);
    FATAL("can not link shader program: %s",messages);
    return;
  }

  // Get pointer to the input variables of the shader
  mvpMatrixHandle = glGetUniformLocation(shaderProgramHandle, "mvpMatrix");
  positionInHandle = glGetAttribLocation(shaderProgramHandle, "positionIn");
  colorInHandle = glGetUniformLocation(shaderProgramHandle, "colorIn");
  timeOffsetHandle = glGetUniformLocation(shaderProgramHandle, "timeOffset");
  textureCoordinateInHandle = glGetAttribLocation(shaderProgramHandle, "textureCoordinateIn");
  textureImageInHandle = glGetUniformLocation(shaderProgramHandle, "textureImageIn");
  textureEnabledHandle = glGetUniformLocation(shaderProgramHandle, "textureEnabled");
  timeColoringEnabledHandle = glGetUniformLocation(shaderProgramHandle, "timeColoringEnabled");
  colorOffsetInHandle = glGetAttribLocation(shaderProgramHandle, "colorOffsetIn");
  /*DEBUG("vertexShaderHandle=%d",vertexShaderHandle);
  DEBUG("fragmentShaderHandle=%d",fragmentShaderHandle);
  DEBUG("shaderProgramHandle=%d",shaderProgramHandle);
  DEBUG("mvpMatrixHandle=%d",mvpMatrixHandle);
  DEBUG("positionInHandle=%d",positionInHandle);
  DEBUG("colorInHandle=%d",colorInHandle);
  DEBUG("textureCoordinateInHandle=%d",textureCoordinateInHandle);
  DEBUG("textureImageInHandle=%d",textureImageInHandle);
  DEBUG("textureEnabledHandle=%d",textureEnabledHandle);*/

  // Use this program
  glUseProgram(shaderProgramHandle);

  // Init the view matrix
  glm::vec3 eye = glm::vec3(0.0f,0.0f,1.0f);
  glm::vec3 center = glm::vec3(0.0f,0.0f,-1.0f);
  glm::vec3 up = glm::vec3(0.0f,-1.0f,0.0f);
  viewMatrix = glm::lookAt(eye,center,up);

  // Init the model matrix
  modelMatrix = glm::mat4x4(1.0);

  // Update dependent matrixes
  vpMatrix=projectionMatrix*viewMatrix;
  mvpMatrix=vpMatrix*modelMatrix;

  // Init the time offset
  setTimeOffset(0);
  setTimeColoringMode(false);

  // Check for error
  GLenum error=glGetError();
  if (error!=GL_NO_ERROR) {
    FATAL("something went wrong during createGraphic (error=0x%08x)",error);
  }
}

// Destroys the graphic
void Screen::destroyGraphic() {
  if (ellipseCoordinatesBuffer!=bufferNotDefined) {
    destroyBufferInfo(ellipseCoordinatesBuffer);
    ellipseCoordinatesBuffer=bufferNotDefined;
  }
  if (halfEllipseCoordinatesBuffer!=bufferNotDefined) {
    destroyBufferInfo(halfEllipseCoordinatesBuffer);
    halfEllipseCoordinatesBuffer=bufferNotDefined;
  }
  if (testTextureInfo!=getTextureNotDefined())
    destroyTextureInfo(testTextureInfo,"Screen::destroyGraphic");
}

// Destructor
Screen::~Screen() {
  if (screenShotPixel)
    free(screenShotPixel);
}

#ifdef TARGET_ANDROID
// Static variables for EGL context
EGLConfig Screen::eglConfig;
EGLSurface Screen::eglSurface;
EGLContext Screen::eglContext;
EGLDisplay Screen::eglDisplay;
#endif

// Setups the EGL context
bool Screen::setupContext() {

#ifdef TARGET_ANDROID

  // EGL config attributes
  const EGLint confAttr[] = {
          EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
          EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
          EGL_RED_SIZE,   8,
          EGL_GREEN_SIZE, 8,
          EGL_BLUE_SIZE,  8,
          EGL_ALPHA_SIZE, 0,
          EGL_DEPTH_SIZE, 0,
          EGL_NONE
  };

  // EGL context attributes
  const EGLint ctxAttr[] = {
          EGL_CONTEXT_CLIENT_VERSION, 2,
          EGL_NONE
  };

  // surface attributes
  // the surface size is set to the input frame size
  const EGLint surfaceAttr[] = {
           EGL_WIDTH, 1,
           EGL_HEIGHT, 1,
           EGL_NONE
  };

  EGLint eglMajVers, eglMinVers;
  EGLint numConfigs;

  // Create display
  eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (eglDisplay==EGL_NO_DISPLAY) {
    ERROR("can not get default EGL display",NULL);
    return false;
  }
  if (!eglInitialize(eglDisplay, &eglMajVers, &eglMinVers)) {
    ERROR("can not get initialize EGL display",NULL);
    return false;
  }

  // Choose the first config, i.e. best config
  if (!eglChooseConfig(eglDisplay, confAttr, &eglConfig, 1, &numConfigs)) {
    ERROR("can not choose EGL config",NULL);
    return false;
  }

  // Create context
  eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, ctxAttr);
  if (eglContext==EGL_NO_CONTEXT) {
    ERROR("can not create EGL context",NULL);
    return false;
  }

  // create a pixelbuffer surface
  eglSurface = eglCreatePbufferSurface(eglDisplay, eglConfig, surfaceAttr);
  if (eglContext==EGL_NO_SURFACE) {
    ERROR("can not create EGL surface",NULL);
    return false;
  }

  // Activate the context
  if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
    ERROR("can not make EGL context current",NULL);
    return false;
  }

  return true;
#endif

#ifdef TARGET_LINUX
  FATAL("not supported",NULL);
  return false;
#endif
}

// Destroys the EGL context
void Screen::shutdownContext() {
#ifdef TARGET_ANDROID
  eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  if (!(eglDestroySurface(eglDisplay, eglSurface))) {
    FATAL("can not destroy EGL surface",NULL);
  }
  if (!(eglDestroyContext(eglDisplay, eglContext))) {
    FATAL("can not destroy EGL context",NULL);
  }
  if (!(eglTerminate(eglDisplay))) {
    FATAL("can not terminate EGL display",NULL);
  }
  eglDisplay = EGL_NO_DISPLAY;
  eglSurface = EGL_NO_SURFACE;
  eglContext = EGL_NO_CONTEXT;
#endif

#ifdef TARGET_LINUX
  FATAL("not supported",NULL);
#endif
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

// Enables or disables the time coloring mode
void Screen::setTimeColoringMode(bool enable, GraphicBufferInfo buffer) {
  glUniform1i(timeColoringEnabledHandle,enable ? 1 : 0);
  if (enable) {
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glVertexAttribPointer(colorOffsetInHandle, 1, GL_FLOAT, false, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(colorOffsetInHandle);
  } else {
    glDisableVertexAttribArray(colorOffsetInHandle);
  }
}

// Draws a rounded rectangle
void Screen::drawRoundedRectangle(Int width, Int height) {
  Int x1=-width/2;
  Int y1=-height/2;
  Int x2=x1+width;
  Int y2=y1+height;
  drawRectangle(x1,y1,x2,y2,Screen::getTextureNotDefined(),true);
  startObject();
  translate(x1,y1+height/2,0);
  rotate(+90,0,0,1);
  scale(height/2,height/2,0);      
  drawHalfEllipse(true);
  endObject();
  startObject();
  translate(x2,y1+height/2,0);
  rotate(-90,0,0,1);
  scale(height/2,height/2,0);
  drawHalfEllipse(true);
  endObject();
}

}
