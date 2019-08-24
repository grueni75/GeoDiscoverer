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

namespace GEODISCOVERER {

// Constructor
WidgetAddressPoint::WidgetAddressPoint(WidgetPage *widgetPage) : WidgetPrimitive(widgetPage) {
  widgetType=WidgetTypeAddressPoint;
  statusFontString=NULL;
  nextUpdateTime=0;
  updateInterval=1*1000*1000;
  firstRun=true;
  lastClockUpdate=0;
  active=false;
  hideIfNoAddressPointNear=(widgetPage->getWidgetEngine()->getDevice()->getName()=="Default");
  setIsHidden(hideIfNoAddressPointNear);
  maxAddressPointAlarmDistance=core->getConfigStore()->getDoubleValue("Navigation","maxAddressPointAlarmDistance",__FILE__,__LINE__);
}

// Destructor
WidgetAddressPoint::~WidgetAddressPoint() {
  widgetPage->getFontEngine()->lockFont("sansNormal",__FILE__, __LINE__);
  if (statusFontString) widgetPage->getFontEngine()->destroyString(statusFontString);
  widgetPage->getFontEngine()->unlockFont();
}

// Executed every time the graphic engine checks if drawing is required
bool WidgetAddressPoint::work(TimestampInMicroseconds t) {

  FontEngine *fontEngine=widgetPage->getFontEngine();
  std::string value;
  std::string unit;
  bool update;

  // Do the inherited stuff
  bool changed=WidgetPrimitive::work(t);

  // Only update the info at given update interval
  if (t>=nextUpdateTime) {

    // Are we near enough to an address point?
    boolean activateWidget=false;
    double distance;
    TimestampInMicroseconds updateTimestamp;
    NavigationPoint navigationPoint;
    bool found=core->getNavigationEngine()->getNearestAddressPoint(navigationPoint,distance,updateTimestamp);
    if ((found)&&(distance<=maxAddressPointAlarmDistance)) {
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
    if ((hideIfNoAddressPointNear)||(!widgetPage->getWidgetEngine()->getWidgetsActive())) {
      //DEBUG("activateWidget=%d active=%d",activateWidget,active);
      if (activateWidget!=active) {
        if (activateWidget) {
          //DEBUG("activating widget",NULL);
          setFadeAnimation(updateTimestamp,getColor(),getActiveColor(),false,widgetPage->getGraphicEngine()->getFadeDuration());
        } else {
          //DEBUG("de-activating widget",NULL);
          setFadeAnimation(updateTimestamp,getColor(),getInactiveColor(),false,widgetPage->getGraphicEngine()->getFadeDuration());
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
