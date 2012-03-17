//============================================================================
// Name        : GraphicRectangle.cpp
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

// Constructors
GraphicRectangle::GraphicRectangle() : GraphicPrimitive()
{
  type=GraphicRectangleType;
  setFilled(true);
  setWidth(0);
  setHeight(0);
  iconWidth=0;
  iconHeight=0;
}

// Destructor
GraphicRectangle::~GraphicRectangle() {
}

// Loads an icon and sets it as the texture of the primitive
void GraphicRectangle::setTextureFromIcon(std::string iconFilename) {

  // Load the icon
  iconFilename+=".png";
  Int imageWidth, imageHeight;
  UInt pixelSize;
  double dpiScale;
  ImagePixel *iconImage=core->getImage()->loadPNGIcon(iconFilename,imageWidth,imageHeight,dpiScale,pixelSize);
  if (!iconImage)
    return;
  if (pixelSize!=Image::getRGBAPixelSize()) {
    ERROR("only PNG images with RGBA color space are supported as icons",NULL);
    return;
  }

  // Find the proper texture width
  Int textureWidth=1;
  while (textureWidth<imageWidth) textureWidth=2*textureWidth;
  Int textureHeight=1;
  while (textureHeight<imageHeight) textureHeight=2*textureHeight;
  if (textureHeight>textureWidth) {
    textureWidth=textureHeight;
  } else {
    textureHeight=textureWidth;
  }

  // Set the dimension of the icon that matches the screen dpi
  width=dpiScale*(double)textureWidth;
  height=dpiScale*(double)textureHeight;
  iconWidth=dpiScale*(double)imageWidth;
  iconHeight=dpiScale*(double)imageHeight;

  // Convert it to RGBA5551
  UShort *textureImage;
  textureImage=(UShort*)malloc(textureWidth*textureHeight*sizeof(UShort));
  if (!textureImage) {
    FATAL("can not reserve memory for texture image",NULL);
    return;
  }
  memset(textureImage,0,textureWidth*textureHeight*sizeof(UShort));
  for(Int y=0;y<imageHeight;y++) {
    for(Int x=0;x<imageWidth;x++) {
      UByte red=iconImage[(y*imageWidth+x)*Image::getRGBAPixelSize()+0]>>3;
      UByte green=iconImage[(y*imageWidth+x)*Image::getRGBAPixelSize()+1]>>3;
      UByte blue=iconImage[(y*imageWidth+x)*Image::getRGBAPixelSize()+2]>>3;
      UByte alpha=iconImage[(y*imageWidth+x)*Image::getRGBAPixelSize()+3]>>7;
      textureImage[(y+textureHeight-imageHeight)*textureWidth+x]=(red<<11)|(green<<6)|(blue<<1)|alpha;
    }
  }
  free(iconImage);

  // Create the texture
  //if (getTexture()!=core->getScreen()->getTextureNotDefined()) {
  //  core->getScreen()->destroyTextureInfo(getTexture());
  //}
  setTexture(core->getScreen()->createTextureInfo());
  core->getScreen()->setTextureImage(getTexture(),textureImage,textureWidth,textureHeight,graphicTextureFormatRGBA1);
  free(textureImage);
}

// Called when the rectangle must be drawn
void GraphicRectangle::draw(Screen *screen, TimestampInMicroseconds t) {

  // Set color
  screen->setColor(getColor().getRed(),getColor().getGreen(),getColor().getBlue(),getColor().getAlpha());

  // Draw widget
  Int x1=getX();
  Int y1=getY();
  Int x2=(GLfloat)(getWidth()+x1);
  Int y2=(GLfloat)(getHeight()+y1);
  screen->drawRectangle(x1,y1,x2,y2,getTexture(),getFilled());

}


}
