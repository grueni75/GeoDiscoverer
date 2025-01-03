//============================================================================
// Name        : WidgetEBike.cpp
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
#include <WidgetEBike.h>
#include <WidgetContainer.h>
#include <FontEngine.h>
#include <GraphicEngine.h>
#include <FloatingPoint.h>
#include <WidgetPage.h>

namespace GEODISCOVERER {

// Constructor
WidgetEBike::WidgetEBike(WidgetContainer *widgetContainer) : 
  WidgetPrimitive(widgetContainer),
  batteryLevelBackground(widgetContainer->getScreen()),
  batteryLevelForeground(widgetContainer->getScreen())
{
  powerLevelFontString=NULL;
  engineTemperatureFontString=NULL;
  distanceElectricFontString=NULL;
  core->getConfigStore()->setIntValue("EBikeMonitor","connected",0,__FILE__,__LINE__);
  /*core->getConfigStore()->setStringValue("EBikeMonitor","powerLevel","5",__FILE__,__LINE__);
  core->getConfigStore()->setStringValue("EBikeMonitor","batteryLevel","70",__FILE__,__LINE__);
  core->getConfigStore()->setStringValue("EBikeMonitor","engineTemperature","10",__FILE__,__LINE__);
  core->getConfigStore()->setStringValue("EBikeMonitor","distanceElectric","1200",__FILE__,__LINE__);
  core->getConfigStore()->setIntValue("EBikeMonitor","connected",1,__FILE__,__LINE__);*/
  connected=false;
  firstRun=true;  
  updateRequired=true;  
  setIsHidden(true);
}

// Destructor
WidgetEBike::~WidgetEBike() {
  widgetContainer->getFontEngine()->lockFont("sansTiny",__FILE__, __LINE__);
  if (distanceElectricFontString) widgetContainer->getFontEngine()->destroyString(distanceElectricFontString);
  if (engineTemperatureFontString) widgetContainer->getFontEngine()->destroyString(engineTemperatureFontString);
  widgetContainer->getFontEngine()->unlockFont();
  widgetContainer->getFontEngine()->lockFont("sansLarge",__FILE__, __LINE__);
  if (powerLevelFontString) widgetContainer->getFontEngine()->destroyString(powerLevelFontString);
  widgetContainer->getFontEngine()->unlockFont();
}

// Executed every time the graphic engine checks if drawing is required
bool WidgetEBike::work(TimestampInMicroseconds t) {

  FontEngine *fontEngine=widgetContainer->getFontEngine();
  ConfigStore *configStore=core->getConfigStore();
  std::string value;
  std::string unit;
  bool update;

  // Do the inherited stuff
  bool changed=WidgetPrimitive::work(t);

  /* Debug showing and hiding
  int coin=rand();
  if (coin>2140000000) {
    connected=configStore->getIntValue("EBikeMonitor","connected",__FILE__,__LINE__);
    configStore->setIntValue("EBikeMonitor","connected",!connected,__FILE__,__LINE__);
    updateRequired=true;
    DEBUG("coin=%d connected=%d",coin,connected);
  }*/

  // Only update the info at given update interval
  if (updateRequired) {
    
    // Is ebike connected?
    boolean prevConnected=connected;
    connected=configStore->getIntValue("EBikeMonitor","connected",__FILE__,__LINE__);
    
    // Update the widget visibility
    if (firstRun) {
      color.setAlpha(0);
      firstRun=false;
    }
    if (connected!=prevConnected) {
      WidgetPage *page=(WidgetPage*)widgetContainer;      
      GraphicColor targetColor=getActiveColor();
      if (!connected) {        
        //DEBUG("hiding widget",NULL);
        targetColor.setAlpha(0);
        setFadeAnimation(t,getColor(),targetColor,false,widgetContainer->getGraphicEngine()->getAnimDuration());
      } else {
        //DEBUG("showing widget",NULL);
        setIsHidden(false);
        setFadeAnimation(t,getColor(),targetColor,false,widgetContainer->getGraphicEngine()->getAnimDuration());
        page->setWidgetsActive(t,true);
      }
    }
  
    // Update the content
    if (connected) {
    
      /*engineTemperatureOffsetX=0.8*getIconWidth();
      engineTemperatureOffsetY=0.25*getIconHeight();
      engineTemperatureBackgroundRadius=0.13*getIconHeight();
      engineTemperatureForegroundRadius=0.8*engineTemperatureBackgroundRadius;
      engineTemperatureMaxHeight=0.50*getIconHeight();
      engineTemperatureBackgroundWidth=0.15*iconHeight;
      engineTemperatureForegroundWidth=0.6*engineTemperatureBackgroundWidth;
      distanceElectricOffsetX=0.35*getIconWidth();
      distanceElectricOffsetY=0.25*getIconHeight();
      batteryLevelRadius=0.25*iconHeight;
      batteryLevelBackgroundWidth=0.15*iconHeight;
      batteryLevelForegroundWidth=0.6*batteryLevelBackgroundWidth;
      gaugeBackgroundColor.setAlpha(255);
      gaugeBackgroundColor.setRed(255);
      gaugeBackgroundColor.setGreen(127);
      gaugeBackgroundColor.setBlue(0); 
      gaugeForegroundColor.setAlpha(255);
      gaugeForegroundColor.setRed(255);
      gaugeForegroundColor.setGreen(190);
      gaugeForegroundColor.setBlue(127); */

      // Update the font string objects
      Int engineTemperature=atoi(configStore->getStringValue("EBikeMonitor","engineTemperature",__FILE__,__LINE__).c_str());
      if (engineTemperature<=90)
        engineTemperatureCurrentHeight=engineTemperature*engineTemperatureMaxHeight/90;
      else 
        engineTemperatureCurrentHeight=engineTemperatureMaxHeight;
      fontEngine->lockFont("sansLarge",__FILE__, __LINE__);
      fontEngine->updateString(&powerLevelFontString,configStore->getStringValue("EBikeMonitor","powerLevel",__FILE__,__LINE__));
      fontEngine->unlockFont();
      powerLevelFontString->setX(x+powerLevelOffsetX-powerLevelFontString->getIconWidth()/2);
      powerLevelFontString->setY(y+powerLevelOffsetY-powerLevelFontString->getIconHeight()/2-powerLevelFontString->getBaselineOffsetY());
      fontEngine->lockFont("sansTiny",__FILE__, __LINE__);
      std::stringstream s;
      s<<engineTemperature;
      fontEngine->updateString(&engineTemperatureFontString,s.str());
      s.str("");
      s.clear();
      s<<configStore->getStringValue("EBikeMonitor","distanceElectric",__FILE__,__LINE__)<<" km";
      fontEngine->updateString(&distanceElectricFontString,s.str());
      fontEngine->unlockFont();
      engineTemperatureFontString->setX(x+engineTemperatureOffsetX-engineTemperatureFontString->getIconWidth()/2);
      engineTemperatureFontString->setY(y+engineTemperatureOffsetY-engineTemperatureFontString->getIconHeight()/2-engineTemperatureFontString->getBaselineOffsetY());
      distanceElectricFontString->setX(x+distanceElectricOffsetX-distanceElectricFontString->getIconWidth()/2);
      distanceElectricFontString->setY(y+distanceElectricOffsetY-distanceElectricFontString->getIconHeight()/2-distanceElectricFontString->getBaselineOffsetY());      
      
      // Update the battery level
      double batteryLevelValue=atof(configStore->getStringValue("EBikeMonitor","batteryLevel",__FILE__,__LINE__).c_str())/100.0;      
      double batteryLevelAngleSweep=220;
      double batteryLevelCircumference=FloatingPoint::degree2rad(batteryLevelAngleSweep)*batteryLevelRadius;
      batteryLevelBackground.setColor(gaugeBackgroundColor);
      batteryLevelBackground.setAngle(90);
      batteryLevelBackground.setRadius(batteryLevelRadius);
      batteryLevelBackground.setX(x+powerLevelOffsetX);
      batteryLevelBackground.setY(y+powerLevelOffsetY);
      batteryLevelBackground.setWidth(batteryLevelCircumference);
      batteryLevelBackground.setHeight(batteryLevelBackgroundWidth);
      batteryLevelBackground.setIconWidth(batteryLevelBackground.getWidth());
      batteryLevelBackground.setIconHeight(batteryLevelBackground.getHeight());      
      changed |= batteryLevelBackground.work(t);            
      batteryLevelForeground.setColor(gaugeForegroundColor);
      batteryLevelForeground.setAngle(90+(1.0-batteryLevelValue)*batteryLevelAngleSweep/2);
      batteryLevelForeground.setRadius(batteryLevelRadius);
      batteryLevelForeground.setX(x+powerLevelOffsetX);
      batteryLevelForeground.setY(y+powerLevelOffsetY);
      batteryLevelForeground.setWidth(batteryLevelValue*batteryLevelCircumference);
      batteryLevelForeground.setHeight(batteryLevelForegroundWidth);
      batteryLevelForeground.setIconWidth(batteryLevelForeground.getWidth());
      batteryLevelForeground.setIconHeight(batteryLevelForeground.getHeight());      
      changed |= batteryLevelForeground.work(t);
      
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
    if (getIsHidden()) {
      DEBUG("enabling widget (alpha=%d, connected=%d)",getColor().getAlpha(),connected);
      setIsHidden(false);
    }
  }

  // Return result
  return changed;
}

// Executed every time the graphic engine needs to draw
void WidgetEBike::draw(TimestampInMicroseconds t) {
  
  // Do not draw if primitive is hidden
  if (getIsHidden())
    return;
  
  // Let the primitive draw the background
  WidgetPrimitive::draw(t);
  
  // Draw the battery level gauge
  GraphicColor c=batteryLevelBackground.getColor();
  c.setAlpha(color.getAlpha());
  batteryLevelBackground.setColor(c);
  batteryLevelBackground.draw(t);
  c=batteryLevelForeground.getColor();
  c.setAlpha(color.getAlpha());
  batteryLevelForeground.setColor(c);
  batteryLevelForeground.draw(t);
  if (powerLevelFontString) {
    powerLevelFontString->setColor(color);
    powerLevelFontString->draw(t);
  }
  
  // Draw the distance travelled with engine
  if (distanceElectricFontString) {
    distanceElectricFontString->setColor(color);
    distanceElectricFontString->draw(t);
  }  
  
  // Draw the engine temperature gauge
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
  }
}

// Called when the widget has changed its position
void WidgetEBike::updatePosition(Int x, Int y, Int z) {
  WidgetPrimitive::updatePosition(x,y,z);
  updateRequired=true;
}

// Called when some data has changed
void WidgetEBike::onDataChange() {
  //DEBUG("data has changed",NULL);
  updateRequired=true;
}

}
