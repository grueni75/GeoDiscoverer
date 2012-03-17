//============================================================================
// Name        : ConfigStore.h
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

#ifndef CONFIGSTORE_H_
#define CONFIGSTORE_H_

#include <ConfigStorePlatform.h>

namespace GEODISCOVERER {

class ConfigStore {

protected:

  // Name of the config file
  std::string configFilepath;

  // Width of the value field in config file
  Int configValueWidth;

  // Pointer to the data
  XMLDocument document;

  // Context for using xpath
  XMLXPathContext xpathCtx;

  // Mutex to sequentialize changes in the config
  ThreadMutexInfo *mutex;


  // Inits the data
  void init();

  // Deinits the data
  void deinit();

  // Writes the config to disk
  void write();

  // Reads the config
  void read();

  // Creates a node inclusive its path
  XMLNode createNodeWithPath(XMLNode parentNode, std::string path, std::string name, std::string description, std::string value);

  // Finds a set of nodes at the given path
  std::list<XMLNode> findNodes(std::string path);

public:

  // Constructor
  ConfigStore();

  // Destructor
  virtual ~ConfigStore();

  // Test if a path exists
  bool pathExists(std::string path);

  // Access methods to config values
  void setStringValue(std::string path, std::string name, std::string value, std::string description="");
  void setIntValue(std::string path, std::string name, Int value, std::string description="");
  void setDoubleValue(std::string path, std::string name, double value, std::string description="");
  std::string getStringValue(std::string path, std::string name, std::string description="", std::string defaultValue="");
  Int getIntValue(std::string path, std::string name, std::string description="", Int defaultValue=0);
  UInt getUIntValue(std::string path, std::string name, std::string desription="", UInt defaultValue=0);
  double getDoubleValue(std::string path, std::string name, std::string description="", double defaultValue=0.0);
  GraphicColor getGraphicColorValue(std::string path, std::string description="", GraphicColor defaultValue=GraphicColor());

  // Returns a list of attribute values for a given path and attribute name
  std::list<std::string> getAttributeValues(std::string path, std::string attributeName);

};

}

#endif /* CONFIGSTORE_H_ */
