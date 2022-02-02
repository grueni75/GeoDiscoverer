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
#include <ElevationEngine.h>
#include <GraphicRectangle.h>

namespace GEODISCOVERER {

// Constructor
WidgetScale::WidgetScale(WidgetContainer *widgetContainer) : 
    WidgetPrimitive(widgetContainer),
    altitudeIcon(widgetContainer->getScreen())
 {
  widgetType=WidgetTypeScale;
  updateInterval=1000000;
  nextUpdateTime=0;
  topLabelOffsetY=0;
  bottomLabelOffsetY=0;
  backgroundWidth=0;
  backgroundHeight=0;
  backgroundAlpha=0;
  topLabelFontString=NULL;
  bottomLabelLeftFontString=NULL;
  bottomLabelRightFontString=NULL;
  altitudeIconScale=1.0;
  altitudeIconX=0;
  altitudeIconY=0;
}

// Destructor
WidgetScale::~WidgetScale() {
  widgetContainer->getFontEngine()->lockFont("sansSmall",__FILE__, __LINE__);
  if (bottomLabelLeftFontString) widgetContainer->getFontEngine()->destroyString(bottomLabelLeftFontString);
  if (bottomLabelRightFontString) widgetContainer->getFontEngine()->destroyString(bottomLabelRightFontString);
  widgetContainer->getFontEngine()->unlockFont();
  widgetContainer->getFontEngine()->lockFont("sansTiny",__FILE__, __LINE__);
  if (topLabelFontString) widgetContainer->getFontEngine()->destroyString(topLabelFontString);
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
  //DEBUG("t=%ld nextUpdateTime=%ld",t,nextUpdateTime);
  if (t>=nextUpdateTime) {

    // Compute the scale in meters
    GraphicPosition *visPos=widgetContainer->getGraphicEngine()->lockPos(__FILE__, __LINE__);
    double angle=visPos->getAngleRad();
    double zoom=visPos->getZoom();
    widgetContainer->getGraphicEngine()->unlockPos();
    MapPosition pos=*(core->getMapEngine()->lockMapPos(__FILE__, __LINE__));
    core->getMapEngine()->unlockMapPos();
    MapCalibrator *calibrator=NULL;
    //DEBUG("%s %f 0x%08x",layerName.c_str(),pos.getLngScale(),pos.getMapTile());
    double metersPerTick;
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

    // Compute the bottom label
    fontEngine->lockFont("sansSmall",__FILE__, __LINE__);
    std::stringstream bottomLabel;
    meters=3*metersPerTick;
    if (meters>0) {
      core->getUnitConverter()->formatMeters(meters,value,unit,0);
      bottomLabel << value << " " << unit;
    } else {
      bottomLabel << "?";
    }
    bottomLabel << " (";
    fontEngine->updateString(&bottomLabelLeftFontString,bottomLabel.str());
    bottomLabel.str(" ");
    core->getElevationEngine()->getElevation(&pos);
    if (pos.getHasAltitude()) {
      core->getUnitConverter()->formatMeters(pos.getAltitude(),value,unit,0,"m");
      bottomLabel << value << " " << unit;
    } else {
      bottomLabel << "?";
    }
    bottomLabel << ")";
    fontEngine->updateString(&bottomLabelRightFontString,bottomLabel.str());
    bottomLabelLeftFontString->setY(y+bottomLabelOffsetY);
    altitudeIcon.setY(0);
    altitudeIconY=y+bottomLabelOffsetY;
    altitudeIconScale=((double)bottomLabelLeftFontString->getIconHeight()+bottomLabelLeftFontString->getBaselineOffsetY())/((double)altitudeIcon.getIconHeight());
    Int altitudeIconWidth=altitudeIcon.getIconWidth()*altitudeIconScale;
    bottomLabelRightFontString->setY(y+bottomLabelOffsetY);
    Int totalWidth=bottomLabelLeftFontString->getIconWidth()+bottomLabelRightFontString->getIconWidth()+altitudeIconWidth;
    Int startX=x+iconWidth/2-totalWidth/2;
    bottomLabelLeftFontString->setX(startX);
    startX+=bottomLabelLeftFontString->getIconWidth();
    altitudeIcon.setX(0);
    altitudeIconX=startX;
    startX+=altitudeIconWidth;
    bottomLabelRightFontString->setX(startX);
    fontEngine->unlockFont();

    // Compute the top label
    fontEngine->lockFont("sansTiny",__FILE__, __LINE__);
    std::string layerName=core->getMapSource()->getMapLayerName(core->getMapEngine()->getZoomLevel());
    std::size_t i=layerName.find_last_of(" ");
    Int keepEndCharCount=-1;
    if (i!=std::string::npos) {
      std::string layerNamePostfix=layerName.substr(i);
      //DEBUG("layerNamePostfix=%s",layerNamePostfix.c_str());
      keepEndCharCount=layerNamePostfix.length();
    }
    fontEngine->updateString(&topLabelFontString,layerName,getIconWidth(),keepEndCharCount);
    topLabelFontString->setX(x+iconWidth/2-topLabelFontString->getIconWidth()/2);
    topLabelFontString->setY(y+topLabelOffsetY);
    fontEngine->unlockFont();

    // Set the next update time
    nextUpdateTime=t+updateInterval;
    //DEBUG("nextUpdateTime=%ld",nextUpdateTime);
  }

  // Return result
  return changed;
}

// Executed every time the graphic engine needs to draw
void WidgetScale::draw(TimestampInMicroseconds t) {

  // Draw the background
  screen->startObject();
  screen->translate(x+iconWidth/2,y+iconHeight/2,0);
  screen->setColor(getColor().getRed(),getColor().getGreen(),getColor().getBlue(),color.getAlpha()*backgroundAlpha/255);
  screen->drawRoundedRectangle(backgroundWidth,backgroundHeight);
  screen->endObject();

  // Let the primitive draw the scale icon
  WidgetPrimitive::draw(t);

  // Draw all labels and icons
  if (topLabelFontString) {
    topLabelFontString->setColor(color);
    topLabelFontString->draw(t);
  }
  if (bottomLabelLeftFontString) {
    bottomLabelLeftFontString->setColor(color);
    bottomLabelLeftFontString->draw(t);
    screen->startObject();
    screen->translate(altitudeIconX,altitudeIconY,altitudeIcon.getZ());
    screen->scale(altitudeIconScale,altitudeIconScale,1.0);
    altitudeIcon.setColor(color);
    altitudeIcon.draw(t);
    screen->endObject();
    bottomLabelRightFontString->setColor(color);
    bottomLabelRightFontString->draw(t);
  }
}

// Called when the widget has changed its position
void WidgetScale::updatePosition(Int x, Int y, Int z) {
  WidgetPrimitive::updatePosition(x,y,z);
  nextUpdateTime=0;
}

}
