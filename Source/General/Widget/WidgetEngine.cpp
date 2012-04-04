//============================================================================
// Name        : WidgetEngine.cpp
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
WidgetEngine::WidgetEngine() {

  // Get global config
  ConfigStore *c=core->getConfigStore();
  selectedWidgetColor=core->getConfigStore()->getGraphicColorValue("Widget/SelectedColor","selected widget",GraphicColor(255,127,0,255));
  buttonRepeatDelay=c->getIntValue("Widget","buttonRepeatDelay","Time to wait before dispatching repeating commands [us]",500000);
  buttonRepeatPeriod=c->getIntValue("Widget","buttonRepeatPeriod","Time distance between command dispatching [us]",100000);

  // Init the rest
  init();
}

// Destructor
WidgetEngine::~WidgetEngine() {
  deinit();
}

// Adds a widget to a page
void WidgetEngine::addWidgetToPage(
  std::string pageName,
  WidgetType widgetType,
  std::string widgetName,
  double portraitX, double portraitY, Int portraitZ,
  double landscapeX, double landscapeY, Int landscapeZ,
  UByte activeRed, UByte activeGreen, UByte activeBlue, UByte activeAlpha,
  UByte inactiveRed, UByte inactiveGreen, UByte inactiveBlue, UByte inactiveAlpha,
  ParameterMap parameters) {

  std::string path="Widget/Page[@name='" + pageName + "']/Primitive[@name='" + widgetName + "']";
  ConfigStore *c=core->getConfigStore();
  std::string widgetTypeString="unknown";
  switch(widgetType) {
    case WidgetTypeButton: widgetTypeString="button"; break;
    case WidgetTypeCheckbox: widgetTypeString="checkbox"; break;
    case WidgetTypeMeter: widgetTypeString="meter"; break;
    case WidgetTypeScale: widgetTypeString="scale"; break;
    case WidgetTypeStatus: widgetTypeString="status"; break;
    default: FATAL("unknown widget type",NULL); break;
  }
  c->getStringValue(path,"type","Kind of widget",widgetTypeString);
  c->getDoubleValue(path + "/Portrait","x","X position in percent",portraitX);
  c->getDoubleValue(path + "/Portrait","y","Y position in percent",portraitY);
  c->getIntValue(path + "/Portrait","z","Z position in pixel",portraitZ);
  c->getDoubleValue(path + "/Landscape","x","X position in percent",landscapeX);
  c->getDoubleValue(path + "/Landscape","y","Y position in percent",landscapeY);
  c->getIntValue(path + "/Landscape","z","Z position in pixel",landscapeZ);
  c->getGraphicColorValue(path + "/ActiveColor","active widget",GraphicColor(activeRed,activeGreen,activeBlue,activeAlpha));
  c->getGraphicColorValue(path + "/InactiveColor","inactive widget",GraphicColor(inactiveRed,inactiveGreen,inactiveBlue,inactiveAlpha));
  switch(widgetType) {
    case WidgetTypeButton:
      c->getStringValue(path,"iconFilename","Filename of the background image",parameters["iconFilename"]);
      c->getStringValue(path,"command","Command to execute if widget is clicked",parameters["command"]);
      break;
    case WidgetTypeCheckbox:
      c->getStringValue(path,"checkedIconFilename","Filename of the background image showing the unchecked state",parameters["checkedIconFilename"]);
      c->getStringValue(path,"checkedCommand","Command to execute if widget is unchecked",parameters["checkedCommand"]);
      c->getStringValue(path,"uncheckedIconFilename","Filename of the background image showing the checked state",parameters["uncheckedIconFilename"]);
      c->getStringValue(path,"uncheckedCommand","Command to execute if widget is checked",parameters["uncheckedCommand"]);
      c->getStringValue(path,"stateConfigPath","Path to the config entry that holds the check state",parameters["stateConfigPath"]);
      c->getStringValue(path,"stateConfigName","Name of the config entry that holds the check state",parameters["stateConfigName"]);
      break;
    case WidgetTypeMeter:
      c->getStringValue(path,"iconFilename","Filename of the background image",parameters["iconFilename"]);
      c->getStringValue(path,"meterType","Kind of meter to display",parameters["meterType"]);
      c->getStringValue(path,"updateInterval","Time in microseconds to elapse before updating the widget",parameters["updateInterval"]);
      c->getStringValue(path,"labelY","Vertical position of the label in percent of the widget height",parameters["labelY"]);
      c->getStringValue(path,"valueY","Vertical position of the value in percent of the widget height",parameters["valueY"]);
      c->getStringValue(path,"unitY","Vertical position of the unit in percent of the widget height",parameters["unitY"]);
      break;
    case WidgetTypeScale:
      c->getStringValue(path,"iconFilename","Filename of the background image",parameters["iconFilename"]);
      c->getStringValue(path,"updateInterval","Time in microseconds to elapse before updating the widget",parameters["updateInterval"]);
      c->getStringValue(path,"tickLabelOffsetX","Horizontal offset for the scale values in percent of the widget width",parameters["tickLabelOffsetX"]);
      c->getStringValue(path,"mapLabelOffsetY","Vertical offset for the map label in percent of the widget height",parameters["mapLabelOffsetY"]);
      break;
    case WidgetTypeStatus:
      c->getStringValue(path,"labelWidth","Width of the text",parameters["labelWidth"]);
      c->getStringValue(path,"iconFilename","Filename of the background image",parameters["iconFilename"]);
      c->getStringValue(path,"updateInterval","Time in microseconds to elapse before updating the widget",parameters["updateInterval"]);
      break;
  }
}

// (Re)creates all widget pages from the current config
void WidgetEngine::init() {

  ConfigStore *c=core->getConfigStore();

  // Get all widget pages
  // If no exist, create the default ones
  std::list<std::string> pageNames=c->getAttributeValues("Widget/Page","name");
  if (pageNames.size()==0) {
    ParameterMap parameters;
    parameters.clear();
    parameters["iconFilename"]="zoomIn";
    parameters["command"]="zoom(1.125)";
    addWidgetToPage("Default",WidgetTypeButton,"Zoom In",               87.5, 23.0,0,93.0, 87.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="zoomOut";
    parameters["command"]="zoom(0.875)";
    addWidgetToPage("Default",WidgetTypeButton,"Zoom Out",              62.5, 23.0,0,93.0, 62.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="rotateLeft";
    parameters["command"]="rotate(+3)";
    addWidgetToPage("Default",WidgetTypeButton,"Rotate Left",           37.5, 23.0,0,93.0, 37.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="rotateRight";
    parameters["command"]="rotate(-3)";
    addWidgetToPage("Default",WidgetTypeButton,"Rotate Right",          12.5, 23.0,0,93.0, 12.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["uncheckedIconFilename"]="trackRecordingOff";
    parameters["uncheckedCommand"]="setRecordTrack(0)";
    parameters["checkedIconFilename"]="trackRecordingOn";
    parameters["checkedCommand"]="setRecordTrack(1)";
    parameters["stateConfigPath"]="Navigation";
    parameters["stateConfigName"]="recordTrack";
    addWidgetToPage("Default",WidgetTypeCheckbox,"Track Recording",     87.5, 93.0,0,7.0, 87.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="createNewTrack";
    parameters["command"]="createNewTrack()";
    addWidgetToPage("Default",WidgetTypeButton,"Create New Track",      62.5, 93.0,0,7.0, 62.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["uncheckedIconFilename"]="wakeLockOff";
    parameters["uncheckedCommand"]="setWakeLock(0)";
    parameters["checkedIconFilename"]="wakeLockOn";
    parameters["checkedCommand"]="setWakeLock(1)";
    parameters["stateConfigPath"]="Screen";
    parameters["stateConfigName"]="wakeLock";
    addWidgetToPage("Default",WidgetTypeCheckbox,"Wake Lock",           37.5, 93.0,0,7.0, 37.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["uncheckedIconFilename"]="returnToLocationOff";
    parameters["uncheckedCommand"]="setReturnToLocation(0)";
    parameters["checkedIconFilename"]="returnToLocationOn";
    parameters["checkedCommand"]="setReturnToLocation(1)";
    parameters["stateConfigPath"]="Map";
    parameters["stateConfigName"]="returnToLocation";
    addWidgetToPage("Default",WidgetTypeCheckbox,"Return To Location",  12.5, 93.0,0,7.0, 12.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["uncheckedIconFilename"]="zoomLevelLockOff";
    parameters["uncheckedCommand"]="setZoomLevelLock(0)";
    parameters["checkedIconFilename"]="zoomLevelLockOn";
    parameters["checkedCommand"]="setZoomLevelLock(1)";
    parameters["stateConfigPath"]="Map";
    parameters["stateConfigName"]="zoomLevelLock";
    addWidgetToPage("Default",WidgetTypeCheckbox,"Zoom Level Lock",     87.5, 81.0,0,75.0,87.5,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="meterBackground";
    parameters["meterType"]="altitude";
    parameters["updateInterval"]="1000000";
    parameters["labelY"]="78.0";
    parameters["valueY"]="40.0";
    parameters["unitY"]="10.0";
    addWidgetToPage("Default",WidgetTypeMeter,"Current altitude",       17.0, 9.0,0,25.0, 14.0,0,255,255,255,255,255,255,255,100,parameters);
    parameters["meterType"]="speed";
    addWidgetToPage("Default",WidgetTypeMeter,"Current speed",          50.0, 9.0,0,50.0, 14.0,0,255,255,255,255,255,255,255,100,parameters);
    parameters["meterType"]="trackLength";
    addWidgetToPage("Default",WidgetTypeMeter,"Track length",           83.0, 9.0,0,75.0, 14.0,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="scale";
    parameters["updateInterval"]="1000000";
    parameters["tickLabelOffsetX"]="-3.5";
    parameters["mapLabelOffsetY"]="9.0";
    addWidgetToPage("Default",WidgetTypeScale,"Map scale",              35.0, 81.0,0,37.0, 88.0,0,255,255,255,255,255,255,255,100,parameters);
    parameters.clear();
    parameters["iconFilename"]="statusBackground";
    parameters["updateInterval"]="100000";
    parameters["labelWidth"]="230";
    addWidgetToPage("Default",WidgetTypeStatus,"Status",                50.0, 33.0,0,50.0, 35.0,0,255,255,255,255,255,255,255,100,parameters);
    pageNames=c->getAttributeValues("Widget/Page","name");
  }

  // Create the widgets from the config
  pageNames=c->getAttributeValues("Widget/Page","name");
  std::list<std::string>::iterator i;
  for(i=pageNames.begin();i!=pageNames.end();i++) {
    DEBUG("found a widget page with name %s",(*i).c_str());

    // Create the page
    WidgetPage *page=new WidgetPage(*i);
    if (!page) {
      FATAL("can not create widget page object",NULL);
      return;
    }
    WidgetPagePair pair=WidgetPagePair(*i,page);
    pageMap.insert(pair);

    // Go through all widgets of this page
    std::string path="Widget/Page[@name='" + *i + "']/Primitive";
    std::list<std::string> widgetNames=c->getAttributeValues(path,"name");
    std::list<std::string>::iterator j;
    for(j=widgetNames.begin();j!=widgetNames.end();j++) {

      // Create the type-specific widget
      std::string widgetPath=path + "[@name='" + *j + "']";
      std::string widgetType=c->getStringValue(widgetPath,"type");
      WidgetPrimitive *primitive;
      WidgetButton *button;
      WidgetCheckbox *checkbox;
      WidgetMeter *meter;
      WidgetScale *scale;
      WidgetStatus *status;
      if (widgetType=="button") {
        button=new WidgetButton();
        primitive=button;
      }
      if (widgetType=="checkbox") {
        checkbox=new WidgetCheckbox();
        primitive=checkbox;
      }
      if (widgetType=="meter") {
        meter=new WidgetMeter();
        primitive=meter;
      }
      if (widgetType=="scale") {
        scale=new WidgetScale();
        primitive=scale;
      }
      if (widgetType=="status") {
        status=new WidgetStatus();
        primitive=status;
      }

      // Set type-independent properties
      std::list<std::string> name;
      name.push_back(*j);
      primitive->setName(name);
      primitive->setActiveColor(c->getGraphicColorValue(widgetPath + "/ActiveColor"));
      primitive->setInactiveColor(c->getGraphicColorValue(widgetPath + "/InactiveColor"));
      primitive->setColor(primitive->getInactiveColor());

      // Load the image of the widget
      if ((widgetType=="button")||(widgetType=="meter")||(widgetType=="scale")||(widgetType=="status")) {
        primitive->setTextureFromIcon(c->getStringValue(widgetPath,"iconFilename"));
      }
      if (widgetType=="checkbox") {
        primitive->setTextureFromIcon(c->getStringValue(widgetPath,"checkedIconFilename"));
        checkbox->setCheckedTexture(primitive->getTexture());
        primitive->setTextureFromIcon(c->getStringValue(widgetPath,"uncheckedIconFilename"));
        checkbox->setUncheckedTexture(primitive->getTexture());
      }

      // Set type-dependent properties
      if (widgetType=="button") {
        button->setCommand(c->getStringValue(widgetPath,"command"));
      }
      if (widgetType=="checkbox") {
        checkbox->setConfigPath(widgetPath);
        checkbox->setUncheckedCommand(c->getStringValue(widgetPath,"uncheckedCommand"));
        checkbox->setCheckedCommand(c->getStringValue(widgetPath,"checkedCommand"));
        checkbox->setStateConfigPath(c->getStringValue(widgetPath,"stateConfigPath"));
        checkbox->setStateConfigName(c->getStringValue(widgetPath,"stateConfigName"));
      }
      if (widgetType=="meter") {
        std::string meterType=c->getStringValue(widgetPath,"meterType");
        if (meterType=="altitude") {
          meter->setMeterType(WidgetMeterTypeAltitude);
        } else if (meterType=="speed") {
          meter->setMeterType(WidgetMeterTypeSpeed);
        } else if (meterType=="trackLength") {
          meter->setMeterType(WidgetMeterTypeTrackLength);
        } else {
          FATAL("unknown meter type",NULL);
          return;
        }
        meter->setUpdateInterval(c->getIntValue(widgetPath,"updateInterval"));
        meter->setLabelY(c->getDoubleValue(widgetPath,"labelY")*meter->getIconHeight()/100.0);
        meter->setValueY(c->getDoubleValue(widgetPath,"valueY")*meter->getIconHeight()/100.0);
        meter->setUnitY(c->getDoubleValue(widgetPath,"unitY")*meter->getIconHeight()/100.0);
      }
      if (widgetType=="scale") {
        scale->setUpdateInterval(c->getIntValue(widgetPath,"updateInterval"));
        scale->setTickLabelOffsetX(c->getDoubleValue(widgetPath,"tickLabelOffsetX")*meter->getIconWidth()/100.0);
        scale->setMapLabelOffsetY(c->getDoubleValue(widgetPath,"mapLabelOffsetY")*meter->getIconHeight()/100.0);
      }
      if (widgetType=="status") {
        status->setUpdateInterval(c->getIntValue(widgetPath,"updateInterval"));
        status->setLabelWidth(c->getIntValue(widgetPath,"labelWidth"));
        GraphicColor c=status->getColor();
        c.setAlpha(0);
        status->setColor(c);
      }

      // Add the widget to the page
      page->addWidget(primitive);

    }

  }

  // Set the default page on the graphic engine
  WidgetPageMap::iterator j;
  j=pageMap.find("Default");
  if (j==pageMap.end()) {
    FATAL("default page does not exist",NULL);
    return;
  } else {
    currentPage=j->second;
    core->getGraphicEngine()->setWidgetPage(currentPage);
  }

  // Set the positions of the widgets
  updateWidgetPositions();
}

// Updates the positions of the widgets in dependence of the current screen dimension
void WidgetEngine::updateWidgetPositions() {

  // Find out the orientation for which we need to update the positioning
  std::string orientation="Unknown";
  switch(core->getScreen()->getOrientation()) {
    case graphicScreenOrientationLandscape: orientation="Landscape"; break;
    case graphicScreenOrientationProtrait: orientation="Portrait"; break;
  }
  DEBUG("orientation=%s",orientation.c_str());
  Int width=core->getScreen()->getWidth();
  Int height=core->getScreen()->getHeight();

  // Go through all pages
  TimestampInMicroseconds t=core->getClock()->getMicrosecondsSinceStart();
  ConfigStore *c=core->getConfigStore();
  WidgetPageMap::iterator i;
  for(i=pageMap.begin();i!=pageMap.end();i++) {
    WidgetPage *page=i->second;

    // Go through all widgets
    std::list<WidgetPrimitive*> primitives;
    GraphicPrimitiveMap *widgetMap=page->getGraphicObject()->getPrimitiveMap();
    GraphicPrimitiveMap::iterator j;
    for(j=widgetMap->begin();j!=widgetMap->end();j++) {
      WidgetPrimitive *primitive=(WidgetPrimitive*)j->second;

      // Update the position
      std::string path="Widget/Page[@name='" + page->getName() + "']/Primitive[@name='" + primitive->getName().front() + "']/" + orientation;
      primitive->setZ(c->getIntValue(path,"z"));
      primitive->setX(width*c->getDoubleValue(path,"x")/100.0-width/2-primitive->getIconWidth()/2);
      primitive->setY(height*c->getDoubleValue(path,"y")/100.0-height/2-primitive->getIconHeight()/2);
      primitives.push_back(primitive);

    }

    // Re-add the widget to the graphic object to get them sorted
    page->deinit(false);
    for(std::list<WidgetPrimitive*>::iterator i=primitives.begin();i!=primitives.end();i++) {
      page->addWidget(*i);
    }

  }
}

// Clears all widget pages
void WidgetEngine::deinit() {

  // Delete all pages
  WidgetPageMap::iterator i;
  for(i = pageMap.begin(); i!=pageMap.end(); i++) {
    delete i->second;
  }
  pageMap.clear();

}

// Called when the screen is touched
bool WidgetEngine::onTouchDown(TimestampInMicroseconds t, Int x, Int y) {
  return currentPage->onTouchDown(t,x,y);
}

// Called when the screen is untouched
bool WidgetEngine::onTouchUp(TimestampInMicroseconds t, Int x, Int y) {
  return currentPage->onTouchUp(t,x,y);
}

}
