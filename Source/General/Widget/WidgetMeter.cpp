//============================================================================
// Name        : WidgetMeter.cpp
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
#include <WidgetMeter.h>
#include <WidgetContainer.h>
#include <FontEngine.h>
#include <NavigationEngine.h>
#include <UnitConverter.h>
#include <NavigationPath.h>

namespace GEODISCOVERER {

// Constructor
WidgetMeter::WidgetMeter(WidgetContainer *widgetContainer) : WidgetPrimitive(widgetContainer) {
  widgetType=WidgetTypeMeter;
  valueFontString=NULL;
  unitFontString=NULL;
  labelFontString=NULL;
  setMeterType(WidgetMeterTypeAltitude);
  nextUpdateTime=0;
  lastWorkTime=0;
  updateInterval=1*1000*1000;
}

// Destructor
WidgetMeter::~WidgetMeter() {
  widgetContainer->getFontEngine()->lockFont("sansNormal",__FILE__, __LINE__);
  if (unitFontString) widgetContainer->getFontEngine()->destroyString(unitFontString);
  widgetContainer->getFontEngine()->unlockFont();
  widgetContainer->getFontEngine()->lockFont("sansBoldNormal",__FILE__, __LINE__);
  if (labelFontString) widgetContainer->getFontEngine()->destroyString(labelFontString);
  widgetContainer->getFontEngine()->unlockFont();
  widgetContainer->getFontEngine()->lockFont("sansLarge",__FILE__, __LINE__);
  if (valueFontString) widgetContainer->getFontEngine()->destroyString(valueFontString);
  widgetContainer->getFontEngine()->unlockFont();
}

// Executed every time the graphic engine checks if drawing is required
bool WidgetMeter::work(TimestampInMicroseconds t) {

  FontEngine *fontEngine=widgetContainer->getFontEngine();
  std::string value;
  std::string unit;
  bool update;

  // Do the inherited stuff
  bool changed=WidgetPrimitive::work(t);

  // Check for overflow in microseconds counter
  if (t<lastWorkTime) {
    DEBUG("microseconds overflow detected (before=%llu, after=%llu)",lastWorkTime,t);
    nextUpdateTime=t;
  }
  lastWorkTime=t;

  // Only update the info at given update interval
  if (t>=nextUpdateTime) {

    // Get the new value
    MapPosition *locationPos;
    NavigationPath *trackPath;
    switch(meterType) {
      case WidgetMeterTypeAltitude:
        locationPos=core->getNavigationEngine()->lockLocationPos(__FILE__, __LINE__);
        if (locationPos->getHasAltitude()) {
          core->getUnitConverter()->formatMeters(locationPos->getAltitude(),value,unit);
        } else {
          core->getUnitConverter()->formatMeters(0,value,unit);
          value="--";
        }
        core->getNavigationEngine()->unlockLocationPos();
        break;
      case WidgetMeterTypeSpeed:
        locationPos=core->getNavigationEngine()->lockLocationPos(__FILE__, __LINE__);
        if (locationPos->getHasSpeed()) {
          core->getUnitConverter()->formatMetersPerSecond(locationPos->getSpeed(),value,unit);
        } else {
          core->getUnitConverter()->formatMetersPerSecond(0,value,unit);
          value="--";
        }
        core->getNavigationEngine()->unlockLocationPos();
        break;
      case WidgetMeterTypeTrackLength:
        trackPath=core->getNavigationEngine()->lockRecordedTrack(__FILE__, __LINE__);
        if (trackPath)
          core->getUnitConverter()->formatMeters(trackPath->getLength(),value,unit);
        else {
          core->getUnitConverter()->formatMeters(0,value,unit);
          value="--";
        }
        core->getNavigationEngine()->unlockRecordedTrack();
        break;
      default:
        FATAL("unknown meter type",NULL);
        return false;
    }
    changed=true;

    // Update the font string objects
    fontEngine->lockFont("sansNormal",__FILE__, __LINE__);
    fontEngine->updateString(&unitFontString,unit);
    fontEngine->unlockFont();
    unitFontString->setX(x+(iconWidth-unitFontString->getIconWidth())/2);
    unitFontString->setY(y+unitY);
    fontEngine->lockFont("sansLarge",__FILE__, __LINE__);
    fontEngine->updateString(&valueFontString,value);
    fontEngine->unlockFont();
    valueFontString->setX(x+(iconWidth-valueFontString->getIconWidth())/2);
    valueFontString->setY(y+valueY);
    labelFontString->setX(x+(iconWidth-labelFontString->getIconWidth())/2);
    labelFontString->setY(y+labelY);

    // Set the next update time
    nextUpdateTime=t+updateInterval;

  }

  // Return result
  return changed;
}

// Executed every time the graphic engine needs to draw
void WidgetMeter::draw(TimestampInMicroseconds t) {

  // Let the primitive draw the background
  WidgetPrimitive::draw(t);

  // Draw the text
  if (labelFontString) {
    labelFontString->setColor(color);
    labelFontString->draw(t);
  }
  if (valueFontString) {
    valueFontString->setColor(color);
    valueFontString->draw(t);
  }
  if (unitFontString) {
    unitFontString->setColor(color);
    unitFontString->draw(t);
  }
}

// Sets the type of meter
void WidgetMeter::setMeterType(WidgetMeterType meterType)
{
  FontEngine *fontEngine=widgetContainer->getFontEngine();

  // Set the type
  this->meterType=meterType;

  // Set the label
  fontEngine->lockFont("sansBoldNormal",__FILE__, __LINE__);
  if (labelFontString) {
    fontEngine->destroyString(labelFontString);
    labelFontString=NULL;
  }
  switch(meterType) {
    case WidgetMeterTypeAltitude:
      labelFontString=fontEngine->createString("Altitude");
      break;
    case WidgetMeterTypeSpeed:
      labelFontString=fontEngine->createString("Speed");
      break;
    case WidgetMeterTypeTrackLength:
      labelFontString=fontEngine->createString("Track");
      break;
    default:
      FATAL("meter type not supported",NULL);
      return;
  }
  fontEngine->unlockFont();
}

// Called when the widget has changed its position
void WidgetMeter::updatePosition(Int x, Int y, Int z) {
  WidgetPrimitive::updatePosition(x,y,z);
  nextUpdateTime=0;
}

}
