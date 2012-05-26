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
  xmlKeepBlanksDefault(0);
}

// Deinits the data
void ConfigStore::deinit()
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

  // Only one thread may enter read
  core->getThread()->lockMutex(mutex);

  // Read the schema
  schema = xmlReadFile(schemaFilepath.c_str(), NULL, 0);
  if (!schema) {
    FATAL("read of schema file <%s> failed",schemaFilepath.c_str());
    core->getThread()->unlockMutex(mutex);
    return;
  }

  // Check if the file exists
  if (access(configFilepath.c_str(),F_OK)) {

    // No, so create empty XML config
    doc = xmlNewDoc(BAD_CAST "1.0");
    if (!doc) {
      FATAL("can not create xml document",NULL);
      core->getThread()->unlockMutex(mutex);
      return;
    }
    rootNode = xmlNewNode(NULL, BAD_CAST "GDC");
    if (!rootNode) {
      FATAL("can not create xml root node",NULL);
      core->getThread()->unlockMutex(mutex);
      return;
    }
    if (!xmlNewProp(rootNode, BAD_CAST "version", BAD_CAST "1.0")) {
      FATAL("can not create xml property",NULL);
      core->getThread()->unlockMutex(mutex);
      return;
    }
    xmlDocSetRootElement(doc, rootNode);

    // Create the namespaces
    xmlNsPtr configNamespace=xmlNewNs(rootNode,BAD_CAST "http://www.untouchableapps.de/GeoDiscoverer/config/1/0", BAD_CAST NULL);
    if (!configNamespace) {
     FATAL("can not create config namespace",NULL);
     core->getThread()->unlockMutex(mutex);
     return;
    }
    xmlNsPtr xmlNamespace=xmlNewNs(rootNode,BAD_CAST "http://www.w3.org/2001/XMLSchema-instance", BAD_CAST "xsi");
    if (!xmlNamespace) {
     FATAL("can not create xml namespace",NULL);
     core->getThread()->unlockMutex(mutex);
     return;
    }
    if (!xmlNewNsProp(rootNode, xmlNamespace, BAD_CAST "schemaLocation", BAD_CAST "http://www.untouchableapps.de/GeoDiscoverer/config/1/0 http://www.untouchableapps.de/GeoDiscoverer/config/1/0/config.xsd")) {
      FATAL("can not create xml property",NULL);
      core->getThread()->unlockMutex(mutex);
      return;
    }

    // Save the XML config
    config=doc;
    write();

    // Cleanup
    xmlFreeDoc(doc);
    config=NULL;

  }

  // Read the config
  doc = xmlReadFile(configFilepath.c_str(), NULL, 0);
  if (!doc) {
    FATAL("read of config file <%s> failed",configFilepath.c_str());
    core->getThread()->unlockMutex(mutex);
    return;
  }
  rootNode = xmlDocGetRootElement(doc);
  if (!rootNode) {
    FATAL("could not extract root node",NULL);
    core->getThread()->unlockMutex(mutex);
    return;
  }

  // Version handling
  xmlChar *text = xmlGetProp(rootNode,BAD_CAST "version");
  if (strcmp((char*)text,"1.0")!=0) {
    FATAL("config file <%s> has unknown version",configFilepath.c_str());
    core->getThread()->unlockMutex(mutex);
    return;
  }
  xmlFree(text);

  // Remember the config
  config=doc;

  // Prepare xpath evaluation
  xpathConfigCtx = xmlXPathNewContext(doc);
  if (xpathConfigCtx == NULL) {
    FATAL("can not create config xpath context",NULL);
    core->getThread()->unlockMutex(mutex);
    return;
  }
  xmlXPathRegisterNs(xpathConfigCtx, BAD_CAST "gdc", BAD_CAST "http://www.untouchableapps.de/GeoDiscoverer/config/1/0");
  xpathSchemaCtx = xmlXPathNewContext(schema);
  if (xpathSchemaCtx == NULL) {
    FATAL("can not create schema xpath context",NULL);
    core->getThread()->unlockMutex(mutex);
    return;
  }
  xmlXPathRegisterNs(xpathSchemaCtx, BAD_CAST "xsd", BAD_CAST "http://www.w3.org/2001/XMLSchema");

  // That's it
  core->getThread()->unlockMutex(mutex);
}

// Writes the config
void ConfigStore::write()
{
  xmlDocPtr doc = (xmlDocPtr) config;
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
XMLNode ConfigStore::createNodeWithPath(XMLNode parentNode, std::string path, std::string name, std::string value)
{
  xmlNodePtr childNode;

  // Have we reached the last node in the path?
  if (path=="") {

    // Create the text element node
    childNode=xmlNewChild(parentNode, NULL, BAD_CAST name.c_str(), BAD_CAST value.c_str());
    if (!childNode) {
      FATAL("can not create node <%s>",name.c_str());
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
    return createNodeWithPath(childNode,remainingPath,name,value);

  }
}

// Finds config nodes
std::list<XMLNode> ConfigStore::findConfigNodes(std::string path) {
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
    path=path.replace(j,1,"/gdc:");
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
std::list<XMLNode> ConfigStore::findSchemaNodes(std::string path, std::string extension) {
  std::list<XMLNode> result;
  xmlXPathObjectPtr xpathObj=NULL;
  xmlNodePtr node=NULL;
  xmlNodeSetPtr nodes;
  Int size;
  bool found;
  size_t i,j;

  // Remove any attribute constraints from the apth
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

// Gets a string value from the config
std::string ConfigStore::getStringValue(std::string path, std::string name)
{
  xmlDocPtr doc = config;
  std::string value;
  xmlNodePtr node,rootNode,childNode;
  std::string xpath;
  if (path=="")
    xpath="/GDC/" + name;
  else
    xpath="/GDC/" + path + "/" + name;

  // Only one thread may enter getStringValue
  core->getThread()->lockMutex(mutex);

  // Check if node exists
  std::list<XMLNode> configNodes,schemaNodes;
  configNodes=findConfigNodes(xpath);
  if (configNodes.size() == 0) {

    // Find the schema node
    schemaNodes=findSchemaNodes(xpath);
    if (schemaNodes.size()!=1) {
      FATAL("could not find path <%s/%s> in schema",path.c_str(),name.c_str());
      core->getThread()->unlockMutex(mutex);
      return "";
    }

    // Get the default value
    xmlAttr *attr;
    std::string defaultValue="";
    for (attr=schemaNodes.front()->properties;attr!=NULL;attr=attr->next) {
      if ((attr->type==XML_ATTRIBUTE_NODE)&&(strcmp((const char*)attr->name,"default")==0)) {
        defaultValue=(char *)attr->children->content;
      }
    }
    if (defaultValue=="") {
      FATAL("can not find default value for config path <%s/%s>",path.c_str(),name.c_str());
      core->getThread()->unlockMutex(mutex);
      return "";
    }

    // Create node
    setStringValue(path,name,defaultValue,true);

    // Find it again
    configNodes=findConfigNodes(xpath);
    if (configNodes.size()==0) {
      FATAL("could not find new node (%s/%s)",path.c_str(),name.c_str());
      core->getThread()->unlockMutex(mutex);
      return "";
    }

  }

  // Sanity check
  if (configNodes.size()!=1) {
    FATAL("more than one node found (%s/%s)",path.c_str(),name.c_str());
    core->getThread()->unlockMutex(mutex);
    return "";
  }
  node=configNodes.front();

  // Return result
  if ((!node->children)||(std::string((char*)node->children->name)!="text")) {
    FATAL("node has no text child",NULL);
    core->getThread()->unlockMutex(mutex);
    return "";
  }
  value=std::string((char*)node->children->content);
  core->getThread()->unlockMutex(mutex);
  return value;
}

// Sets a string value in the config
void ConfigStore::setStringValue(std::string path, std::string name, std::string value, bool innerCall)
{
  xmlDocPtr doc = config;
  xmlNodePtr node,rootNode;
  std::string xpath;
  if (path=="")
    xpath="/GDC/" + name;
  else
    xpath="/GDC/" + path + "/" + name;

  // Only one thread may enter setStringValue
  if (!innerCall) core->getThread()->lockMutex(mutex);

  // Check if node exists
  std::list<XMLNode> configNodes, schemaNodes;
  configNodes=findConfigNodes(xpath);
  if (configNodes.size() == 0) {

    // Check if node exists in the schema
    schemaNodes=findSchemaNodes(xpath);
    if (schemaNodes.size()!=1) {
      FATAL("could not find path <%s> in schema",path.c_str());
      if (!innerCall) core->getThread()->unlockMutex(mutex);
      return;
    }

    // No, so create node
    rootNode = xmlDocGetRootElement(doc);
    if (!rootNode) {
      FATAL("could not extract root node",NULL);
      if (!innerCall) core->getThread()->unlockMutex(mutex);
      return;
    }
    createNodeWithPath(rootNode,path,name,value);

  } else {

    // Sanity check
    if (configNodes.size()!=1) {
      FATAL("more than one node found",NULL);
      if (!innerCall) core->getThread()->unlockMutex(mutex);
      return;
    }
    node=configNodes.front();

    // Update node
    if ((!node->children)||(std::string((char*)node->children->name)!="text")) {
      FATAL("node has no text child",NULL);
      if (!innerCall) core->getThread()->unlockMutex(mutex);
      return;
    }
    xmlChar *valueEncoded = xmlEncodeSpecialChars(doc,(const xmlChar *)value.c_str());
    xmlNodeSetContent(node->children,valueEncoded);
    xmlFree(valueEncoded);
  }
  write();
  if (!innerCall) core->getThread()->unlockMutex(mutex);
}

// Returns a list of attribute values for a given path and attribute name
std::list<std::string> ConfigStore::getAttributeValues(std::string path, std::string attributeName) {

  std::list<XMLNode> nodes;
  std::list<std::string> values;
  xmlNodePtr n;

  core->getThread()->lockMutex(mutex);
  nodes=findConfigNodes("/GDC/" + path);
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
  core->getThread()->unlockMutex(mutex);
  return values;
}

// Lists all elements for the given path
std::list<std::string> ConfigStore::getNodeNames(std::string path) {
  std::string xpath = "/GDC" + path;
  if (path=="")
    xpath="/GDC";
  else
    xpath="/GDC/" + path;
  std::list<XMLNode> nodes = findSchemaNodes(xpath,"/xsd:complexType/xsd:sequence/xsd:element");
  std::list<std::string> names;
  for(std::list<XMLNode>::iterator i=nodes.begin();i!=nodes.end();i++) {
    for(xmlAttr *attr=(*i)->properties;attr!=NULL;attr=attr->next) {
       if (attr->type==XML_ATTRIBUTE_NODE) {
         if (strcmp((const char *)attr->name,"name")==0) {
           names.push_back(std::string((char*)attr->children->content));
         }
       }
    }
  }
  return names;
}

// Returns information about the given node
StringMap ConfigStore::getNodeInfo(std::string path) {

  StringMap info;

  // Find the node in the schema
  std::string xpath;
  if (path=="")
    xpath="/GDC";
  else
    xpath="/GDC/" + path;
  std::list<XMLNode> nodes = findSchemaNodes(xpath);
  if (nodes.size()!=1) {
    FATAL("could not find path <%s> in schema",path.c_str());
    return info;
  }
  XMLNode node=nodes.front();

  // Extract information
  bool typeFound=false;
  bool nameFound=false;
  bool isUnbounded=false;
  bool isOptional=false;
  for(xmlAttr *attr=node->properties;attr!=NULL;attr=attr->next) {
    if (attr->type==XML_ATTRIBUTE_NODE) {
      if (strcmp((const char *)attr->name,"name")==0) {
        info.insert(StringPair("name",(char*)attr->children->content));
        nameFound=true;
      }
      if (strcmp((const char *)attr->name,"maxOccurs")==0) {
        if (strcmp((const char *)attr->children->content,"unbounded")==0) {
          isUnbounded=true;
        }
      }
      if (strcmp((const char *)attr->name,"minOccurs")==0) {
        if (strcmp((const char *)attr->children->content,"0")==0) {
          info.insert(StringPair("isOptional","1"));
        }
      }
      if (strcmp((const char *)attr->name,"type")==0) {
        std::string type=(char*)attr->children->content;
        if (type=="xsd:integer") {
          type="integer";
        } else if (type=="xsd:string") {
          type="string";
        } else if (type=="xsd:boolean") {
          type="boolean";
        } else if (type=="xsd:double") {
          type="double";
        } else {
          FATAL("unknown type <%s> found in schema",type.c_str());
        }
        info.insert(StringPair("type",type));
        typeFound=true;
      }
    }
  }

  // Check that name was found
  if (!nameFound) {
    FATAL("element <%s> has no name!",path.c_str());
  }

  // Check for special types if no type could be extracted
  if (!typeFound) {

    // Color node?
    nodes = findSchemaNodes(xpath,"/xsd:complexType/xsd:sequence/xsd:element");
    bool redFound=false;
    bool greenFound=false;
    bool blueFound=false;
    bool alphaFound=false;
    if (nodes.size()==4) {
      for(std::list<XMLNode>::iterator i=nodes.begin();i!=nodes.end();i++) {
        node = *i;
        for(xmlAttr *attr=node->properties;attr!=NULL;attr=attr->next) {
          if (attr->type==XML_ATTRIBUTE_NODE) {
            if (strcmp((const char *)attr->name,"name")==0) {
              if (strcmp((char*)attr->children->content,"red")==0)
                redFound=true;
              if (strcmp((char*)attr->children->content,"green")==0)
                greenFound=true;
              if (strcmp((char*)attr->children->content,"blue")==0)
                blueFound=true;
              if (strcmp((char*)attr->children->content,"alpha")==0)
                alphaFound=true;
            }
          }
        }
      }
    }
    if (redFound&&greenFound&&blueFound&&alphaFound) {
      info.insert(StringPair("type","color"));
    } else {

      // Container node?
      if (nodes.size()!=0) {
        info.insert(StringPair("type","container"));
      } else {

        // Enumeration?
        nodes=findSchemaNodes(xpath,"/xsd:simpleType/xsd:restriction/xsd:enumeration");
        if (nodes.size()>0) {
          info.insert(StringPair("type","enumeration"));
          Int enumerationNumber=0;
          for(std::list<XMLNode>::iterator i=nodes.begin();i!=nodes.end();i++) {
            node=*i;
            for(xmlAttr *attr=node->properties;attr!=NULL;attr=attr->next) {
              if (attr->type==XML_ATTRIBUTE_NODE) {
                if (strcmp((const char *)attr->name,"value")==0) {
                  std::stringstream s; s << enumerationNumber;
                  info.insert(StringPair(s.str(),(char*)attr->children->content));
                }
              }
            }
            enumerationNumber++;
          }
        } else {
          FATAL("can not determine type of schema node",NULL);
        }
      }
    }
  }

  // In case of a container node, check if it has attributes
  if (info["type"]=="container") {

    // If it is an unbounded container, find out the iterating attribute
    if (isUnbounded) {
      bool nameAttributeFound=false;
      nodes=findSchemaNodes(xpath,"/xsd:complexType/xsd:attribute");
      for(std::list<XMLNode>::iterator i=nodes.begin();i!=nodes.end();i++) {
        node=*i;
        for(xmlAttr *attr=node->properties;attr!=NULL;attr=attr->next) {
          if (attr->type==XML_ATTRIBUTE_NODE) {
            if (strcmp((const char *)attr->name,"name")==0) {
              if (strcmp((const char *)attr->children->content,"name")==0) {
                info.insert(StringPair("isUnbounded","name"));
                nameAttributeFound=true;
              }
            }
          }
        }
      }
      if (!nameAttributeFound) {
        FATAL("container element <%s> is unbounded but has no name attribute",path.c_str());
      }
    }


  }

  // Get the description
  nodes=findSchemaNodes(xpath,"/xsd:annotation/xsd:documentation");
  if (nodes.size()!=1) {
    FATAL("element <%s> has no or more than one documentation",path.c_str());
  }
  info.insert(StringPair("documentation",(char*)nodes.front()->children->content));

  // That's it
  return info;
}

// Removes the node from the config
void ConfigStore::removePath(std::string path) {

  // Search for all matching nodes
  std::string xpath;
  if (path=="")
    xpath="/GDC";
  else
    xpath="/GDC/" + path;
  std::list<XMLNode> configNodes=findConfigNodes(xpath);

  // Remove all of them
  for (std::list<XMLNode>::iterator i=configNodes.begin();i!=configNodes.end();i++) {
    XMLNode n=*i;
    xmlUnlinkNode(n);
    xmlFreeNode(n);
  }
}

}
