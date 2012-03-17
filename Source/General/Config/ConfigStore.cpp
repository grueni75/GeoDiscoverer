//============================================================================
// Name        : ConfigStore.cpp
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
ConfigStore::ConfigStore() {
  configFilepath=core->getHomePath() + "/config.xml";
  configValueWidth=40;
  document=NULL;
  xpathCtx=NULL;
  mutex=core->getThread()->createMutex();
  init();
  read();
}

// Destructor
ConfigStore::~ConfigStore() {
  deinit();
  core->getThread()->destroyMutex(mutex);
}

// Sets an integer value in the config
void ConfigStore::setIntValue(std::string path, std::string name, Int value, std::string description)
{
  std::stringstream out;
  out << value;
  std::string valueString = out.str();
  setStringValue(path,name,valueString,description);
}

// Sets an integer value in the config
void ConfigStore::setDoubleValue(std::string path, std::string name, double value, std::string description)
{
  std::stringstream out;
  out << value;
  std::string valueString = out.str();
  setStringValue(path,name,valueString,description);
}

// Gets a integer value from the config
Int ConfigStore::getIntValue(std::string path, std::string name, std::string description, Int defaultValue)
{
  std::string value;
  std::string defaultValueString;
  std::stringstream out;
  Int valueInt;
  out << defaultValue;
  defaultValueString = out.str();
  value=getStringValue(path,name,description,defaultValueString);
  std::istringstream in(value);
  in >> valueInt;
  return valueInt;

}

// Gets a unsigned integer value from the config
UInt ConfigStore::getUIntValue(std::string path, std::string name, std::string description, UInt defaultValue)
{
  std::string value;
  std::string defaultValueString;
  std::stringstream out;
  UInt valueUInt;
  out << defaultValue;
  defaultValueString = out.str();
  value=getStringValue(path,name,description,defaultValueString);
  std::istringstream in(value);
  in >> valueUInt;
  return valueUInt;

}

// Gets a double value from the config
double ConfigStore::getDoubleValue(std::string path, std::string name, std::string description, double defaultValue)
{
  std::string value;
  std::string defaultValueString;
  std::stringstream out;
  double valueDouble;
  out << defaultValue;
  defaultValueString = out.str();
  value=getStringValue(path,name,description,defaultValueString);
  std::istringstream in(value);
  in >> valueDouble;
  return valueDouble;

}

// Gets a color value from the config
GraphicColor ConfigStore::getGraphicColorValue(std::string path, std::string description, GraphicColor defaultValue) {
  UByte red=core->getConfigStore()->getIntValue(path,"red","Red component [0-255] of the " + description,defaultValue.getRed());
  UByte green=core->getConfigStore()->getIntValue(path,"green","Green component [0-255] of the " + description,defaultValue.getGreen());
  UByte blue=core->getConfigStore()->getIntValue(path,"blue","Blue component [0-255] of the " + description,defaultValue.getBlue());
  UByte alpha=core->getConfigStore()->getIntValue(path,"alpha","Alpha component [0-255] of the " + description,defaultValue.getAlpha());
  return GraphicColor(red,green,blue,alpha);
}

// Test if a path exists
bool ConfigStore::pathExists(std::string path) {
  std::list<XMLNode> nodes=findNodes("/GDC/" + path);
  if (nodes.size()>0) {
    return true;
  } else {
    return false;
  }
}

}
