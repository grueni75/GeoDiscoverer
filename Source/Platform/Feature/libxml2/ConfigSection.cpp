//============================================================================
// Name        : ConfigSection.cpp
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
ConfigSection::ConfigSection() {
  config=NULL;
  schema=NULL;
  xpathSchemaCtx=NULL;
  xpathConfigCtx=NULL;
}

// Deinits the data
void ConfigSection::deinit()
{
  if (xpathSchemaCtx)
    xmlXPathFreeContext(xpathSchemaCtx);
  if (xpathConfigCtx)
    xmlXPathFreeContext(xpathConfigCtx);
  if (schema)
    xmlFreeDoc((xmlDocPtr)schema);
  if (config)
    xmlFreeDoc((xmlDocPtr)config);
  xpathConfigCtx=NULL;
  xpathSchemaCtx=NULL;
  config=NULL;
  schema=NULL;
}

// Reads the schema
bool ConfigSection::readSchema(std::string schemaFilepath) {
  schema = xmlReadFile(schemaFilepath.c_str(), NULL, 0);
  if (!schema) {
    FATAL("read of schema file <%s> failed",schemaFilepath.c_str());
    return false;
  }
  return true;
}

// Reads the config
bool ConfigSection::readConfig(std::string configFilepath) {
  config = xmlReadFile(configFilepath.c_str(), NULL, 0);
  if (!config) {
    ERROR("read of config file <%s> failed",configFilepath.c_str());
    return false;
  }
  return true;
}

// Destructor
ConfigSection::~ConfigSection() {
  deinit();
}

// Prepare xpath evaluation
void ConfigSection::prepareXPath(std::string schemaNamespace) {
  xpathConfigCtx = xmlXPathNewContext(config);
  if (xpathConfigCtx == NULL) {
    FATAL("can not create config xpath context",NULL);
    return;
  }
  xmlXPathRegisterNs(xpathConfigCtx, BAD_CAST "gd", BAD_CAST schemaNamespace.c_str());
  xpathSchemaCtx = xmlXPathNewContext(schema);
  if (xpathSchemaCtx == NULL) {
    FATAL("can not create schema xpath context",NULL);
    return;
  }
  xmlXPathRegisterNs(xpathSchemaCtx, BAD_CAST "xsd", BAD_CAST "http://www.w3.org/2001/XMLSchema");
}

// Finds config nodes
std::list<XMLNode> ConfigSection::findConfigNodes(std::string path) {
  std::list<XMLNode> result;
  xmlXPathObjectPtr xpathObj=NULL;
  xmlNodePtr node=NULL;
  xmlNodeSetPtr nodes;
  Int size;
  bool found;
  size_t i,j;

  // Add the correct namespace to the path
  i=0;
  while((j=path.find("/",i))!=std::string::npos) {
    path=path.replace(j,1,"/gd:");
    i=j+1;
  }

  // Execute the xpath expression
  xpathObj = xmlXPathEvalExpression(BAD_CAST path.c_str(), xpathConfigCtx);
  if (xpathObj == NULL) {
    FATAL("can not evaluate xpath expression <%s>",path.c_str());
    return result;
  }
  nodes=xpathObj->nodesetval;
  size = (nodes) ? nodes->nodeNr : 0;
  found = false;
  for(Int i = 0; i < size; ++i) {
    if(nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
      result.push_back(nodes->nodeTab[i]);
    }
  }
  xmlXPathFreeObject(xpathObj);
  return result;
}

// Finds schema nodes
std::list<XMLNode> ConfigSection::findSchemaNodes(std::string path, std::string extension) {
  std::list<XMLNode> result;
  xmlXPathObjectPtr xpathObj=NULL;
  xmlNodePtr node=NULL;
  xmlNodeSetPtr nodes;
  Int size;
  bool found;
  size_t i,j;

  // Remove any attribute constraints from the path
  i=0;
  while((j=path.find("[",i))!=std::string::npos) {
    Int k=path.find("]",j+1);
    if (k==std::string::npos) {
      k=path.size();
    }
    std::string constraint=path.substr(j,k-j+1);
    path=path.replace(j,constraint.size(),"");
    i=j+1;
  }

  // Modify the config path to match the schema pathes
  i=0;
  while((j=path.find("/",i))!=std::string::npos) {
    Int k=path.find("/",j+1);
    if (k==std::string::npos) {
      k=path.size();
    }
    std::string elementName=path.substr(j+1,k-j-1);
    std::string replaceString;
    if (i==0) {
      replaceString="/xsd:schema/xsd:element[@name='" + elementName + "']";
    } else {
      replaceString="/xsd:complexType/xsd:sequence/xsd:element[@name='" + elementName + "']";
    }
    path=path.replace(j,elementName.size()+1,replaceString);
    i=j+replaceString.size();
  }
  path=path+extension;

  // Execute the xpath expression
  xpathObj = xmlXPathEvalExpression(BAD_CAST path.c_str(), xpathSchemaCtx);
  if (xpathObj == NULL) {
    FATAL("can not evaluate xpath expression <%s>",path.c_str());
    return result;
  }
  nodes=xpathObj->nodesetval;
  size = (nodes) ? nodes->nodeNr : 0;
  found = false;
  for(Int i = 0; i < size; ++i) {
    if (nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
      result.push_back(nodes->nodeTab[i]);
    }
  }
  xmlXPathFreeObject(xpathObj);
  return result;
}

// Returns the text contents as string of the given node
bool ConfigSection::getNodeText(XMLNode node, std::string &nodeText) {
  xmlNodePtr n;
  for (n = node->children; n; n = n->next) {
    if ((n->name)&&(strcmp((char*)n->name,"text")==0)
    &&(n->content)&&(strcmp((char*)n->content,"")!=0)) {
      nodeText=std::string((char*)node->children->content);
      return true;
    }
  }
  return false;
}

// Returns the text contents as string of a given element in the tree
bool ConfigSection::getNodeText(XMLNode parent, std::string nodeName, std::string &nodeText) {
  for (XMLNode n=parent->children;n!=NULL;n=n->next) {
    if ((n->type==XML_ELEMENT_NODE)&&(strcmp((char*)n->name,nodeName.c_str())==0)) {
      getNodeText(n,nodeText);
      return true;
    }
  }
  return false;
}

// Returns the text contents as double of a given element in the tree
bool ConfigSection::getNodeText(XMLNode parent, std::string nodeName, double &nodeText) {
  std::string t;
  if (getNodeText(parent,nodeName,t)) {
    std::stringstream s(t);
    s >> nodeText;
    return true;
  } else {
    return false;
  }
}

} /* namespace GEODISCOVERER */
