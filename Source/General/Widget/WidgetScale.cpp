//============================================================================
// Name        : WidgetScale.cpp
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
WidgetScale::WidgetScale() : WidgetPrimitive() {
  widgetType=WidgetTypeScale;
  updateInterval=1000000;
  nextUpdateTime=0;
  tickLabelOffsetX=0;
  mapLabelOffsetY=0;
  mapNameFontString=NULL;
  scaledNumberFontString=std::vector<FontString*>(4);
  for(Int i=0;i<4;i++) {
    scaledNumberFontString[i]=NULL;
  }
}

// Destructor
WidgetScale::~WidgetScale() {
  core->getFontEngine()->lockFont("sansNormal",__FILE__, __LINE__);
  if (mapNameFontString) core->getFontEngine()->destroyString(mapNameFontString);
  for(Int i=0;i<4;i++) {
    if (scaledNumberFontString[i]) core->getFontEngine()->destroyString(scaledNumberFontString[i]);
  }
  core->getFontEngine()->unlockFont();
}

// Executed every time the graphic engine checks if drawing is required
bool WidgetScale::work(TimestampInMicroseconds t) {

  bool update;
  Int textX,textY;
  FontEngine *fontEngine=core->getFontEngine();
  double meters;
  std::string value,unit,lockedUnit;

  // Do the inherited stuff
  bool changed=WidgetPrimitive::work(t);

  // Only update the info at given update interval
  if (t>=nextUpdateTime) {

    // Get the map name
    mapName=core->getMapSource()->getFolder();

    // Compute the scale in meters
    MapPosition *pos=core->getMapEngine()->lockMapPos(__FILE__, __LINE__);
    if ((pos->getLngScale()>0)&&(pos->getMapTile())) {
      MapPosition pos2=*pos;
      pos2.setLng(pos->getLng()+(iconWidth/3)/pos->getLngScale());
      metersPerTick=pos->computeDistance(pos2);
    } else {
      metersPerTick=0;
    }
    core->getMapEngine()->unlockMapPos();
    changed=true;

    // Lock the used font
    fontEngine->lockFont("sansNormal",__FILE__, __LINE__);

    // Compute the scale numbers
    textY=y-fontEngine->getFontHeight();
    std::string lockedUnit="";
    for (Int i=0;i<4;i++) {
      meters=i*metersPerTick;
      textX=x+i*iconWidth/3+tickLabelOffsetX;
      core->getUnitConverter()->formatMeters(meters,value,unit,0,lockedUnit);
      if (i==1) {
        lockedUnit=unit;
      }
      FontString *t=scaledNumberFontString[i];
      fontEngine->updateString(&t,value);
      textX -= t->getIconWidth()/2;
      t->setX(textX);
      t->setY(textY);
      scaledNumberFontString[i]=t;
    }

    // Compute the map name
    fontEngine->updateString(&mapNameFontString,mapName + " (" + unit + ")",getIconWidth());
    textY=y+iconHeight+mapLabelOffsetY;
    textX=x+(iconWidth-mapNameFontString->getIconWidth())/2;
    mapNameFontString->setX(textX);
    mapNameFontString->setY(textY);

    // Unlock the used font
    core->getFontEngine()->unlockFont();

    // Set the next update time
    nextUpdateTime=t+updateInterval;

  }

  // Return result
  return changed;
}

// Executed every time the graphic engine needs to draw
void WidgetScale::draw(Screen *screen, TimestampInMicroseconds t) {

  // Let the primitive draw the background
  WidgetPrimitive::draw(screen,t);

  // Draw the scale numbers
  for (Int i=0;i<4;i++) {
    if (scaledNumberFontString[i]) {
      scaledNumberFontString[i]->setColor(color);
      scaledNumberFontString[i]->draw(screen,t);
    }
  }

  // Draw the map name
  if (mapNameFontString) {
    mapNameFontString->setColor(color);
    mapNameFontString->draw(screen,t);
  }

}

// Called when the widget has changed its position
void WidgetScale::updatePosition(Int x, Int y, Int z) {
  WidgetPrimitive::updatePosition(x,y,z);
  nextUpdateTime=0;
}

}
