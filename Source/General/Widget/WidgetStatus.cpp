//============================================================================
// Name        : WidgetStatus.cpp
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
  core->getFontEngine()->lockFont("sansSmall",__FILE__, __LINE__);
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
    status=core->getNavigationEngine()->getStatus(__FILE__, __LINE__);

    // Then check if the map source is doing something
    if (status.size()==0)
      status=core->getMapSource()->getStatus(__FILE__, __LINE__);

    // Only display if a status is given
    if (status.size()!=0) {

      // Compute the graphical representation of the status
      fontEngine->lockFont("sansSmall",__FILE__, __LINE__);
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
