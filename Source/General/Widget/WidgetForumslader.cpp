//============================================================================
// Name        : WidgetForumslader.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2021 Matthias Gruenewald
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
#include <WidgetForumslader.h>
#include <WidgetContainer.h>
#include <FontEngine.h>
#include <GraphicEngine.h>
#include <FloatingPoint.h>

namespace GEODISCOVERER {

// Constructor
WidgetForumslader::WidgetForumslader(WidgetContainer *widgetContainer) : 
  WidgetPrimitive(widgetContainer),
  powerDrawGaugeBackground(widgetContainer->getScreen()),
  powerDrawGaugeFillground(widgetContainer->getScreen()),
  powerDrawGaugeForeground(widgetContainer->getScreen())
{
  phBatteryLevelFontString=NULL;
  flBatteryLevelFontString=NULL;
  powerDrawLevelFontString=NULL;
  flBatteryLabelFontString=NULL;
  phBatteryLabelFontString=NULL;
  firstRun=true;  
  updateRequired=true;  
  connected=false;
  core->getConfigStore()->setIntValue("Forumslader","connected",0,__FILE__,__LINE__);
}

// Destructor
WidgetForumslader::~WidgetForumslader() {
  widgetContainer->getFontEngine()->lockFont("sansNormal",__FILE__, __LINE__);
  if (phBatteryLevelFontString) widgetContainer->getFontEngine()->destroyString(phBatteryLevelFontString);
  if (flBatteryLevelFontString) widgetContainer->getFontEngine()->destroyString(flBatteryLevelFontString);
  widgetContainer->getFontEngine()->unlockFont();
  widgetContainer->getFontEngine()->lockFont("sansNormal",__FILE__, __LINE__);
  if (powerDrawLevelFontString) widgetContainer->getFontEngine()->destroyString(powerDrawLevelFontString);
  widgetContainer->getFontEngine()->unlockFont();
  widgetContainer->getFontEngine()->lockFont("sansTiny",__FILE__, __LINE__);
  if (flBatteryLabelFontString) widgetContainer->getFontEngine()->destroyString(flBatteryLabelFontString);
  if (phBatteryLabelFontString) widgetContainer->getFontEngine()->destroyString(phBatteryLabelFontString);
  widgetContainer->getFontEngine()->unlockFont();
}

// Executed every time the graphic engine checks if drawing is required
bool WidgetForumslader::work(TimestampInMicroseconds t) {

  FontEngine *fontEngine=widgetContainer->getFontEngine();
  ConfigStore *configStore=core->getConfigStore();
  std::string value;
  std::string unit;
  bool update;

  // Do the inherited stuff
  bool changed=WidgetPrimitive::work(t);

  // Only update the info at given update interval
  if (updateRequired) {
    
    // Is ebike connected?
    boolean prevConnected=connected;
    connected=configStore->getIntValue("Forumslader","connected",__FILE__,__LINE__);
    //connected=true;
    //core->getConfigStore()->setIntValue("Forumslader","powerDrawLevel",-10,__FILE__,__LINE__);
    //DEBUG("prevConnected=%d connected=%d",prevConnected,connected);
    
    // Update the widget visibility
    if (firstRun) {
      color.setAlpha(0);
      firstRun=false;
    }
    if (connected!=prevConnected) {
      GraphicColor targetColor=getActiveColor();
      if (!connected) {
        targetColor.setAlpha(0);
      }
      setFadeAnimation(t,getColor(),targetColor,false,widgetContainer->getGraphicEngine()->getFadeDuration());
    }
  
    // Update the content
    if (connected) {
    
      /*powerDrawLevelOffsetY=0.12*getIconHeight();
      powerDrawGaugeOffsetY=0.35*getIconHeight();
      powerDrawGaugeRadius=0.20*widgetContainer->getScreen()->getDPI();
      powerDrawGaugeBackgroundWidth=0.14*widgetContainer->getScreen()->getDPI();
      powerDrawGaugeForegroundWidth=0.08*widgetContainer->getScreen()->getDPI();
      batteryLevelOffsetY=0.12*getIconHeight();
      batteryGaugeOffsetX=0.35*getIconWidth();
      batteryGaugeOffsetY=0.35*getIconHeight();
      batteryGaugeMaxHeight=0.33*getIconHeight();
      batteryGaugeBackgroundWidth=0.15*widgetContainer->getScreen()->getDPI();
      batteryGaugeForegroundWidth=0.09*widgetContainer->getScreen()->getDPI();
      batteryGaugeTipWidth=0.08*widgetContainer->getScreen()->getDPI();
      batteryGaugeTipHeight=0.05*widgetContainer->getScreen()->getDPI();*/

      // Update the font string objects
      std::stringstream s;
      s.setf(std::ios_base::fixed, std::ios_base::floatfield);
      s.precision(0);
      double flBatteryLevel;
      std::string value=configStore->getStringValue("Forumslader","batteryLevel",__FILE__,__LINE__);
      if (value=="?") {
        s<<"NV";
        flBatteryLevel=0;
      } else {
        flBatteryLevel=atof(value.c_str());
        s<<flBatteryLevel<< "%";
      }
      fontEngine->lockFont("sansNormal",__FILE__, __LINE__);
      fontEngine->updateString(&flBatteryLevelFontString,s.str());
      fontEngine->unlockFont();
      flBatteryLevelFontString->setX(x+getIconWidth()/2-batteryGaugeOffsetX-flBatteryLevelFontString->getIconWidth()/2);
      flBatteryLevelFontString->setY(y+batteryLevelOffsetY);
      Int phBatteryLevel=core->getBatteryLevel();      
      s.str("");
      s<<phBatteryLevel<< "%";
      fontEngine->lockFont("sansNormal",__FILE__, __LINE__);
      fontEngine->updateString(&phBatteryLevelFontString,s.str());
      fontEngine->unlockFont();
      phBatteryLevelFontString->setX(x+getIconWidth()/2+batteryGaugeOffsetX-phBatteryLevelFontString->getIconWidth()/2);
      phBatteryLevelFontString->setY(y+batteryLevelOffsetY);
      double powerDrawLevel;
      value=configStore->getStringValue("Forumslader","powerDrawLevel",__FILE__,__LINE__);
      s.str("");
      s.precision(1);
      if (value=="?") {
        s<<"NV";
        powerDrawLevel=0;
      } else {
        powerDrawLevel=atof(value.c_str());
        s<<powerDrawLevel<< "W";
      }
      fontEngine->lockFont("sansNormal",__FILE__, __LINE__);
      fontEngine->updateString(&powerDrawLevelFontString,s.str());
      fontEngine->unlockFont();
      powerDrawLevelFontString->setX(x+getIconWidth()/2-powerDrawLevelFontString->getIconWidth()/2);
      powerDrawLevelFontString->setY(y+powerDrawLevelOffsetY);
      if (flBatteryLabelFontString==NULL) {
        fontEngine->lockFont("sansTiny",__FILE__, __LINE__);
        fontEngine->updateString(&flBatteryLabelFontString,"F");
        fontEngine->updateString(&phBatteryLabelFontString,"P");
        fontEngine->unlockFont();
        flBatteryLabelFontString->setX(x+getIconWidth()/2-batteryGaugeOffsetX-flBatteryLabelFontString->getIconWidth()/2);
        flBatteryLabelFontString->setY(y+batteryGaugeOffsetY+batteryGaugeMaxHeight/2+(batteryGaugeBackgroundWidth-batteryGaugeForegroundWidth)/2-flBatteryLabelFontString->getIconHeight()/2);
        phBatteryLabelFontString->setX(x+getIconWidth()/2+batteryGaugeOffsetX-phBatteryLabelFontString->getIconWidth()/2);
        phBatteryLabelFontString->setY(y+batteryGaugeOffsetY+batteryGaugeMaxHeight/2+(batteryGaugeBackgroundWidth-batteryGaugeForegroundWidth)/2-phBatteryLabelFontString->getIconHeight()/2);
      }
      
      // Update the power draw gauge
      double powerDrawLevelMax=atof(configStore->getStringValue("Forumslader","powerDrawLevelMax",__FILE__,__LINE__).c_str());      
      if (powerDrawLevel>powerDrawLevelMax) powerDrawLevel=powerDrawLevelMax;
      if (powerDrawLevel<-powerDrawLevelMax) powerDrawLevel=-powerDrawLevelMax;
      double powerDrawPercentage=powerDrawLevel/powerDrawLevelMax;
      double powerDrawAngleSweep=180;
      double powerDrawCircumference=floor(FloatingPoint::degree2rad(powerDrawAngleSweep)*powerDrawGaugeRadius);
      double radius=powerDrawCircumference/FloatingPoint::degree2rad(powerDrawAngleSweep);
      powerDrawGaugeBackground.setColor(gaugeBackgroundColor);
      powerDrawGaugeBackground.setAngle(90);
      powerDrawGaugeBackground.setRadius(radius);
      powerDrawGaugeBackground.setX(x+getIconWidth()/2);
      powerDrawGaugeBackground.setY(y+powerDrawGaugeOffsetY);
      powerDrawGaugeBackground.setWidth(powerDrawCircumference);
      powerDrawGaugeBackground.setHeight(powerDrawGaugeBackgroundWidth);
      powerDrawGaugeBackground.setIconWidth(powerDrawGaugeBackground.getWidth());
      powerDrawGaugeBackground.setIconHeight(powerDrawGaugeBackground.getHeight());      
      changed |= powerDrawGaugeBackground.work(t);         
      powerDrawGaugeFillground.setColor(gaugeFillgroundColor);
      powerDrawGaugeFillground.setAngle(90.0);
      powerDrawGaugeFillground.setWidth(powerDrawCircumference);
      powerDrawGaugeFillground.setRadius(radius);
      powerDrawGaugeFillground.setX(x+getIconWidth()/2);
      powerDrawGaugeFillground.setY(y+powerDrawGaugeOffsetY);
      powerDrawGaugeFillground.setHeight(powerDrawGaugeForegroundWidth);
      powerDrawGaugeFillground.setIconWidth(powerDrawGaugeFillground.getWidth());
      powerDrawGaugeFillground.setIconHeight(powerDrawGaugeFillground.getHeight());      
      changed |= powerDrawGaugeFillground.work(t);
      powerDrawGaugeForeground.setColor(gaugeForegroundColor);
      powerDrawGaugeForeground.setAngle(90.0-powerDrawPercentage*powerDrawAngleSweep/4);
      powerDrawGaugeForeground.setWidth(fabs(powerDrawPercentage)*powerDrawCircumference/2);
      powerDrawGaugeForeground.setRadius(radius);
      powerDrawGaugeForeground.setX(x+getIconWidth()/2);
      powerDrawGaugeForeground.setY(y+powerDrawGaugeOffsetY);
      powerDrawGaugeForeground.setHeight(powerDrawGaugeForegroundWidth);
      powerDrawGaugeForeground.setIconWidth(powerDrawGaugeForeground.getWidth());
      powerDrawGaugeForeground.setIconHeight(powerDrawGaugeForeground.getHeight());      
      changed |= powerDrawGaugeForeground.work(t);

      // Update the battery levels
      batteryGaugeFlCurrentHeight=flBatteryLevel*batteryGaugeMaxHeight/100.0;
      batteryGaugePhCurrentHeight=core->getBatteryLevel()*batteryGaugeMaxHeight/100.0;
    }

    // Update the flags
    changed=true;
    updateRequired=false;

  }

  // Handle visibility
  if (getColor().getAlpha()==0) {
    if (!getIsHidden())
      setIsHidden(true);
  } else {
    if (getIsHidden())
      setIsHidden(false);
  }

  // Return result
  return changed;
}

// Draws a battery symbol
void WidgetForumslader::drawBattery(TimestampInMicroseconds t, Int offsetX, Int batteryGaugeCurrentHeight, FontString *label, FontString *level) {
  GraphicColor c=gaugeBackgroundColor;
  c.setAlpha(color.getAlpha());
  screen->setColor(c.getRed(),c.getGreen(),c.getBlue(),c.getAlpha());
  screen->drawRectangle(x+getIconWidth()/2+offsetX-batteryGaugeBackgroundWidth/2,
                        y+batteryGaugeOffsetY,
                        x+getIconWidth()/2+offsetX+batteryGaugeBackgroundWidth/2,
                        y+batteryGaugeOffsetY+batteryGaugeMaxHeight+(batteryGaugeBackgroundWidth-batteryGaugeForegroundWidth),
                        Screen::getTextureNotDefined(),true);
  screen->drawRectangle(x+getIconWidth()/2+offsetX-batteryGaugeTipWidth/2,
                        y+batteryGaugeOffsetY+batteryGaugeMaxHeight+(batteryGaugeBackgroundWidth-batteryGaugeForegroundWidth),
                        x+getIconWidth()/2+offsetX+batteryGaugeTipWidth/2,
                        y+batteryGaugeOffsetY+batteryGaugeMaxHeight+(batteryGaugeBackgroundWidth-batteryGaugeForegroundWidth)+batteryGaugeTipHeight,
                        Screen::getTextureNotDefined(),true);
  c=gaugeFillgroundColor;
  c.setAlpha(color.getAlpha());
  screen->setColor(c.getRed(),c.getGreen(),c.getBlue(),c.getAlpha());
  screen->drawRectangle(x+getIconWidth()/2+offsetX-batteryGaugeForegroundWidth/2,
                        y+batteryGaugeOffsetY+(batteryGaugeBackgroundWidth-batteryGaugeForegroundWidth)/2,
                        x+getIconWidth()/2+offsetX+batteryGaugeForegroundWidth/2,
                        y+batteryGaugeOffsetY+(batteryGaugeBackgroundWidth-batteryGaugeForegroundWidth)/2+batteryGaugeMaxHeight,
                        Screen::getTextureNotDefined(),true);
  c=gaugeForegroundColor;
  c.setAlpha(color.getAlpha());
  screen->setColor(c.getRed(),c.getGreen(),c.getBlue(),c.getAlpha());
  screen->drawRectangle(x+getIconWidth()/2+offsetX-batteryGaugeForegroundWidth/2,
                        y+batteryGaugeOffsetY+(batteryGaugeBackgroundWidth-batteryGaugeForegroundWidth)/2,
                        x+getIconWidth()/2+offsetX+batteryGaugeForegroundWidth/2,
                        y+batteryGaugeOffsetY+(batteryGaugeBackgroundWidth-batteryGaugeForegroundWidth)/2+batteryGaugeCurrentHeight,
                        Screen::getTextureNotDefined(),true);
  if (label) {
    label->setColor(color);
    label->draw(t);
  }
  if (level) {
    level->setColor(color);
    level->draw(t);
  }
}

// Executed every time the graphic engine needs to draw
void WidgetForumslader::draw(TimestampInMicroseconds t) {
  
  // Do not draw if primitive is hidden
  if (getIsHidden())
    return;
  
  // Let the primitive draw the background
  WidgetPrimitive::draw(t);
  
  // Draw the power draw gauge
  GraphicColor c=powerDrawGaugeBackground.getColor();
  c.setAlpha(color.getAlpha());
  powerDrawGaugeBackground.setColor(c);
  powerDrawGaugeBackground.draw(t);
  c=powerDrawGaugeFillground.getColor();
  c.setAlpha(color.getAlpha());
  powerDrawGaugeFillground.setColor(c);
  powerDrawGaugeFillground.draw(t);
  screen->setColor(gaugeBackgroundColor.getRed(),gaugeBackgroundColor.getGreen(),gaugeBackgroundColor.getBlue(),gaugeBackgroundColor.getAlpha());
  Int w=powerDrawGaugeBackgroundWidth-powerDrawGaugeForegroundWidth;
  screen->drawRectangle(x+getIconWidth()/2-w/4,
                        y+powerDrawGaugeOffsetY+powerDrawGaugeRadius+powerDrawGaugeBackgroundWidth/2,
                        x+getIconWidth()/2+w/4,
                        y+powerDrawGaugeOffsetY+powerDrawGaugeRadius-powerDrawGaugeBackgroundWidth/2,
                        Screen::getTextureNotDefined(),true);
  c=powerDrawGaugeForeground.getColor();
  c.setAlpha(color.getAlpha());
  powerDrawGaugeForeground.setColor(c);
  powerDrawGaugeForeground.draw(t);  
  if (powerDrawLevelFontString) {
    powerDrawLevelFontString->setColor(color);
    powerDrawLevelFontString->draw(t);
  }

  // Draw the battery symbols
  drawBattery(t,-batteryGaugeOffsetX,batteryGaugeFlCurrentHeight,flBatteryLabelFontString,flBatteryLevelFontString);
  drawBattery(t,+batteryGaugeOffsetX,batteryGaugePhCurrentHeight,phBatteryLabelFontString,phBatteryLevelFontString);
  
  /* Draw the engine temperature gauge
  screen->setColor(gaugeBackgroundColor.getRed(),gaugeBackgroundColor.getGreen(),gaugeBackgroundColor.getBlue(),gaugeBackgroundColor.getAlpha());
  screen->drawRectangle(x+engineTemperatureOffsetX-engineTemperatureBackgroundWidth/2,
                        y+engineTemperatureOffsetY+engineTemperatureForegroundRadius/2,
                        x+engineTemperatureOffsetX+engineTemperatureBackgroundWidth/2,
                        y+engineTemperatureOffsetY+engineTemperatureForegroundRadius/2+engineTemperatureMaxHeight,
                        Screen::getTextureNotDefined(),true);
  screen->startObject();
  screen->translate(x+engineTemperatureOffsetX,y+engineTemperatureOffsetY+engineTemperatureForegroundRadius/2+engineTemperatureMaxHeight,0);
  screen->scale(engineTemperatureBackgroundWidth/2,engineTemperatureBackgroundWidth/2,1.0);
  screen->drawEllipse(true);
  screen->endObject();
  screen->startObject();
  screen->translate(x+engineTemperatureOffsetX,y+engineTemperatureOffsetY,0);
  screen->setColor(gaugeBackgroundColor.getRed(),gaugeBackgroundColor.getGreen(),gaugeBackgroundColor.getBlue(),gaugeBackgroundColor.getAlpha());
  screen->startObject();
  screen->scale(engineTemperatureBackgroundRadius,engineTemperatureBackgroundRadius,1.0);
  screen->drawEllipse(true);
  screen->endObject();
  screen->setColor(gaugeForegroundColor.getRed(),gaugeForegroundColor.getGreen(),gaugeForegroundColor.getBlue(),gaugeForegroundColor.getAlpha());
  screen->startObject();
  screen->scale(engineTemperatureForegroundRadius,engineTemperatureForegroundRadius,1.0);
  screen->drawEllipse(true);
  screen->endObject();
  screen->endObject();
  screen->setColor(gaugeForegroundColor.getRed(),gaugeForegroundColor.getGreen(),gaugeForegroundColor.getBlue(),gaugeForegroundColor.getAlpha());
  screen->drawRectangle(x+engineTemperatureOffsetX-engineTemperatureForegroundWidth/2,
                        y+engineTemperatureOffsetY+engineTemperatureForegroundRadius/2,
                        x+engineTemperatureOffsetX+engineTemperatureForegroundWidth/2,
                        y+engineTemperatureOffsetY+engineTemperatureForegroundRadius/2+engineTemperatureCurrentHeight,
                        Screen::getTextureNotDefined(),true);
  screen->startObject();
  screen->translate(x+engineTemperatureOffsetX,y+engineTemperatureOffsetY+engineTemperatureForegroundRadius/2+engineTemperatureCurrentHeight,0);
  screen->scale(engineTemperatureForegroundWidth/2,engineTemperatureForegroundWidth/2,1.0);
  screen->drawEllipse(true);
  screen->endObject();
  if (engineTemperatureFontString) {
    engineTemperatureFontString->setColor(color);
    engineTemperatureFontString->draw(t);
  }*/
}

// Called when the widget has changed its position
void WidgetForumslader::updatePosition(Int x, Int y, Int z) {
  WidgetPrimitive::updatePosition(x,y,z);
  updateRequired=true;
}

// Called when some data has changed
void WidgetForumslader::onDataChange() {
  //DEBUG("data has changed",NULL);
  updateRequired=true;
}

}
