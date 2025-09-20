//============================================================================
// Name        : WidgetHeartRate.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2025 Matthias Gruenewald
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
#include <WidgetHeartRate.h>
#include <WidgetContainer.h>
#include <FontEngine.h>
#include <GraphicEngine.h>
#include <FloatingPoint.h>
#include <WidgetPage.h>

namespace GEODISCOVERER {

// Constructor
WidgetHeartRate::WidgetHeartRate(WidgetContainer *widgetContainer) : 
  WidgetPrimitive(widgetContainer)
{
  heartRateFontString=NULL;
  core->getConfigStore()->setIntValue("HeartRateMonitor","connected",0,__FILE__,__LINE__);
  /*core->getConfigStore()->setIntValue("HeartRateMonitor","heartRate",100,__FILE__,__LINE__);
  core->getConfigStore()->setIntValue("HeartRateMonitor","heartRateZone",4,__FILE__,__LINE__);
  core->getConfigStore()->setIntValue("HeartRateMonitor","batteryLevel",75,__FILE__,__LINE__);
  core->getConfigStore()->setIntValue("HeartRateMonitor","connected",1,__FILE__,__LINE__);*/
  connected=false;
  firstRun=true;  
  updateRequired=true;  
  setIsHidden(true);
}

// Destructor
WidgetHeartRate::~WidgetHeartRate() {
  widgetContainer->getFontEngine()->lockFont("sansNormal",__FILE__, __LINE__);
  if (heartRateFontString) widgetContainer->getFontEngine()->destroyString(heartRateFontString);
  widgetContainer->getFontEngine()->unlockFont();
}

// Executed every time the graphic engine checks if drawing is required
bool WidgetHeartRate::work(TimestampInMicroseconds t) {

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
    
    // Is heart rate monitor connected?
    boolean prevConnected=connected;
    connected=configStore->getIntValue("HeartRateMonitor","connected",__FILE__,__LINE__);
    //DEBUG("prevConnected=%d connected=%d",prevConnected,connected);
    
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
    
      /*heartRateZoneColorBackground=GraphicColor(180,180,180,255);
      heartRateZoneColorZoneOne=GraphicColor(100,100,100,255);
      heartRateZoneColorZoneTwo=GraphicColor(34,54,103,255);
      heartRateZoneColorZoneThree=GraphicColor(67,150,201,255);
      heartRateZoneColorZoneFour=GraphicColor(90,176,76,255);
      heartRateZoneColorZoneFive=GraphicColor(233,109,44,255);
      heartRateZoneWidth=0.08*getIconWidth();
      heartRateZoneHeight=0.75*getIconHeight();
      heartRateZoneOffsetX=0.21*getIconWidth();
      heartRateZoneGapX=0.02*getIconWidth();*/

      // Update the font string objects
      fontEngine->lockFont("sansNormal",__FILE__, __LINE__);
      fontEngine->updateString(&heartRateFontString,configStore->getStringValue("HeartRateMonitor","heartRate",__FILE__, __LINE__));
      fontEngine->unlockFont();
      heartRateFontString->setX(x+heartRateOffsetX-heartRateFontString->getIconWidth()/2);
      heartRateFontString->setY(y+heartRateOffsetY-heartRateFontString->getIconHeight()/2);

      // Update the battery level
      std::string value=configStore->getStringValue("HeartRateMonitor","batteryLevel",__FILE__,__LINE__);
      if (value=="?") {
        batteryLevel=0;
      } else {
        batteryLevel=atoi(value.c_str());
      }

      // Update the heart rate zone
      heartRateZone=configStore->getIntValue("HeartRateMonitor","heartRateZone",__FILE__,__LINE__);
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
      //DEBUG("enabling widget (alpha=%d, connected=%d)",getColor().getAlpha(),connected);
      setIsHidden(false);
    }
  }

  // Return result
  return changed;
}

// Executed every time the graphic engine needs to draw
void WidgetHeartRate::draw(TimestampInMicroseconds t) {
  
  // Do not draw if primitive is hidden
  if (getIsHidden())
    return;
  
  // Let the primitive draw the background
  WidgetPrimitive::draw(t);
  
  // Draw the heart rate
  if (heartRateFontString) {
    heartRateFontString->setColor(color);
    heartRateFontString->draw(t);
  }

  // Draw the battery
  drawBattery(
    t,
    batteryGaugeOffsetX,
    batteryLevel,
    NULL,
    NULL,
    batteryGaugeBackgroundWidth,
    batteryGaugeForegroundWidth,
    batteryGaugeTipWidth,
    batteryGaugeTipHeight,
    batteryGaugeOffsetY,
    batteryGaugeMaxHeight,
    false,
    0,
    0,
    NULL,
    0
  );

  // Draw the heart rate zones
  for (int z=1;z<=5;z++) {
    GraphicColor zoneColor;
    if (z>heartRateZone)
      zoneColor=heartRateZoneColorBackground;
    else {
      switch (heartRateZone) {
        case 1: zoneColor=heartRateZoneColorZoneOne; break;
        case 2: zoneColor=heartRateZoneColorZoneTwo; break;
        case 3: zoneColor=heartRateZoneColorZoneThree; break;
        case 4: zoneColor=heartRateZoneColorZoneFour; break;
        case 5: zoneColor=heartRateZoneColorZoneFive; break;
      }
    }
    zoneColor.setAlpha(color.getAlpha());
    screen->setColor(zoneColor.getRed(),zoneColor.getGreen(),zoneColor.getBlue(),zoneColor.getAlpha());
    Int x0=x+heartRateZoneOffsetX+z*(heartRateZoneWidth+heartRateZoneGapX);
    Int y0=y+(getIconHeight()-heartRateZoneHeight)/2;
    screen->drawRectangle(
      x0,
      y0+heartRateZoneHeight,
      x0+heartRateZoneWidth,
      y0,
      Screen::getTextureNotDefined(),
      true
    );
  }

  
}

// Called when the widget has changed its position
void WidgetHeartRate::updatePosition(Int x, Int y, Int z) {
  WidgetPrimitive::updatePosition(x,y,z);
  updateRequired=true;
}

// Called when some data has changed
void WidgetHeartRate::onDataChange() {
  //DEBUG("data has changed",NULL);
  updateRequired=true;
}

}
