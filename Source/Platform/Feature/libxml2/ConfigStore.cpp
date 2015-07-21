//============================================================================
// Name        : ConfigStore.cpp
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

// Error handlers for libxml2
void xmlGenericErrorHandler(void *ctx, const char *msg, ...) {
  const Int bufferSize=256;
  char string[bufferSize];
  char *message=strdup(msg);
  message[strlen(message)-1]=0; // skip new line
  va_list arg_ptr;
  va_start(arg_ptr, msg);
  vsnprintf(string, bufferSize, message, arg_ptr);
  va_end(arg_ptr);
  DEBUG("libxml2 generic error = %s",string);
  free(message);
}
void xmlStructuredErrorHandler(void *ctx, xmlErrorPtr p) {
  char *message=strdup(p->message);
  message[strlen(message)-1]=0; // skip new line
  DEBUG("libxml2 structured error = %s",message);
  free(message);
}

// Inits the data
void ConfigStore::init()
{
  if (!ConfigStore::parserInitialized) {
    xmlInitParser();
    xmlSetStructuredErrorFunc(NULL, &xmlStructuredErrorHandler);
    xmlSetGenericErrorFunc(NULL, &xmlGenericErrorHandler);
    ConfigStore::parserInitialized=true;
  }
  xmlKeepBlanksDefault(0);

  // Start the write thread
}

// Deinits the data
void ConfigStore::deinit()
{
  configSection->deinit();
}

// Called when the core is unloaded (process is killed)
void ConfigStore::unload()
{
  xmlCleanupParser();
  ConfigStore::parserInitialized=false;
}

// Reads the config with a given schema
void ConfigStore::read(std::string schemaFilepath, bool recreateConfig)
{
  xmlDocPtr doc = NULL;
  xmlNodePtr rootNode = NULL, node = NULL, node1 = NULL;
  xmlDtdPtr dtd = NULL;
  char buff[256];
  int i, j;

  // Read the schema
  if (!configSection->readSchema(schemaFilepath.c_str())) {
    return;
  }

  // Check if the file exists
  if (recreateConfig) {

    // No, so create empty XML config
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

    // Create the namespaces
    xmlNsPtr configNamespace=xmlNewNs(rootNode,BAD_CAST "http://www.untouchableapps.de/GeoDiscoverer/config/1/0", BAD_CAST NULL);
    if (!configNamespace) {
     FATAL("can not create config namespace",NULL);
     return;
    }
    xmlNsPtr xmlNamespace=xmlNewNs(rootNode,BAD_CAST "http://www.w3.org/2001/XMLSchema-instance", BAD_CAST "xsi");
    if (!xmlNamespace) {
     FATAL("can not create xml namespace",NULL);
     return;
    }
    if (!xmlNewNsProp(rootNode, xmlNamespace, BAD_CAST "schemaLocation", BAD_CAST "http://www.untouchableapps.de/GeoDiscoverer/config/1/0 http://www.untouchableapps.de/GeoDiscoverer/config/1/0/config.xsd")) {
      FATAL("can not create xml property",NULL);
      return;
    }

    // Save the XML config
    configSection->setConfig(doc);
    hasChanged=true;
    write();

    // Cleanup
    xmlFreeDoc(doc);
    configSection->setConfig(NULL);
  }

  // Read the config
  if (!configSection->readConfig(configFilepath)) {
    FATAL("can not read configuration",NULL);
    return;
  }
  doc = configSection->getConfig();
  rootNode = xmlDocGetRootElement(doc);
  if (!rootNode) {
    FATAL("could not extract root node",NULL);
    return;
  }

  // Version handling
  xmlChar *text = xmlGetProp(rootNode,BAD_CAST "version");
  if (strcmp((char*)text,"1.0")!=0) {
    FATAL("config file <%s> has unknown version",configFilepath.c_str());
    return;
  }
  xmlFree(text);

  // Remember the config
  configSection->setConfig(doc);

  // Prepare xpath evaluation
  configSection->prepareXPath("http://www.untouchableapps.de/GeoDiscoverer/config/1/0");
}

// Extracts all schema nodes that hold user-definable values
void ConfigStore::rememberUserConfig(std::string path, XMLNode nodes) {
  std::string newPath = path;
  std::string attributeName;
  std::list<std::string> attributeValues;
  bool oneAttributeFound=false;

  // First extract the information for this level
  for(XMLNode i=nodes;i!=NULL;i=i->next) {
    if (i->type==XML_ELEMENT_NODE) {
      if (std::string((const char *)i->name)=="element") {
        if (i->properties) {
          std::string name;
          bool rememberConfig=false;
          for(XMLAttribute j=i->properties;j!=NULL;j=j->next) {
            if (std::string((const char *)j->name)=="name") {
              name=std::string((const char *)j->children->content);
            }
            if (std::string((const char *)j->name)=="upgrade") {
              if (std::string((const char *)j->children->content)=="restore") {
                rememberConfig=true;
              }
            }
          }
          if (rememberConfig) {
            std::vector<std::string> t(3);
            t[0]=path;
            t[1]=name;
            t[2]=getStringValue(path,name, __FILE__, __LINE__);
            migratedUserConfig.push_back(t);
            //DEBUG("required node found: path=%s name=%s value=%s",t[0].c_str(),t[1].c_str(),t[2].c_str());
          }
        }
      }
      if (std::string((const char *)i->name)=="attribute") {
        if (i->properties) {
          for(XMLAttribute j=i->properties;j!=NULL;j=j->next) {
            if (std::string((const char *)j->name)=="name") {
              if (oneAttributeFound) {
                FATAL("only one attribute per element is supported",NULL);
              } else {
                attributeName=std::string((const char *)j->children->content);
                if (path!="")
                  attributeValues=getAttributeValues(path,attributeName, __FILE__, __LINE__);
                oneAttributeFound=true;
              }
            }
          }
        }
      }
    }
  }

  // Then do the recursion
  for(XMLNode i=nodes;i!=NULL;i=i->next) {
    if (i->type==XML_ELEMENT_NODE) {
      if (std::string((const char *)i->name)=="element") {
        if (i->properties) {
          std::string name;
          bool rememberConfig=false;
          for(XMLAttribute j=i->properties;j!=NULL;j=j->next) {
            if (std::string((const char *)j->name)=="name") {
              name=std::string((const char *)j->children->content);
            }
          }
          if (path=="") {
            if (name=="GDC")
              newPath="";
            else
              newPath = name;
          } else {
            newPath = path + "/" + name;
          }
        }
      }
      if (i->children) {
        if (attributeValues.size()>0) {
          for(std::list<std::string>::iterator j=attributeValues.begin();j!=attributeValues.end();j++) {
            rememberUserConfig(newPath + "[@" + attributeName + "='" + *j + "']",i->children);
          }
        } else {
          rememberUserConfig(newPath,i->children);
        }
      }
    }
  }
}

// Reads the config
void ConfigStore::read()
{
  xmlDocPtr doc = NULL;
  xmlNodePtr rootNode = NULL, node = NULL, node1 = NULL;
  xmlDtdPtr dtd = NULL;
  char buff[256];
  int i, j;
  struct stat schemaShippedStat;
  struct stat schemaCurrentStat;
  bool recreateConfig=false;
  bool copySchema=false;

  // Only one thread may enter read
  core->getThread()->lockMutex(accessMutex, __FILE__, __LINE__);

  // Compare the shipped and the current schema
  if (stat(schemaShippedFilepath.c_str(),&schemaShippedStat)!=0) {
    FATAL("can not obtain file stats of <%s>!",schemaShippedFilepath.c_str());
    core->getThread()->unlockMutex(accessMutex);
    return;
  }
  if ((stat(schemaCurrentFilepath.c_str(),&schemaCurrentStat)!=0)||(access(configFilepath.c_str(),F_OK))) {

    // Current schema or config does not exist, so re-create the config
    recreateConfig=true;
    copySchema=true;

  } else {

    // Check if the schemas have changed
    bool schemaHasChanged=false;
    if (schemaShippedStat.st_size!=schemaCurrentStat.st_size) {
      schemaHasChanged=true;
    } else {
      FILE *in1=fopen(schemaShippedFilepath.c_str(),"r");
      FILE *in2=fopen(schemaCurrentFilepath.c_str(),"r");
      if ((in1==NULL)||(in2==NULL)) {
        FATAL("can not read shipped or current schema",NULL);
        core->getThread()->unlockMutex(accessMutex);
        return;
      }
      UByte *buf1, *buf2;
      const Int bufSize=16384;
      buf1=(UByte*)malloc(bufSize);
      buf2=(UByte*)malloc(bufSize);
      if ((buf1==NULL)||(buf2==NULL)) {
        FATAL("can not create buffers",NULL);
        core->getThread()->unlockMutex(accessMutex);
        return;
      }
      while (!feof(in1)) {
        Int readBytes1 = fread(buf1,1,bufSize,in1);
        Int readBytes2 = fread(buf2,1,bufSize,in2);
        if (readBytes1!=readBytes2) {
          schemaHasChanged=true;
          break;
        }
        if (memcmp(buf1,buf2,readBytes1)!=0) {
          schemaHasChanged=true;
          break;
        }
      }
      if (!feof(in2)) {
        schemaHasChanged=true;
      }
      free(buf1);
      free(buf2);
      fclose(in1);
      fclose(in2);
    }
    if (schemaHasChanged) {

      // Copy all user config values from the old config
      INFO("upgrading schema while keeping all user configuration",NULL);
      read(schemaShippedFilepath,false);
      rememberUserConfig("",configSection->getSchema()->children->children);

      // Free resources
      deinit();
      recreateConfig=true;

      // The new schema must become the current schema
      copySchema=true;

    }
  }

  // Make the shipped schema the new schema
  if (copySchema) {
    std::ifstream src(schemaShippedFilepath.c_str(), std::ios::binary);
    std::ofstream dst(schemaCurrentFilepath.c_str(), std::ios::binary);
    dst << src.rdbuf();
  }

  // Read the config
  read(schemaCurrentFilepath,recreateConfig);

  // Add any migrated user config (if any)
  for (std::list<std::vector<std::string> >::iterator i=migratedUserConfig.begin();i!=migratedUserConfig.end();i++) {
    std::vector<std::string> entry = *i;
    setStringValue(entry[0],entry[1],entry[2], __FILE__, __LINE__);
  }

  // That's it
  core->getThread()->unlockMutex(accessMutex);
}

// Writes the config
void ConfigStore::write()
{
  if (hasChanged) {
    xmlDocPtr doc = (xmlDocPtr) configSection->getConfig();
    if (!doc) {
      FATAL("config object has no xml document",NULL);
      return;
    }
    std::string tempFilepath = configFilepath + "+";
    Int tries;
    for(tries=0;tries<core->getFileOpenForWritingRetries();tries++) {
      if (xmlSaveFormatFileEnc(tempFilepath.c_str(), doc, "UTF-8", 1)!=-1) {
        break;
      }
      usleep(core->getFileOpenForWritingWaitTime());
    }
    if (tries!=0)
      DEBUG("config write tried %d times",tries+1);
    if (tries>=core->getFileOpenForWritingRetries()) {
      FATAL("can not write configuration file <%s>",configFilepath.c_str());
      return;
    }
    rename(tempFilepath.c_str(),configFilepath.c_str());
  }
  hasChanged=false;
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
  return configSection->findConfigNodes(path);
}

// Finds schema nodes
std::list<XMLNode> ConfigStore::findSchemaNodes(std::string path, std::string extension) {
  return configSection->findSchemaNodes(path,extension);
}

// Gets a string value from the config
std::string ConfigStore::getStringValue(std::string path, std::string name, const char *file, int line)
{
  std::string value;
  xmlNodePtr node,rootNode,childNode;
  std::string xpath;
  if (path=="")
    xpath="/GDC/" + name;
  else
    xpath="/GDC/" + path + "/" + name;

  // Only one thread may enter getStringValue
  core->getThread()->lockMutex(accessMutex, file, line);

  // Check if node exists
  std::list<XMLNode> configNodes,schemaNodes;
  configNodes=findConfigNodes(xpath);
  if (configNodes.size() == 0) {

    // Find the schema node
    schemaNodes=findSchemaNodes(xpath);
    if (schemaNodes.size()!=1) {
      FATAL("could not find path <%s/%s> in schema",path.c_str(),name.c_str());
      core->getThread()->unlockMutex(accessMutex);
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
      core->getThread()->unlockMutex(accessMutex);
      return "";
    }

    // Create node
    setStringValue(path,name,defaultValue,file,line);

    // Find it again
    configNodes=findConfigNodes(xpath);
    if (configNodes.size()==0) {
      FATAL("could not find new node (%s/%s)",path.c_str(),name.c_str());
      core->getThread()->unlockMutex(accessMutex);
      return "";
    }

  }

  // Sanity check
  if (configNodes.size()!=1) {
    FATAL("more than one node found (%s/%s)",path.c_str(),name.c_str());
    core->getThread()->unlockMutex(accessMutex);
    return "";
  }
  node=configNodes.front();

  // Return result
  if ((!node->children)||(std::string((char*)node->children->name)!="text")) {
    FATAL("node has no text child",NULL);
    core->getThread()->unlockMutex(accessMutex);
    return "";
  }
  value=std::string((char*)node->children->content);
  core->getThread()->unlockMutex(accessMutex);
  return value;
}

// Sets a string value in the config
void ConfigStore::setStringValue(std::string path, std::string name, std::string value, const char *file, int line)
{
  xmlDocPtr doc = configSection->getConfig();
  xmlNodePtr node,rootNode;
  std::string xpath;
  if (path=="")
    xpath="/GDC/" + name;
  else
    xpath="/GDC/" + path + "/" + name;

  // Only one thread may enter setStringValue
  core->getThread()->lockMutex(accessMutex, file, line);

  // Check if node exists
  std::list<XMLNode> configNodes, schemaNodes;
  configNodes=findConfigNodes(xpath);
  if (configNodes.size() == 0) {

    // Check if node exists in the schema
    schemaNodes=findSchemaNodes(xpath);
    if (schemaNodes.size()!=1) {
      FATAL("could not find path <%s> in schema",path.c_str());
      core->getThread()->unlockMutex(accessMutex);
      return;
    }

    // No, so create node
    rootNode = xmlDocGetRootElement(doc);
    if (!rootNode) {
      FATAL("could not extract root node",NULL);
      core->getThread()->unlockMutex(accessMutex);
      return;
    }
    createNodeWithPath(rootNode,path,name,value);

  } else {

    // Sanity check
    if (configNodes.size()!=1) {
      FATAL("more than one node found",NULL);
      core->getThread()->unlockMutex(accessMutex);
      return;
    }
    node=configNodes.front();

    // Update node
    if ((!node->children)||(std::string((char*)node->children->name)!="text")) {
      FATAL("node has no text child",NULL);
      core->getThread()->unlockMutex(accessMutex);
      return;
    }
    xmlChar *valueEncoded = xmlEncodeSpecialChars(doc,(const xmlChar *)value.c_str());
    xmlNodeSetContent(node->children,valueEncoded);
    xmlFree(valueEncoded);
  }
  hasChanged=true;
  core->getThread()->issueSignal(writeConfigSignal);
  core->getThread()->unlockMutex(accessMutex);
}

// Returns a list of attribute values for a given path and attribute name
std::list<std::string> ConfigStore::getAttributeValues(std::string path, std::string attributeName, const char *file, int line) {

  std::list<XMLNode> nodes;
  std::list<std::string> values;
  xmlNodePtr n;

  core->getThread()->lockMutex(accessMutex, file, line);
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
  core->getThread()->unlockMutex(accessMutex);
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
