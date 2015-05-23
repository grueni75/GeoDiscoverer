//============================================================================
// Name        : MapPosition.cpp
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

// Adds the point to the gpx xml tree
void MapPosition::writeGPX(XMLNode parentNode) {

  std::stringstream out;
  XMLNode node;

  // Create the container
  XMLNode pointNode = xmlNewNode(NULL, BAD_CAST "trkpt");
  if (!pointNode) {
    FATAL("can not create xml node",NULL);
    return;
  }
  xmlAddChild(parentNode,pointNode);

  // Add the contents of the position
  out.str(""); out << lat;
  if (!xmlNewProp(pointNode, BAD_CAST "lat", BAD_CAST out.str().c_str())) {
    FATAL("can not create xml property",NULL);
    return;
  }
  out.str(""); out << lng;
  if (!xmlNewProp(pointNode, BAD_CAST "lon", BAD_CAST out.str().c_str())) {
    FATAL("can not create xml property",NULL);
    return;
  }
  node=xmlNewChild(pointNode, NULL, BAD_CAST "time", BAD_CAST core->getClock()->getXMLDate(timestamp/1000,false).c_str());
  if (!node) {
   FATAL("can not create xml node",NULL);
   return;
  }
  if (hasAltitude) {
    out.str(""); out << altitude;
    node=xmlNewChild(pointNode, NULL, BAD_CAST "ele", BAD_CAST out.str().c_str());
    if (!node) {
     FATAL("can not create xml node",NULL);
     return;
    }
  }

  // Create the geo discoverer point element in the extensions element
  XMLNode extensionsNode = xmlNewNode(NULL, BAD_CAST "extensions");
  if (!extensionsNode) {
    FATAL("can not create xml node",NULL);
    return;
  }
  xmlAddChild(pointNode,extensionsNode);
  XMLNode gdNode = xmlNewNode(NULL, BAD_CAST "pt");
  if (!gdNode) {
    FATAL("can not create xml node",NULL);
    return;
  }
  xmlAddChild(extensionsNode,gdNode);

  // Create the namespaces
  xmlNsPtr geoDiscovererNamespace=xmlNewNs(gdNode,BAD_CAST "http://www.untouchableapps.de/GeoDiscoverer/GPXExtensions/1/0", BAD_CAST NULL);
  if (!geoDiscovererNamespace) {
   FATAL("can not create xml namespace",NULL);
   return;
  }
  xmlNsPtr xmlNamespace=xmlNewNs(gdNode,BAD_CAST "http://www.w3.org/2001/XMLSchema-instance", BAD_CAST "xsi");
  if (!xmlNamespace) {
   FATAL("can not create xml namespace",NULL);
   return;
  }

  // Add the remaining information
  if (source) {
    node=xmlNewTextChild(gdNode, NULL, BAD_CAST "source", BAD_CAST source);
    if (!node) {
     FATAL("can not create xml node",NULL);
     return;
    }
  }
  if (hasBearing) {
    out.str(""); out << bearing;
    node=xmlNewChild(gdNode, NULL, BAD_CAST "bearing", BAD_CAST out.str().c_str());
    if (!node) {
     FATAL("can not create xml node",NULL);
     return;
    }
  }
  if (hasSpeed) {
    out.str(""); out << speed;
    node=xmlNewChild(gdNode, NULL, BAD_CAST "speed", BAD_CAST out.str().c_str());
    if (!node) {
     FATAL("can not create xml node",NULL);
     return;
    }
  }
  if (hasAccuracy) {
    out.str(""); out << accuracy;
    node=xmlNewChild(gdNode, NULL, BAD_CAST "accuracy", BAD_CAST out.str().c_str());
    if (!node) {
     FATAL("can not create xml node",NULL);
     return;
    }
  }
}

// Returns the text contents of a element node
bool MapPosition::getText(XMLNode node, std::string &contents) {
  if (!node->children)
    return false;
  if (!node->children->name)
    return false;
  if ((std::string((char*)node->children->name)!="text"))
    return false;
  if (!node->children->content)
    return false;
  contents=std::string((char*)node->children->content);
  if (contents=="")
    return false;
  else
    return true;
}

// Checks if the namespace of the given xml node is a valid geo discoverer namespace
bool MapPosition::isGDNameSpace(XMLNode node) {
  if (node->ns!=NULL) {
    std::string href=(char*)node->ns->href;
    if (href == GDNamespace)
      return true;
  }
  return false;
}

// Checks if the namespace of the given xml node is a valid gpx namespace
bool MapPosition::isGPXNameSpace(XMLNode node) {
  if (node->ns!=NULL) {
    std::string href=(char*)node->ns->href;
    if (href == GPX10Namespace)
      return true;
    if (href == GPX11Namespace)
      return true;
    return false;
  } else {
    return true;
  }
}

// Extracts the point from the gpx xml node list
bool MapPosition::readGPX(XMLNode wptNode, std::string &error) {

  // Check namespace
  if (!isGPXNameSpace(wptNode)) {
    FATAL("xml node has unexpected namespace",NULL);
    return false;
  }

  // Extract longitude and latitude from attributes
  xmlChar *text;
  std::istringstream iss;
  if (!(text=xmlGetProp(wptNode,BAD_CAST "lat"))) {
    error="contains a waypoint that has no latitude";
    return false;
  }
  iss.str((char*)text); iss.clear(); iss >> lat;
  if (text) xmlFree(text);
  if (!(text=xmlGetProp(wptNode,BAD_CAST "lon"))) {
    error="contains a waypoint that has no longitude";
    return false;
  }
  iss.str((char*)text); iss.clear(); iss >> lng;
  if (text) xmlFree(text);

  // If there are no children stop here
  if (!wptNode->children)
    return true;

  // Go through children and extract standard information
  XMLNode extensionsNode=NULL;
  for (XMLNode node=wptNode->children;node!=NULL;node=node->next) {
    if ((isGPXNameSpace(node))&&(node->type==XML_ELEMENT_NODE)) {
      std::string name=(char*)node->name;
      std::string text;
      if (getText(node,text)) {
        iss.str(text);
        iss.clear();
        if (name == "ele") {
          hasAltitude=true;
          iss >> altitude;
        }
        if (name == "time") {
          //DEBUG("before conversion: %s",text.c_str());
          hasTimestamp=true;
          timestamp=core->getClock()->getXMLDate(text,false);
          timestamp*=1000;
          //DEBUG("after conversion: %s",core->getClock()->getXMLDate(timestamp/1000,false).c_str());
        }
      }
      if (name == "extensions") {
        extensionsNode=node;
      }
    }
  }

  // Extensions node available?
  if (!extensionsNode)
    return true;

  // Find the geo discoverer extension
  XMLNode gdPoint=NULL;
  for (XMLNode node=extensionsNode->children;node=node->next;node!=NULL) {
    if ((isGDNameSpace(node))&&(node->type==XML_ELEMENT_NODE)) {
      std::string name=(char*)node->name;
      if (name=="pt") {
        gdPoint=node;
        break;
      }
    }
  }

  // Geo Discoverer extension available?
  if (gdPoint) {
    for (XMLNode node=gdPoint->children;node=node->next;node!=NULL) {
      if ((isGDNameSpace(node))&&(node->type==XML_ELEMENT_NODE)) {
        std::string name=(char*)node->name;
        std::string text;
        if (getText(node,text)) {
          iss.str(text);
          iss.clear();
          if (name == "source") {
            setSource(text.c_str());
          }
          if (name == "bearing") {
            hasBearing=true;
            iss >> bearing;
          }
          if (name == "speed") {
            hasSpeed=true;
            iss >> speed;
          }
          if (name == "accuracy") {
            hasAccuracy=true;
            iss >> accuracy;
          }
        }
      }
    }
  }

  return true;
}

}
