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
WidgetScale::WidgetScale(WidgetPage *widgetPage) : WidgetPrimitive(widgetPage) {
  widgetType=WidgetTypeScale;
  updateInterval=1000000;
  nextUpdateTime=0;
  tickLabelOffsetX=0;
  mapLabelOffsetY=0;
  layerLabelOffsetY=0;
  mapNameFontString=NULL;
  layerNameFontString=NULL;
  scaledNumberFontString=std::vector<FontString*>(4);
  for(Int i=0;i<4;i++) {
    scaledNumberFontString[i]=NULL;
  }
}

// Destructor
WidgetScale::~WidgetScale() {
  widgetPage->getFontEngine()->lockFont("sansNormal",__FILE__, __LINE__);
  if (mapNameFontString) widgetPage->getFontEngine()->destroyString(mapNameFontString);
  for(Int i=0;i<4;i++) {
    if (scaledNumberFontString[i]) widgetPage->getFontEngine()->destroyString(scaledNumberFontString[i]);
  }
  widgetPage->getFontEngine()->unlockFont();
  widgetPage->getFontEngine()->lockFont("sansTiny",__FILE__, __LINE__);
  if (layerNameFontString) widgetPage->getFontEngine()->destroyString(layerNameFontString);
  widgetPage->getFontEngine()->unlockFont();
}

// Executed every time the graphic engine checks if drawing is required
bool WidgetScale::work(TimestampInMicroseconds t) {

  bool update;
  Int textX,textY;
  FontEngine *fontEngine=widgetPage->getFontEngine();
  double meters;
  std::string value,unit,lockedUnit;

  // Do the inherited stuff
  bool changed=WidgetPrimitive::work(t);

  // Only update the info at given update interval
  if (t>=nextUpdateTime) {

    // Get the map and layer name
    mapName=core->getMapSource()->getFolder();
    layerName=core->getMapSource()->getMapLayerName(core->getMapEngine()->getZoomLevel());

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
    fontEngine->unlockFont();

    // Compute the layer name
    fontEngine->lockFont("sansTiny",__FILE__, __LINE__);
    fontEngine->updateString(&layerNameFontString,layerName,getIconWidth());
    textY=y+iconHeight+layerLabelOffsetY;
    textX=x+(iconWidth-layerNameFontString->getIconWidth())/2;
    layerNameFontString->setX(textX);
    layerNameFontString->setY(textY);

    // Unlock the used font
    fontEngine->unlockFont();

    // Set the next update time
    nextUpdateTime=t+updateInterval;

  }

  // Return result
  return changed;
}

// Executed every time the graphic engine needs to draw
void WidgetScale::draw(TimestampInMicroseconds t) {

  // Let the primitive draw the background
  WidgetPrimitive::draw(t);

  // Draw the scale numbers
  for (Int i=0;i<4;i++) {
    if (scaledNumberFontString[i]) {
      scaledNumberFontString[i]->setColor(color);
      scaledNumberFontString[i]->draw(t);
    }
  }

  // Draw the map name
  if (mapNameFontString) {
    mapNameFontString->setColor(color);
    mapNameFontString->draw(t);
  }

  // Draw the layer name
  if (layerNameFontString) {
    layerNameFontString->setColor(color);
    layerNameFontString->draw(t);
  }

}

// Called when the widget has changed its position
void WidgetScale::updatePosition(Int x, Int y, Int z) {
  WidgetPrimitive::updatePosition(x,y,z);
  nextUpdateTime=0;
}

}
