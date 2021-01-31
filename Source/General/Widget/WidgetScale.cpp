//============================================================================
// Name        : WidgetScale.cpp
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
#include <WidgetScale.h>
#include <WidgetContainer.h>
#include <FontEngine.h>
#include <MapSource.h>
#include <GraphicPosition.h>
#include <MapPosition.h>
#include <GraphicEngine.h>
#include <MapEngine.h>
#include <UnitConverter.h>

namespace GEODISCOVERER {

// Constructor
WidgetScale::WidgetScale(WidgetContainer *widgetContainer) : WidgetPrimitive(widgetContainer) {
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
  widgetContainer->getFontEngine()->lockFont("sansNormal",__FILE__, __LINE__);
  if (mapNameFontString) widgetContainer->getFontEngine()->destroyString(mapNameFontString);
  for(Int i=0;i<4;i++) {
    if (scaledNumberFontString[i]) widgetContainer->getFontEngine()->destroyString(scaledNumberFontString[i]);
  }
  widgetContainer->getFontEngine()->unlockFont();
  widgetContainer->getFontEngine()->lockFont("sansTiny",__FILE__, __LINE__);
  if (layerNameFontString) widgetContainer->getFontEngine()->destroyString(layerNameFontString);
  widgetContainer->getFontEngine()->unlockFont();
}

// Executed every time the graphic engine checks if drawing is required
bool WidgetScale::work(TimestampInMicroseconds t) {

  bool update;
  Int textX,textY;
  FontEngine *fontEngine=widgetContainer->getFontEngine();
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
    GraphicPosition *visPos=widgetContainer->getGraphicEngine()->lockPos(__FILE__, __LINE__);
    double angle=visPos->getAngleRad();
    double zoom=visPos->getZoom();
    widgetContainer->getGraphicEngine()->unlockPos();
    MapPosition pos=*(core->getMapEngine()->lockMapPos(__FILE__, __LINE__));
    core->getMapEngine()->unlockMapPos();
    MapCalibrator *calibrator=NULL;
    if ((pos.getLngScale()>0)&&(pos.getMapTile())&&(pos.getMapTile()->getParentMapContainer())&&((calibrator=pos.getMapTile()->getParentMapContainer()->getMapCalibrator())!=NULL)) {
      //DEBUG("angle=%f zoom=%f",angle,zoom);
      MapPosition pos2=pos;
      pos2.setX(iconWidth/zoom*cos(angle));
      pos2.setY(-iconWidth/zoom*sin(angle));
      //DEBUG("x=%d y=%d",pos2.getX(),pos2.getY());
      calibrator->setGeographicCoordinates(pos2);
      MapPosition pos3=pos2;
      pos3.setX(0);
      pos3.setY(0);
      calibrator->setGeographicCoordinates(pos3);
      metersPerTick=fabs(pos3.computeDistance(pos2))/3.0/core->getDefaultGraphicEngine()->getMapTileToScreenScale(core->getDefaultScreen());
      //DEBUG("metersPerTick=%f",metersPerTick);
    } else {
      metersPerTick=0;
    }
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
    std::string mapNamePostfix=" (" + unit + ")";
    fontEngine->updateString(&mapNameFontString,mapName + mapNamePostfix,getIconWidth(),mapNamePostfix.length());
    textY=y+iconHeight+mapLabelOffsetY;
    textX=x+(iconWidth-mapNameFontString->getIconWidth())/2;
    mapNameFontString->setX(textX);
    mapNameFontString->setY(textY);
    fontEngine->unlockFont();

    // Compute the layer name
    fontEngine->lockFont("sansTiny",__FILE__, __LINE__);
    std::size_t t=layerName.find_last_of(" ");
    Int keepEndCharCount=-1;
    if (t!=std::string::npos) {
      std::string layerNamePostfix=layerName.substr(t);
      //DEBUG("layerNamePostfix=%s",layerNamePostfix.c_str());
      keepEndCharCount=layerNamePostfix.length();
    }
    fontEngine->updateString(&layerNameFontString,layerName,getIconWidth(),keepEndCharCount);
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
