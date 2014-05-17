//============================================================================
// Name        : WidgetStatus.cpp
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

// Constructor
WidgetStatus::WidgetStatus() : WidgetPrimitive() {
  widgetType=WidgetTypeStatus;
  updateInterval=100000;
  labelWidth=0;
  nextUpdateTime=0;
  firstStatusFontString=NULL;
  secondStatusFontString=NULL;
  isHidden=true;
}

// Destructor
WidgetStatus::~WidgetStatus() {
  core->getFontEngine()->lockFont("sansSmall");
  if (firstStatusFontString) core->getFontEngine()->destroyString(firstStatusFontString);
  if (secondStatusFontString) core->getFontEngine()->destroyString(secondStatusFontString);
  core->getFontEngine()->unlockFont();
}

// Executed every time the graphic engine checks if drawing is required
bool WidgetStatus::work(TimestampInMicroseconds t) {

  FontEngine *fontEngine=core->getFontEngine();
  Int textX, textY;
  std::list<std::string> status;

  // Do the inherited stuff
  bool changed=WidgetPrimitive::work(t);

  // Only update the info at given update interval
  if (t>=nextUpdateTime) {

    // First check if the navigation engine is doing something
    status=core->getNavigationEngine()->getStatus();

    // Then check if the map source is doing something
    if (status.size()==0)
      status=core->getMapSource()->getStatus();

    // Only display if a status is given
    if (status.size()!=0) {

      // Compute the graphical representation of the status
      fontEngine->lockFont("sansSmall");
      fontEngine->updateString(&firstStatusFontString,status.front(),labelWidth);
      textY=y+(iconHeight/2)+fontEngine->getLineHeight()/2-fontEngine->getFontHeight()/3;
      textX=x+(iconWidth/2)-(firstStatusFontString->getIconWidth())/2;
      firstStatusFontString->setX(textX);
      firstStatusFontString->setY(textY);
      fontEngine->updateString(&secondStatusFontString,status.back(),labelWidth);
      textY=y+(iconHeight/2)-fontEngine->getLineHeight()/2-fontEngine->getFontHeight()/3;
      textX=x+(iconWidth/2)-(secondStatusFontString->getIconWidth())/2;
      secondStatusFontString->setX(textX);
      secondStatusFontString->setY(textY);
      core->getFontEngine()->unlockFont();

      // Start fade animation if the status was not displayed before
      if ((color.getAlpha()==0)&&(fadeStartTime==fadeEndTime)) {
        if (core->getWidgetEngine()->getWidgetsActive())
          setFadeAnimation(t,color,this->activeColor,false,core->getGraphicEngine()->getFadeDuration());
        else
          setFadeAnimation(t,color,this->inactiveColor,false,core->getGraphicEngine()->getFadeDuration());
      }
      isHidden=false;

    } else {

      // Start fade animation if the status was not displayed before
      if ((color.getAlpha()!=0)&&(fadeStartTime==fadeEndTime)) {
        setFadeAnimation(t,color,GraphicColor(255,255,255,0),false,core->getGraphicEngine()->getFadeDuration());
      }
      isHidden=true;
    }

    // Set the next update time
    nextUpdateTime=t+updateInterval;

  }

  // Return result
  return changed;
}

// Executed every time the graphic engine needs to draw
void WidgetStatus::draw(Screen *screen, TimestampInMicroseconds t) {

  // Let the primitive draw the background
  WidgetPrimitive::draw(screen,t);

  // Draw the status
  if (color.getAlpha()!=0) {
    if (firstStatusFontString) {
      firstStatusFontString->setColor(color);
      firstStatusFontString->draw(screen,t);
    }
    if (secondStatusFontString) {
      secondStatusFontString->setColor(color);
      secondStatusFontString->draw(screen,t);
    }
  }

}

// Called when the widget has changed its position
void WidgetStatus::updatePosition(Int x, Int y, Int z) {
  WidgetPrimitive::updatePosition(x,y,z);
  nextUpdateTime=0;
}

} /* namespace GEODISCOVERER */
