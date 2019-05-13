//============================================================================
// Name        : ConfigSection.h
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
  std::string schemaNamespace;
  XMLXPathContext xpathConfigCtx;
  XMLXPathContext xpathSchemaCtx;

  // Folder this config is loaded from
  std::string folder;

  // Sets a default namespace for all the nodes
  void setDefaultNamespace(XMLNamespace ns, XMLNode node);

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

  // Finds a child node of the given parent node
  static XMLNode findNode(XMLNode parent, std::string nodeName);

  // Sets the text contents of the given node
  bool setNodeText(XMLNode node, std::string nodeText);

  // Sets the text contents of the given node
  template <class T> bool setNodeText(XMLNode node, T nodeText) {
    std::stringstream s;
    s << nodeText;
    return setNodeText(node,s.str());
  }

  // Returns the text contents as string of the given node
  static bool getNodeText(XMLNode node, std::string &nodeText);

  // Returns the text contents as given type of the given node
  template <class T> static bool getNodeText(XMLNode node, T &nodeText) {
    std::string t;
    if (getNodeText(node,t)) {
      std::stringstream s(t);
      s >> nodeText;
      return true;
    } else {
      return false;
    }
  }

  // Returns the text contents as given type of a given element in the tree
  template <class T> static bool getNodeText(XMLNode parent, std::string nodeName, T &nodeText) {
    XMLNode n=findNode(parent,nodeName);
    if (n) {
      return getNodeText(n,nodeText);
    } else {
      return false;
    }
  }

  // Returns the text contents as given type for a given path
  template <class T> bool getNodeText(std::string path, T &nodeText) {
    std::list<XMLNode> n=findConfigNodes(path);
    if (n.size()==1) {
      return getNodeText(n.front(),nodeText);
    }
    return false;
  }

  // Sets the text contents as given type for a given path
  template <class T> bool setNodeText(std::string path, T nodeText) {
    std::list<XMLNode> n=findConfigNodes(path);
    if (n.size()==1) {
      return setNodeText(n.front(),nodeText);
    }
    return false;
  }

  // Sets the text contents as given type of a given element in the tree
  template <class T> bool setNodeText(XMLNode parent, std::string nodeName, T nodeText) {
    XMLNode n=findNode(parent,nodeName);
    if (!n) {
      n=xmlNewTextChild(parent,NULL,(const xmlChar *)nodeName.c_str(),(const xmlChar *)"");
    }
    return setNodeText(n,nodeText);
  }

  // Returns the contents of a list of nodes as a list of string
  bool getAllNodesText(XMLNode parent, std::string nodeName, std::list<std::string> &nodeTexts);

  // Changes a path such that it can be used in xpath expressions
  static std::string makeXPathCompatible(std::string path);

  // Unescapes all special characters in a string
  static std::string unescapeChars(std::string value);

  // Escapes all special characters in a string
  static std::string escapeChars(std::string value);

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

  const std::string& getFolder() const {
    return folder;
  }
};

} /* namespace GEODISCOVERER */

#endif /* CONFIGSECTION_H_ */
