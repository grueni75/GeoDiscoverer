//============================================================================
// Name        : ConfigSection.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#ifndef CONFIGSECTION_H_
#define CONFIGSECTION_H_

#include <ConfigStorePlatform.h>

namespace GEODISCOVERER {

class ConfigSection {

protected:

  // Pointer to the schema
  XMLDocument schema;

  // Pointer to the data
  XMLDocument config;

  // Context for using xpath
  XMLXPathContext xpathConfigCtx;
  XMLXPathContext xpathSchemaCtx;

public:

  // Constructor
  ConfigSection();

  // Destructor
  virtual ~ConfigSection();

  // Deinits the data
  void deinit();

  // Reads the schema
  bool readSchema(std::string schemaFilepath);

  // Reads the config
  bool readConfig(std::string configFilepath);

  // Prepare xpath evaluation
  void prepareXPath(std::string schemaNamespace);

  // Finds a set of config nodes at the given path
  std::list<XMLNode> findConfigNodes(std::string path);

  // Finds a set of schema nodes at the given path
  std::list<XMLNode> findSchemaNodes(std::string path, std::string extension="");

  // Getters and setters
  XMLDocument getConfig() const {
    return config;
  }

  void setConfig(XMLDocument config) {
    this->config = config;
  }

  XMLDocument getSchema() const {
    return schema;
  }
};

} /* namespace GEODISCOVERER */

#endif /* CONFIGSECTION_H_ */
