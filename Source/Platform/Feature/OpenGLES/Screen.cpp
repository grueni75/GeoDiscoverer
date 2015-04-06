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

namespace GEODISCOVERER {

// Program for the vertex shader
const char *Screen::vertexShaderProgram =
    // Inputs
"   uniform mat4 mvpMatrix;                           \n"
"   uniform vec4 colorIn;                             \n"
"   attribute vec4 positionIn;                        \n"
"   attribute vec2 textureCoordinateIn;               \n"
    // Variables shared with the fragment shader
"   varying vec4 colorOut;                            \n"
"   varying vec2 textureCoordinateOut;                \n"
"                                                     \n"
"   void main()                                       \n"
"   {                                                 \n"
"     colorOut = colorIn;                             \n"
"     gl_Position = mvpMatrix * positionIn;           \n"
"     textureCoordinateOut = textureCoordinateIn;     \n"
"   }                                                 \n";

// Program for the fragment shader
const char *Screen::fragmentShaderProgram =
    // Inputs
"   uniform bool textureEnabled;                                                  \n"
"   uniform sampler2D textureImageIn;                                             \n"
    // Variables shared with the vertex shader
"   varying lowp vec4 colorOut;                                                   \n"
"   varying lowp vec2 textureCoordinateOut;                                       \n"
"                                                                                 \n"
"   void main()                                                                   \n"
"   {                                                                             \n"
"     if (textureEnabled)                                                         \n"
"       gl_FragColor = colorOut * texture2D(textureImageIn, textureCoordinateOut);\n"
"     else                                                                        \n"
"       gl_FragColor = colorOut;                                                  \n"
"   }                                                                             \n";

// Constructor: open window and init opengl
Screen::Screen(Device *device) {
  this->device=device;
  this->allowDestroying=false;
  this->allowAllocation=false;
  this->orientation=GraphicScreenOrientationProtrait;
  this->wakeLock=core->getConfigStore()->getIntValue("General","wakeLock",__FILE__, __LINE__);
  this->ellipseCoordinatesBuffer=bufferNotDefined;
  this->separateFramebuffer=(device!=core->getDefaultDevice());
  this->testTextureInfo=getTextureNotDefined();
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
  textureCoordinateInHandle=-1;
  textureImageInHandle=-1;
  textureEnabledHandle=-1;
  lineWidth=0;
  openglES30Supported=false;
  pixelBuffer=0;
  pixelBufferSize=0;
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
  projectionMatrix = glm::frustum<GLfloat>(-((GLfloat)getWidth()) / 2.0, ((GLfloat)getWidth()) / 2.0, -((GLfloat)getHeight()) / 2.0, ((GLfloat)getHeight()) / 2.0, -1.0, 1.0);
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
  modelMatrix = glm::tmat4x4<GLfloat>(1.0);
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
  glm::tvec3<GLfloat> axis = glm::tvec3<GLfloat>(x,y,z);
  modelMatrix=glm::rotate(modelMatrix,(GLfloat)FloatingPoint::degree2rad(angle),axis);
  mvpMatrix=vpMatrix*modelMatrix;
  glUniformMatrix4fv(mvpMatrixHandle,1,false,glm::value_ptr(mvpMatrix));
}

// Scales the scene
void Screen::scale(double x, double y, double z) {
  glm::tvec3<GLfloat> factors = glm::tvec3<GLfloat>(x,y,z);
  modelMatrix=glm::scale(modelMatrix,factors);
  mvpMatrix=vpMatrix*modelMatrix;
  glUniformMatrix4fv(mvpMatrixHandle,1,false,glm::value_ptr(mvpMatrix));
}

// Translates the scene
void Screen::translate(Int x, Int y, Int z) {
  glm::tvec3<GLfloat> translation = glm::tvec3<GLfloat>(x,y,z);
  modelMatrix=glm::translate(modelMatrix,translation);
  mvpMatrix=vpMatrix*modelMatrix;
  glUniformMatrix4fv(mvpMatrixHandle,1,false,glm::value_ptr(mvpMatrix));
}

// Sets the drawing color
void Screen::setColor(UByte r, UByte g, UByte b, UByte a) {
  drawingColor = glm::tvec4<GLfloat>((GLfloat) (r) / 255.0, (GLfloat) (g) / 255.0, (GLfloat) (b) / 255.0, (GLfloat) (a) / 255.0);
  glUniform4fv(colorInHandle,1,glm::value_ptr(drawingColor));
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
  GLfloat box[] = {x1,y1, x2,y1, x1,y2, x1,y2, x2,y1, x2,y2};
  //DEBUG("x1=%d y1=%d x2=%d y2=%d",x1,y1,x2,y2);
  if (texture!=Screen::getTextureNotDefined()) {
    GLfloat tex[] = {0.0,1.0, 1.0,1.0, 0.0,0.0, 0.0,0.0, 1.0,1.0, 1.0,0.0};
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
void Screen::drawTriangles(Int numberOfTriangles, GraphicBufferInfo pointCoordinatesBuffer, GraphicTextureInfo textureInfo, GraphicBufferInfo textureCoordinatesBuffer) {

  if (textureInfo!=textureNotDefined) {
    glBindBuffer(GL_ARRAY_BUFFER, textureCoordinatesBuffer);
    glVertexAttribPointer(textureCoordinateInHandle, 2, GL_SHORT, false, 0, 0);
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

// Finished the drawing of the scene
void Screen::endScene() {

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
      if (!contextLost)
        glDeleteTextures(1,&textureInfo);
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

  // Shall we do off-screen rendering?
  if (separateFramebuffer) {

    // Create a suitable frame buffer
    glGenFramebuffers(1,&framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    glGenRenderbuffers(1,&colorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER,colorRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8_OES, getWidth(), getHeight());
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
  textureCoordinateInHandle = glGetAttribLocation(shaderProgramHandle, "textureCoordinateIn");
  textureImageInHandle = glGetUniformLocation(shaderProgramHandle, "textureImageIn");
  textureEnabledHandle = glGetUniformLocation(shaderProgramHandle, "textureEnabled");
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
  glm::tvec3<GLfloat> eye = glm::tvec3<GLfloat>(0.0f,0.0f,1.0f);
  glm::tvec3<GLfloat> center = glm::tvec3<GLfloat>(0.0f,0.0f,-1.0f);
  glm::tvec3<GLfloat> up = glm::tvec3<GLfloat>(0.0f,-1.0f,0.0f);
  viewMatrix = glm::lookAt(eye,center,up);

  // Init the model matrix
  modelMatrix = glm::tmat4x4<GLfloat>(1.0);

  // Update dependent matrixes
  vpMatrix=projectionMatrix*viewMatrix;
  mvpMatrix=vpMatrix*modelMatrix;

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
  if (testTextureInfo!=getTextureNotDefined())
    destroyTextureInfo(testTextureInfo,"Screen::destroyGraphic");
}

// Destructor
Screen::~Screen() {
  if (screenShotPixel)
    free(screenShotPixel);
}

// Static variables for EGL context
EGLConfig Screen::eglConfig;
EGLSurface Screen::eglSurface;
EGLContext Screen::eglContext;
EGLDisplay Screen::eglDisplay;

// Setups the EGL context
bool Screen::setupContext() {

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
}

// Destroys the EGL context
void Screen::shutdownContext() {
  eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroyContext(eglDisplay, eglContext);
  eglDestroySurface(eglDisplay, eglSurface);
  eglTerminate(eglDisplay);

  eglDisplay = EGL_NO_DISPLAY;
  eglSurface = EGL_NO_SURFACE;
  eglContext = EGL_NO_CONTEXT;
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
