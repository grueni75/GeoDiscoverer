//============================================================================
// Name        : WidgetScale.cpp
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
    MapPosition *pos=core->getMapEngine()->lockMapPos();
    if (pos->getLngScale()>0) {
      MapPosition pos2=*pos;
      pos2.setLng(pos->getLng()+(iconWidth/3)/pos->getLngScale());
      metersPerTick=pos->computeDistance(pos2);
    } else {
      metersPerTick=0;
    }
    core->getMapEngine()->unlockMapPos();
    changed=true;

    // Compute the scale numbers
    fontEngine->setFont("sansNormal");
    textY=y-fontEngine->getFontHeight();
    std::string lockedUnit="";
    for (Int i=0;i<4;i++) {
      meters=i*metersPerTick;
      textX=x+i*iconWidth/3+tickLabelOffsetX;
      core->getUnitConverter()->formatMeters(meters,value,unit,0,lockedUnit);
      if (i==1) {
        lockedUnit=unit;
      }
      if (i==3) {
        value=value+" "+unit;
      }
      FontString *t=scaledNumberFontString[i];
      fontEngine->updateString(&t,value);
      t->setX(textX);
      t->setY(textY);
      scaledNumberFontString[i]=t;
    }

    // Compute the map name
    fontEngine->updateString(&mapNameFontString,mapName);
    textY=y+iconHeight+mapLabelOffsetY;
    textX=x+(iconWidth-mapNameFontString->getIconWidth())/2;
    mapNameFontString->setX(textX);
    mapNameFontString->setY(textY);

    // Set the next update time
    nextUpdateTime+=updateInterval;

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
    scaledNumberFontString[i]->setColor(color);
    scaledNumberFontString[i]->draw(screen,t);
  }

  // Draw the map name
  mapNameFontString->setColor(color);
  mapNameFontString->draw(screen,t);

}

}