//============================================================================
// Name        : WidgetCursorInfo.cpp
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
WidgetCursorInfo::WidgetCursorInfo(WidgetPage *widgetPage) : WidgetPrimitive(widgetPage) {
  widgetType=WidgetTypeCursorInfo;
  updateInterval=100000;
  labelWidth=0;
  nextUpdateTime=0;
  labelPos=0;
  infoFontString=NULL;
  info="";
  isHidden=true;
}

// Destructor
WidgetCursorInfo::~WidgetCursorInfo() {
  widgetPage->getFontEngine()->lockFont("sansSmall",__FILE__, __LINE__);
  if (infoFontString) widgetPage->getFontEngine()->destroyString(infoFontString);
  widgetPage->getFontEngine()->unlockFont();
}

// Executed every time the graphic engine checks if drawing is required
bool WidgetCursorInfo::work(TimestampInMicroseconds t) {

  FontEngine *fontEngine=widgetPage->getFontEngine();
  Int textX, textY;
  std::list<std::string> status;

  // Do the inherited stuff
  bool changed=WidgetPrimitive::work(t);

  // Only update the info at given update interval
  if (t>=nextUpdateTime) {

    /*
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
      widgetPage->getFontEngine()->unlockFont();

      // Start fade animation if the status was not displayed before
      if ((color.getAlpha()==0)&&(fadeStartTime==fadeEndTime)) {
        if (widgetPage->getWidgetEngine()->getWidgetsActive())
          setFadeAnimation(t,color,this->activeColor,false,widgetPage->getGraphicEngine()->getFadeDuration());
        else
          setFadeAnimation(t,color,this->inactiveColor,false,widgetPage->getGraphicEngine()->getFadeDuration());
      }
      isHidden=false;

    } else {

      // Start fade animation if the status was not displayed before
      if ((color.getAlpha()!=0)&&(fadeStartTime==fadeEndTime)) {
        setFadeAnimation(t,color,GraphicColor(255,255,255,0),false,widgetPage->getGraphicEngine()->getFadeDuration());
      }
      isHidden=true;
    }

    // Set the next update time
    nextUpdateTime=t+updateInterval;

     */

  }

  // Return result
  return changed;
}

// Executed every time the graphic engine needs to draw
void WidgetCursorInfo::draw(TimestampInMicroseconds t) {

  // Let the primitive draw the background
  WidgetPrimitive::draw(t);

  // Draw the status
  if (color.getAlpha()!=0) {
    if (infoFontString) {
      infoFontString->setColor(color);
      infoFontString->draw(t);
    }
  }

}

// Called when the widget has changed its position
void WidgetCursorInfo::updatePosition(Int x, Int y, Int z) {
  WidgetPrimitive::updatePosition(x,y,z);
  nextUpdateTime=0;
}

// Called when the map has changed
void WidgetCursorInfo::onMapChange(bool widgetVisible, MapPosition pos) {

  // Check if cursor is above address point icon


}

} /* namespace GEODISCOVERER */
