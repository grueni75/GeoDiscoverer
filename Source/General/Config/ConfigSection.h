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