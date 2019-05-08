//============================================================================
// Name        : ConfigSection.cpp
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

// Sets a default namespace for all the nodes
void ConfigSection::setDefaultNamespace(XMLNamespace ns, XMLNode node) {
  if (node->ns==NULL) {
    xmlSetNs(node,ns);
  }
  for (XMLNode n=node->children;n!=NULL;n=n->next) {
    setDefaultNamespace(ns,n);
  }
}

// Reads the config
bool ConfigSection::readConfig(std::string configFilepath) {

  // Remember the folder of this config
  char *t = strdup(configFilepath.c_str());
  folder=std::string(dirname(t));
  free(t);

  // Merge the info if config is already set
  if (config) {

    // Read the doc
    XMLDocument doc =  xmlReadFile(configFilepath.c_str(), NULL, 0);
    if (!doc) {
      ERROR("read of config file <%s> failed",configFilepath.c_str());
      return false;
    }

    // Set the default namespace (if doc has none)
    XMLNode currentSibling=xmlGetLastChild(findConfigNodes("/GDS").front());

    // Copy all nodes below the root
    XMLNamespace ns=currentSibling->ns;
    for (XMLNode n=xmlDocGetRootElement(doc)->children;n!=NULL;n=n->next) {
      XMLNode n2=xmlCopyNode(n,true);
      setDefaultNamespace(ns,n2);
      xmlAddNextSibling(currentSibling,n2);
      currentSibling=n2;
    }

    // Free memory
    xmlFreeDoc(doc);

  } else {
    config = xmlReadFile(configFilepath.c_str(), NULL, 0);
  }
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
  this->schemaNamespace=schemaNamespace;
  if (!xpathConfigCtx) {
    XMLNode root=xmlDocGetRootElement(config);
    if (!root->ns) {
      XMLNamespace ns=xmlNewNs(root,BAD_CAST schemaNamespace.c_str(),NULL);
      setDefaultNamespace(ns,root);
    }
    xpathConfigCtx = xmlXPathNewContext(config);
    if (xpathConfigCtx == NULL) {
      FATAL("can not create config xpath context",NULL);
      return;
    }
    xmlXPathRegisterNs(xpathConfigCtx, BAD_CAST "gd", BAD_CAST schemaNamespace.c_str());
  }
  if (!xpathSchemaCtx) {
    xpathSchemaCtx = xmlXPathNewContext(schema);
    if (xpathSchemaCtx == NULL) {
      FATAL("can not create schema xpath context",NULL);
      return;
    }
    xmlXPathRegisterNs(xpathSchemaCtx, BAD_CAST "xsd", BAD_CAST "http://www.w3.org/2001/XMLSchema");
  }
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
  bool predicateActive=false;
  bool charConsumed;
  std::string newPath="";
  for (j=0;j<path.length();j++) {
    charConsumed=false;
    switch (path[j]) {
    case '[':
      predicateActive=true;
      break;
    case ']':
      predicateActive=false;
      break;
    case '/':
      if (!predicateActive) {
        newPath+="/gd:";
        charConsumed=true;
      }
      break;
    }
    if (!charConsumed)
      newPath+=path[j];
  }
  path=newPath;
  //DEBUG("path=%s",path.c_str());

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

// Sets the text contents of the given node
bool ConfigSection::setNodeText(XMLNode node, std::string nodeText) {
  xmlNodePtr n;
  for (n = node->children; n; n = n->next) {
    if ((n->name)&&(strcmp((char*)n->name,"text")==0)) {
      xmlNodeSetContent(n,(const xmlChar *)nodeText.c_str());
      return true;
    }
  }
  return false;
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

// Finds a child node of the given parent node
XMLNode ConfigSection::findNode(XMLNode parent, std::string nodeName) {
  for (XMLNode n=parent->children;n!=NULL;n=n->next) {
    if ((n->type==XML_ELEMENT_NODE)&&(strcmp((char*)n->name,nodeName.c_str())==0)) {
      return n;
    }
  }
  return NULL;
}

// Returns the contents of a list of nodes as a list of string
bool ConfigSection::getAllNodesText(XMLNode parent, std::string nodeName, std::list<std::string> &nodeTexts) {
  bool result=false;
  for (XMLNode n=parent->children;n!=NULL;n=n->next) {
    if ((n->type==XML_ELEMENT_NODE)&&(strcmp((char*)n->name,nodeName.c_str())==0)) {
      std::string nodeText;
      getNodeText(n,nodeText);
      nodeTexts.push_back(nodeText);
      result=true;
    }
  }
  return result;
}

} /* namespace GEODISCOVERER */
