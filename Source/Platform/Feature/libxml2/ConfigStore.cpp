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

// Inits the data
void ConfigStore::init()
{
  xmlInitParser();
}

// Deinits the data
void ConfigStore::deinit()
{
  if (xpathCtx)
    xmlXPathFreeContext(xpathCtx);
  if (document)
    xmlFreeDoc((xmlDocPtr)document);
  xpathCtx=NULL;
  document=NULL;
  xmlCleanupParser();
}

// Reads the config
void ConfigStore::read()
{
  xmlDocPtr doc = NULL;
  xmlNodePtr rootNode = NULL, node = NULL, node1 = NULL;
  xmlDtdPtr dtd = NULL;
  char buff[256];
  int i, j;

  // Check if the file exists
  if (access(configFilepath.c_str(),F_OK)) {

    // No, so create empty XML document
    doc = xmlNewDoc(BAD_CAST "1.0");
    if (!doc) {
      FATAL("can not create xml document",NULL);
      return;
    }
    rootNode = xmlNewNode(NULL, BAD_CAST "GDC");
    if (!rootNode) {
      FATAL("can not create xml root node",NULL);
      return;
    }
    if (!xmlNewProp(rootNode, BAD_CAST "version", BAD_CAST "1.0")) {
      FATAL("can not create xml property",NULL);
      return;
    }
    xmlDocSetRootElement(doc, rootNode);

    // Save the XML document
    document=doc;
    write();

    // Cleanup
    xmlFreeDoc(doc);
    document=NULL;

  }

  // Read the document
  doc = xmlReadFile(configFilepath.c_str(), NULL, 0);
  if (!doc) {
    FATAL("read of config file <%s> failed",configFilepath.c_str());
    return;
  }
  rootNode = xmlDocGetRootElement(doc);
  if (!rootNode) {
    FATAL("could not extract root node",NULL);
    return;
  }

  // Version handling
  std::string version=(char *)xmlGetProp(rootNode,BAD_CAST "version");
  if (version!="1.0") {
    FATAL("config file <%s> has unknown version",configFilepath.c_str());
    return;
  }

  // Remember the document
  document=doc;

  // Prepare xpath evaluation
  xpathCtx = xmlXPathNewContext(doc);
  if (xpathCtx == NULL) {
    FATAL("can not create xpath context",NULL);
    return;
  }
}

// Writes the config
void ConfigStore::write()
{
  xmlDocPtr doc = (xmlDocPtr) document;
  if (!doc) {
    FATAL("config object has no xml document",NULL);
    return;
  }
  std::string tempFilepath = configFilepath + "+";
  if (xmlSaveFormatFileEnc(tempFilepath.c_str(), doc, "UTF-8", 1)==-1) {
    FATAL("can not write configuration file <%s>",configFilepath.c_str());
    return;
  }
  rename(tempFilepath.c_str(),configFilepath.c_str());
}

// Crates a node inclusive its path
XMLNode ConfigStore::createNodeWithPath(XMLNode parentNode, std::string path, std::string name, std::string description, std::string value)
{
  xmlNodePtr childNode;

  // Have we reached the last node in the path
  if (path=="") {

    // Create the text element node
    childNode=xmlNewChild(parentNode, NULL, BAD_CAST name.c_str(), BAD_CAST value.c_str());
    if (!childNode) {
      FATAL("can not create node <%s>",name.c_str());
      return NULL;
    }
    if (!(xmlNewProp(childNode,BAD_CAST "description", BAD_CAST description.c_str()))) {
      FATAL("can not add property to node <%s>",name.c_str());
      return NULL;
    }
    return childNode;

  } else {

    // Extract the name of the first element in the path and the remaining path
    Int pos=path.find_first_of('/');
    std::string firstPathElement;
    std::string remainingPath;
    if (pos!=std::string::npos) {
      firstPathElement=path.substr(0,pos);
      remainingPath=path.substr(pos+1);
    } else {
      firstPathElement=path;
      remainingPath="";
    }

    // Extract the attribute name if given
    std::string attributeName="";
    std::string attributeValue;
    pos=firstPathElement.find_first_of("[@");
    if (pos!=std::string::npos) {
      std::string t=firstPathElement.substr(pos+2);
      //DEBUG("t=%s",t.c_str());
      firstPathElement=firstPathElement.substr(0,pos);
      //DEBUG("firstPathElement=%s",firstPathElement.c_str());
      t=t.substr(0,t.find_first_of("]"));
      //DEBUG("t=%s",t.c_str());
      pos=t.find_first_of("='");
      attributeName=t.substr(0,pos);
      attributeValue=t.substr(pos+2);
      attributeValue=attributeValue.substr(0,attributeValue.find_first_of("'"));
      //DEBUG("attributeName=%s attributeValue=%s",attributeName.c_str(),attributeValue.c_str());
    }

    // Search for the first element in the current node
    xmlNodePtr n;
    bool found=false;
    for (n = parentNode->children; n; n = n->next) {
      if (n->type == XML_ELEMENT_NODE) {
        if (firstPathElement == (char*)n->name) {

          // If an attribute is given, check that this node matches it
          if (attributeName=="")  {
            found=true;
          } else {
            xmlAttr *n2;
            for (n2 = n->properties; n2; n2 = n2->next) {
              if ((n2->type==XML_ATTRIBUTE_NODE)&&(attributeName==(char*)n2->name)) {
                if (attributeValue==(char*)n2->children->content) {
                  found=true;
                  break;
                }
              }
            }
          }
          if (found) {
            childNode=n;
            break;
          }

        }
      }
    }

    // Was the element not found?
    if (!found) {

      // Add the new node
      childNode=xmlNewChild(parentNode, NULL, BAD_CAST firstPathElement.c_str(), NULL);
      if (!childNode) {
        FATAL("can not create node <%s>",firstPathElement.c_str());
        return NULL;
      }

      // Add the attribute if given
      if (attributeName!="") {
        if (!xmlNewProp(childNode, BAD_CAST attributeName.c_str(), BAD_CAST attributeValue.c_str())) {
          FATAL("can not create attribute <%s> of node <%s>",attributeName.c_str(),firstPathElement.c_str());
          return NULL;
        }
      }

    }

    // Continue recursively with the remaining path
    return createNodeWithPath(childNode,remainingPath,name,description,value);

  }
}

// Finds nodes
std::list<XMLNode> ConfigStore::findNodes(std::string path) {
  std::list<XMLNode> result;
  xmlXPathObjectPtr xpathObj=NULL;
  xmlNodePtr node=NULL;
  xmlNodeSetPtr nodes;
  Int size;
  bool found;
  xpathObj = xmlXPathEvalExpression(BAD_CAST path.c_str(), xpathCtx);
  if (xpathObj == NULL) {
    xmlXPathFreeContext(xpathCtx);
    FATAL("can not evaluate xpath expression",NULL);
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

// Gets a string value from the config
std::string ConfigStore::getStringValue(std::string path, std::string name, std::string description, std::string defaultValue)
{
  xmlDocPtr doc = document;
  std::string value;
  xmlNodePtr node,rootNode;
  std::string xpath="/GDC/" + path + "/" + name;

  // Check if node exists
  std::list<XMLNode> nodes;
  nodes=findNodes(xpath);
  if (nodes.size() == 0) {

    // No, so create node
    setStringValue(path,name,defaultValue,description);

    // Find it again
    nodes=findNodes(xpath);
    if (nodes.size()==0) {
      FATAL("could not find new node",NULL);
      return "";
    }

  }

  // Sanity check
  if (nodes.size()!=1) {
    FATAL("more than one node found (path=%s)",path.c_str());
    return "";
  }
  node=nodes.front();

  // Return result
  if ((!node->children)||(std::string((char*)node->children->name)!="text")) {
    FATAL("node has no text child",NULL);
    return "";
  }
  value=std::string((char*)node->children->content);
  return value;
}

// Sets a string value in the config
void ConfigStore::setStringValue(std::string path, std::string name, std::string value, std::string description)
{
  xmlDocPtr doc = document;
  xmlNodePtr node,rootNode;
  std::string xpath="/GDC/" + path + "/" + name;

  // Only one thread may enter setStringValue
  core->getThread()->lockMutex(mutex);

  // Check if node exists
  std::list<XMLNode> nodes;
  nodes=findNodes(xpath);
  if (nodes.size() == 0) {

    // No, so create node
    rootNode = xmlDocGetRootElement(doc);
    if (!rootNode) {
      FATAL("could not extract root node",NULL);
      return;
    }
    createNodeWithPath(rootNode,path,name,description,value);

  } else {

    // Sanity check
    if (nodes.size()!=1) {
      FATAL("more than one node found",NULL);
      return;
    }
    node=nodes.front();

    // Update node
    if ((!node->children)||(std::string((char*)node->children->name)!="text")) {
      FATAL("node has no text child",NULL);
      return;
    }
    xmlNodeSetContent(node->children,xmlEncodeSpecialChars(doc,(const xmlChar *)value.c_str()));
  }
  write();
  core->getThread()->unlockMutex(mutex);
}

// Returns a list of attribute values for a given path and attribute name
std::list<std::string> ConfigStore::getAttributeValues(std::string path, std::string attributeName) {

  std::list<XMLNode> nodes;
  std::list<std::string> values;
  xmlNodePtr n;

  nodes=findNodes("/GDC/" + path);
  std::list<XMLNode>::iterator i;
  for(i=nodes.begin();i!=nodes.end();i++) {
    n=*i;
    xmlAttr *p;
    for (p = n->properties; p; p = p->next) {
      if ((p->type==XML_ATTRIBUTE_NODE)&&(attributeName==(char*)p->name)) {
        values.push_back(std::string((char*)p->children->content));
      }
    }
  }
  return values;
}

}
