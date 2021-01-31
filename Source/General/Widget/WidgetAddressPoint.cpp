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
#include <WidgetAddressPoint.h>
#include <WidgetContainer.h>
#include <FontEngine.h>
#include <WidgetEngine.h>
#include <NavigationPoint.h>
#include <Device.h>
#include <NavigationEngine.h>
#include <UnitConverter.h>
#include <GraphicEngine.h>

namespace GEODISCOVERER {

// Constructor
WidgetAddressPoint::WidgetAddressPoint(WidgetContainer *widgetContainer) : WidgetPrimitive(widgetContainer) {
  widgetType=WidgetTypeAddressPoint;
  statusFontString=NULL;
  nextUpdateTime=0;
  updateInterval=1*1000*1000;
  firstRun=true;
  lastClockUpdate=0;
  active=true;
  hideIfNoAddressPointNear=(widgetContainer->getWidgetEngine()->getDevice()->getName()=="Default");
}

// Destructor
WidgetAddressPoint::~WidgetAddressPoint() {
  widgetContainer->getFontEngine()->lockFont("sansNormal",__FILE__, __LINE__);
  if (statusFontString) widgetContainer->getFontEngine()->destroyString(statusFontString);
  widgetContainer->getFontEngine()->unlockFont();
}

// Executed every time the graphic engine checks if drawing is required
bool WidgetAddressPoint::work(TimestampInMicroseconds t) {

  FontEngine *fontEngine=widgetContainer->getFontEngine();
  std::string value;
  std::string unit;
  bool update;

  // Do the inherited stuff
  bool changed=WidgetPrimitive::work(t);

  // Only update the info at given update interval
  if ((firstRun)||(t>=nextUpdateTime)) {

    // Hide the widget on the first run
    if ((hideIfNoAddressPointNear)&&(firstRun)) {
      color.setAlpha(0);
    }

    // Are we near enough to an address point?
    boolean activateWidget=false;
    double distance;
    TimestampInMicroseconds updateTimestamp;
    NavigationPoint navigationPoint;
    bool alarm;
    bool found=core->getNavigationEngine()->getNearestAddressPoint(navigationPoint,distance,updateTimestamp,alarm);
    if ((found)&&(alarm)) {
      std::stringstream status;
      std::string value,unit;
      core->getUnitConverter()->formatMeters(distance,value,unit);
      status << "In " << value << " " << unit << ": " << navigationPoint.getName();
      nearestAddressPointName=navigationPoint.getName();
      fontEngine->lockFont("sansNormal",__FILE__, __LINE__);
      fontEngine->updateString(&statusFontString,status.str(),iconWidth*0.95);
      fontEngine->unlockFont();
      statusFontString->setX(x+(iconWidth-statusFontString->getIconWidth())/2);
      statusFontString->setY(y+(iconHeight-statusFontString->getIconHeight())/2-statusFontString->getBaselineOffsetY());
      changed=true;
      activateWidget=true;
    } else {
      TimestampInSeconds t2=core->getClock()->getSecondsSinceEpoch();
      nearestAddressPointName="";
      if ((t2/60!=lastClockUpdate/60)||(firstRun)) {
        fontEngine->lockFont("sansNormal",__FILE__, __LINE__);
        fontEngine->updateString(&statusFontString,core->getClock()->getFormattedDate(t2,"%A -- %H:%M -- %Y-%m-%d",true));
        //fontEngine->updateString(&statusFontString,core->getClock()->getFormattedDate(t2,"%c",true));
        fontEngine->unlockFont();
        lastClockUpdate=t2;
        statusFontString->setX(x+(iconWidth-statusFontString->getIconWidth())/2);
        statusFontString->setY(y+(iconHeight-statusFontString->getIconHeight())/2-statusFontString->getBaselineOffsetY());
        changed=true;
      }
    }

    // Activate widget if not already
    if (hideIfNoAddressPointNear) {
      if (activateWidget!=active) {
        DEBUG("activateWidget=%d active=%d",activateWidget,active);
        GraphicColor c=getActiveColor();
        if (activateWidget) {
          //DEBUG("activating widget",NULL);
          c.setAlpha(255);
          setFadeAnimation(updateTimestamp,getColor(),c,false,widgetContainer->getGraphicEngine()->getFadeDuration());
        } else {
          //DEBUG("de-activating widget",NULL);
          c.setAlpha(0);
          setFadeAnimation(updateTimestamp,getColor(),c,false,widgetContainer->getGraphicEngine()->getFadeDuration());
        }
        active=activateWidget;
      }
    } else {
      active=false;
    }

    // Set the next update time
    firstRun=false;
    nextUpdateTime=t+updateInterval;
  }

  // Hide the widget if not visible
  if (color.getAlpha()==0) {
    if (!getIsHidden()) setIsHidden(true);
  } else {
    if (getIsHidden()) setIsHidden(false);
  }

  // Return result
  return changed;
}

// Executed every time the graphic engine needs to draw
void WidgetAddressPoint::draw(TimestampInMicroseconds t) {

  // Let the primitive draw the background
  WidgetPrimitive::draw(t);

  // Draw the text
  if (statusFontString) {
    statusFontString->setColor(color);
    statusFontString->draw(t);
  }
}

// Called when the widget has changed its position
void WidgetAddressPoint::updatePosition(Int x, Int y, Int z) {
  WidgetPrimitive::updatePosition(x,y,z);
  nextUpdateTime=0;
  firstRun=true;
}

// Called when some data has changed
void WidgetAddressPoint::onDataChange() {
  nextUpdateTime=0;
}

}
