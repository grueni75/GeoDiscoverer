//============================================================================
// Name        : WidgetBase.cpp
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
#include <WidgetPrimitive.h>
#include <Screen.h>
#include <WidgetContainer.h>
#include <MapPosition.h>

namespace GEODISCOVERER {

// Constructor
WidgetPrimitive::WidgetPrimitive(WidgetContainer *widgetContainer) : GraphicRectangle(widgetContainer->getScreen()) {
  type=GraphicTypeWidget;
  this->widgetContainer=widgetContainer;
  widgetType=WidgetTypePrimitive;
  isHit=false;
  isSelected=false;
  isFirstTimeSelected=false;
  isHidden=false;
  xHidden=0;
  yHidden=0;
  xOriginal=0;
  yOriginal=0;
  touchMode=0;
  gaugeBackgroundColor=core->getConfigStore()->getGraphicColorValue("Graphic/Widget/GaugeBackgroundColor",__FILE__, __LINE__);
  gaugeForegroundColor=core->getConfigStore()->getGraphicColorValue("Graphic/Widget/GaugeForegroundColor",__FILE__, __LINE__);
  gaugeFillgroundColor=core->getConfigStore()->getGraphicColorValue("Graphic/Widget/GaugeFillgroundColor",__FILE__, __LINE__);
  gaugeEmptyColor=core->getConfigStore()->getGraphicColorValue("Graphic/Widget/GaugeEmptyColor",__FILE__, __LINE__);
  batteryEmptyLevel=core->getConfigStore()->getIntValue("Graphic/Widget","batteryEmptyLevel",__FILE__, __LINE__);
}

// Destructor
WidgetPrimitive::~WidgetPrimitive() {
}

// Updates various flags
void WidgetPrimitive::updateFlags(Int x, Int y) {
  int w=getIconWidth()*scale;
  int h=getIconHeight()*scale;
  int tx1=getX()+getIconWidth()/2-w/2;
  int tx2=tx1+w;
  int ty1=getY()+getIconHeight()/2-h/2;
  int ty2=ty1+h;
  if ((!isHidden)&&(x>=tx1)&&(x<tx2)&&(y>=ty1)&&(y<ty2)) {
    isHit=true;
  } else {
    isHit=false;
  }
}

// Executed if the widget is currently touched
void WidgetPrimitive::onTouchDown(TimestampInMicroseconds t, Int x, Int y) {
  updateFlags(x,y);
  if (getIsHit()) {
    if (!isSelected) {
      isFirstTimeSelected=true;
    } else {
      isFirstTimeSelected=false;
    }
    isSelected=true;
  } else {
    isSelected=false;
    isFirstTimeSelected=false;
  }
}

// Executed if the widget has been untouched
void WidgetPrimitive::onTouchUp(TimestampInMicroseconds t, Int x, Int y, bool cancel) {
  updateFlags(x,y);
  isSelected=false;
  isFirstTimeSelected=false;
  if (cancel)
    isHit=false;
}

// Called when the map has changed
void WidgetPrimitive::onMapChange(bool widgetVisible, MapPosition pos) {

}

// Called when the location has changed
void WidgetPrimitive::onLocationChange(bool widgetVisible, MapPosition pos) {

}

// Called when a path has changed
void WidgetPrimitive::onPathChange(bool widgetVisible, NavigationPath *path, NavigationPathChangeType changeType) {

}

// Called when some data has changed
void WidgetPrimitive::onDataChange() {

}

// Called when a two fingure gesture is done on the widget
void WidgetPrimitive::onTwoFingerGesture(TimestampInMicroseconds t, Int dX, Int dY, double angleDiff, double scaleDiff) {
  return;
}

// Called when the widget has changed his position
void WidgetPrimitive::updatePosition(Int x, Int y, Int z) {
  setX(x);
  setY(y);
  setZ(z);
}

// Called when the rectangle must be drawn
void WidgetPrimitive::draw(TimestampInMicroseconds t) {

  // Do not draw if primitive is hidden
  if (getIsHidden()) {
    return;
  }

  // Don't draw if scale factor is 0
  if (scale==0)
    return;

  // Set color
  screen->setColor(getColor().getRed(),getColor().getGreen(),getColor().getBlue(),getColor().getAlpha());

  // Draw widget
  screen->startObject();
  screen->translate(getX()+getIconWidth()/2,getY()+getIconHeight()/2,0);
  screen->scale(scale,scale,1.0);
  Int x1=-getIconWidth()/2;
  Int y1=-getIconHeight()/2;
  Int x2=x1+getWidth();
  Int y2=y1+getHeight();
  screen->drawRectangle(x1,y1,x2,y2,getTexture(),getFilled());
  screen->endObject();
}

// Draws a battery symbol
void WidgetPrimitive::drawBattery(
  TimestampInMicroseconds t,
  Int offsetX,
  Int batteryLevel,
  FontString *label,
  FontString *level,
  Int batteryGaugeBackgroundWidth,
  Int batteryGaugeForegroundWidth,
  Int batteryGaugeTipWidth,
  Int batteryGaugeTipHeight,
  Int batteryGaugeOffsetY,
  Int batteryGaugeMaxHeight,
  bool batteryCharging,
  Int batteryChargingIconX,
  Int batteryChargingIconY,
  GraphicRectangle *batteryChargingIcon,
  double batteryChargingIconScale
) {
  Int batteryGaugeCurrentHeight=batteryLevel*batteryGaugeMaxHeight/100.0;
  GraphicColor c = gaugeBackgroundColor;
  c.setAlpha(color.getAlpha());
  screen->setColor(c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha());
  screen->drawRectangle(
    x + getIconWidth() / 2 + offsetX - batteryGaugeBackgroundWidth / 2,
    y + batteryGaugeOffsetY,
    x + getIconWidth() / 2 + offsetX + batteryGaugeBackgroundWidth / 2,
    y + batteryGaugeOffsetY + batteryGaugeMaxHeight + (batteryGaugeBackgroundWidth - batteryGaugeForegroundWidth),
    Screen::getTextureNotDefined(), true);

  screen->drawRectangle(
    x + getIconWidth() / 2 + offsetX - batteryGaugeTipWidth / 2,
    y + batteryGaugeOffsetY + batteryGaugeMaxHeight + (batteryGaugeBackgroundWidth - batteryGaugeForegroundWidth),
    x + getIconWidth() / 2 + offsetX + batteryGaugeTipWidth / 2,
    y + batteryGaugeOffsetY + batteryGaugeMaxHeight + (batteryGaugeBackgroundWidth - batteryGaugeForegroundWidth) + batteryGaugeTipHeight,
    Screen::getTextureNotDefined(), true);

  c = gaugeFillgroundColor;
  c.setAlpha(color.getAlpha());
  screen->setColor(c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha());
  screen->drawRectangle(
    x + getIconWidth() / 2 + offsetX - batteryGaugeForegroundWidth / 2,
    y + batteryGaugeOffsetY + (batteryGaugeBackgroundWidth - batteryGaugeForegroundWidth) / 2,
    x + getIconWidth() / 2 + offsetX + batteryGaugeForegroundWidth / 2,
    y + batteryGaugeOffsetY + (batteryGaugeBackgroundWidth - batteryGaugeForegroundWidth) / 2 + batteryGaugeMaxHeight,
    Screen::getTextureNotDefined(), true);
  
  if (batteryLevel<batteryEmptyLevel)
    c = gaugeEmptyColor;
  else
    c = gaugeForegroundColor;
  c.setAlpha(color.getAlpha());
  screen->setColor(c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha());
  screen->drawRectangle(
    x + getIconWidth() / 2 + offsetX - batteryGaugeForegroundWidth / 2,
    y + batteryGaugeOffsetY + (batteryGaugeBackgroundWidth - batteryGaugeForegroundWidth) / 2,
    x + getIconWidth() / 2 + offsetX + batteryGaugeForegroundWidth / 2,
    y + batteryGaugeOffsetY + (batteryGaugeBackgroundWidth - batteryGaugeForegroundWidth) / 2 + batteryGaugeCurrentHeight,
    Screen::getTextureNotDefined(), true);

  if (label) {
    label->setColor(color);
    label->draw(t);
  }
  if (level) {
    if (batteryCharging) {
      screen->startObject();
      screen->translate(batteryChargingIconX, batteryChargingIconY, batteryChargingIcon->getZ());
      screen->scale(batteryChargingIconScale, batteryChargingIconScale, 1.0);
      batteryChargingIcon->setColor(color);
      batteryChargingIcon->draw(t);
      screen->endObject();
    }
    level->setColor(color);
    level->draw(t);
  }
}

}
