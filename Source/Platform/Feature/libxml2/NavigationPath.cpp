//============================================================================
// Name        : NavigationPath.cpp
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

// Namespaces used when reading gpx files
const char *GPX10Namespace="http://www.topografix.com/GPX/1/0";
const char *GPX11Namespace="http://www.topografix.com/GPX/1/1";
const char *GDNamespace="http://www.perfectapp.de/GeoDiscoverer/GPXExtensions/1/0";

// Writes the path contents into a gpx file
void NavigationPath::writeGPXFile() {

  xmlNodePtr segmentNode;
  xmlNodePtr node;
  std::string filepath=gpxFilefolder + "/" + gpxFilename;

  // Only store if it has changed
  if (isStored) {
    //DEBUG("path is already stored, skipping write",NULL);
    return;
  } else {
    //DEBUG("path has not been stored, writing to disk",NULL);
  }

  // Backup the previous file
  if (access(filepath.c_str(),F_OK)==0) {
    std::string backupFilepath=filepath + "~";
    rename(filepath.c_str(),backupFilepath.c_str());
  }

  // Create empty XML document
  xmlDocPtr doc = NULL;
  doc = xmlNewDoc(BAD_CAST "1.0");
  if (!doc) {
    FATAL("can not create xml document",NULL);
    return;
  }

  // Create the gpx root node
  xmlNodePtr gpxNode = NULL;
  gpxNode = xmlNewNode(NULL, BAD_CAST "gpx");
  if (!gpxNode) {
    FATAL("can not create xml node",NULL);
    return;
  }
  if (!xmlNewProp(gpxNode, BAD_CAST "version", BAD_CAST "1.1")) {
    FATAL("can not create xml property",NULL);
    return;
  }
  if (!xmlNewProp(gpxNode, BAD_CAST "creator", BAD_CAST "Geo Discoverer")) {
    FATAL("can not create xml property",NULL);
    return;
  }
  xmlDocSetRootElement(doc, gpxNode);

  // Create the namespaces
  xmlNsPtr gpxNamespace=xmlNewNs(gpxNode,BAD_CAST "http://www.topografix.com/GPX/1/1", BAD_CAST NULL);
  if (!gpxNamespace) {
   FATAL("can not create xml namespace",NULL);
   return;
  }
  xmlNsPtr xmlNamespace=xmlNewNs(gpxNode,BAD_CAST "http://www.w3.org/2001/XMLSchema-instance", BAD_CAST "xsi");
  if (!xmlNamespace) {
   FATAL("can not create xml namespace",NULL);
   return;
  }
  if (!xmlNewNsProp(gpxNode, xmlNamespace, BAD_CAST "schemaLocation", BAD_CAST "http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd")) {
    FATAL("can not create xml property",NULL);
    return;
  }

  // Create the metadata node
  xmlNodePtr metadataNode;
  metadataNode = xmlNewNode(NULL, BAD_CAST "metadata");
  if (!metadataNode) {
    FATAL("can not create xml node",NULL);
    return;
  }
  xmlAddChild(gpxNode,metadataNode);
  node=xmlNewTextChild(metadataNode, NULL, BAD_CAST "name", BAD_CAST name.c_str());
  if (!node) {
    FATAL("can not create xml node",NULL);
    return;
  }
  node=xmlNewTextChild(metadataNode, NULL, BAD_CAST "desc", BAD_CAST description.c_str());
  if (!node) {
    FATAL("can not create xml node",NULL);
    return;
  }
  node=xmlNewChild(metadataNode, NULL, BAD_CAST "time", BAD_CAST core->getClock()->getXMLDate().c_str());
  if (!node) {
    FATAL("can not create xml node",NULL);
    return;
  }

  // Create the trk node
  xmlNodePtr pathNode;
  pathNode = xmlNewNode(NULL, BAD_CAST "trk");
  if (!pathNode) {
    FATAL("can not create xml node",NULL);
    return;
  }
  xmlAddChild(gpxNode,pathNode);
  node=xmlNewTextChild(pathNode, NULL, BAD_CAST "name", BAD_CAST name.c_str());
  if (!node) {
    FATAL("can not create xml node",NULL);
    return;
  }
  node=xmlNewTextChild(pathNode, NULL, BAD_CAST "desc", BAD_CAST description.c_str());
  if (!node) {
    FATAL("can not create xml node",NULL);
    return;
  }

  // Create the first segment
  segmentNode = xmlNewNode(NULL, BAD_CAST "trkseg");
  if (!segmentNode) {
    FATAL("can not create xml node",NULL);
    return;
  }
  xmlAddChild(pathNode,segmentNode);

  // Iterate through all points
  for (std::list<MapPosition>::iterator i=mapPositions.begin();i!=mapPositions.end();i++) {
    MapPosition pos=*i;

    // Start of a new segment?
    if (pos==NavigationPath::getPathInterruptedPos()) {

      // Create a new segment
      segmentNode = xmlNewNode(NULL, BAD_CAST "trkseg");
      if (!segmentNode) {
        FATAL("can not create xml node",NULL);
        return;
      }
      xmlAddChild(pathNode,segmentNode);

    } else {

      // Add the point to the segment
      pos.writeGPX(segmentNode);

    }
  }

  // Write the file
  if (xmlSaveFormatFileEnc(filepath.c_str(), doc, "UTF-8", 1)==-1) {
    ERROR("can not write gpx file <%s>",filepath.c_str());
    return;
  }

  // Cleanup
  xmlFreeDoc(doc);
  isStored=true;
}

// Finds nodes in a xml tree
std::list<XMLNode> NavigationPath::findNodes(XMLDocument document, XMLXPathContext xpathCtx, std::string path) {
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

// Returns the text contents of a element node
bool NavigationPath::getText(XMLNode node, std::string &contents) {
  if ((!node->children)||(std::string((char*)node->children->name)!="text"))
    return false;
  contents=std::string((char*)node->children->content);
  if (contents=="")
    return false;
  else
    return true;
}

// Extracts information about the path from the given node set
void NavigationPath::extractInformation(std::list<XMLNode> nodes) {
  std::string t;
  for(std::list<XMLNode>::iterator i=nodes.begin();i!=nodes.end();i++) {
    XMLNode node=*i;
    std::string nodeName=(char*)node->name;
    if (nodeName=="name") {
      if (getText(node,t)) name=t;
    }
    if (nodeName=="desc") {
      if (getText(node,t)) description=t;
    }
  }
}

// Reads the path contents from a gpx file
bool NavigationPath::readGPXFile() {

  std::string filepath=gpxFilefolder + "/" + gpxFilename;
  XMLNode node;
  XMLDocument doc=NULL;
  XMLXPathContext xpathCtx=NULL;
  bool GPX10=false;
  bool GPX11=false;
  std::string namespaceURI;
  std::list<XMLNode> nodes;
  std::string t;
  Int numberOfSegments,numberOfPoints,totalNumberOfPoints,processedPoints;
  Int processedPercentage, prevProcessedPercentage=-1;
  std::list<XMLNode> trackNodes;
  std::list<XMLNode> routeNodes;
  bool loadTrack=false;
  bool loadRoute=false;
  MapPosition pos;
  bool result=false;

  // Create the progress dialog
  std::string dialogTitle="Reading <" + gpxFilename + ">";
  DialogKey dialog=core->getDialog()->createProgress(dialogTitle,100);

  // Read the document
  doc = xmlReadFile(filepath.c_str(), NULL, 0);
  if (!doc) {
    ERROR("can not read file <%s>",gpxFilename.c_str());
    goto cleanup;
  }

  // Prepare xpath evaluation
  xpathCtx = xmlXPathNewContext(doc);
  if (xpathCtx == NULL) {
    FATAL("can not create xpath context",NULL);
    goto cleanup;
  }

  // Check which version of gpx it is
  namespaceURI="//*[namespace-uri()='" + std::string(GPX10Namespace) + "']";
  nodes=findNodes(doc,xpathCtx,namespaceURI.c_str());
  if (nodes.size()!=0) {
    GPX10=true;
  }
  namespaceURI="//*[namespace-uri()='" + std::string(GPX11Namespace) + "']";
  nodes=findNodes(doc,xpathCtx,namespaceURI.c_str());
  if (nodes.size()!=0) {
    GPX11=true;
  }
  if ((!GPX10)&&(!GPX11)) {
    ERROR("file <%s> can not be parsed because it is not a V1.0 or V1.1 GPX file",gpxFilename.c_str());
    goto cleanup;
  }
  if (GPX10&&GPX11) {
    ERROR("file <%s> can not be parsed because it contains both V1.0 and V1.1 GPX elements",gpxFilename.c_str());
    goto cleanup;
  }

  // Register namespaces
  if (GPX10) {
    if(xmlXPathRegisterNs(xpathCtx, BAD_CAST "gpx", BAD_CAST GPX10Namespace) != 0) {
      FATAL("can not register namespace",NULL);
      goto cleanup;
    }
  }
  if (GPX11) {
    if(xmlXPathRegisterNs(xpathCtx, BAD_CAST "gpx", BAD_CAST GPX11Namespace) != 0) {
      FATAL("can not register namespace",NULL);
      goto cleanup;
    }
  }
  if(xmlXPathRegisterNs(xpathCtx, BAD_CAST "gd", BAD_CAST GDNamespace) != 0) {
    FATAL("can not register namespace",NULL);
    goto cleanup;
  }

  // Decide which type to load (route or track)
  trackNodes=findNodes(doc,xpathCtx,"/gpx:gpx/gpx:trk");
  routeNodes=findNodes(doc,xpathCtx,"/gpx:gpx/gpx:rte");
  if ((trackNodes.size()>0)&&(routeNodes.size()>0)) {
    WARNING("file <%s> contains both a route and a track, loading the track only",gpxFilename.c_str());
  }
  if (trackNodes.size()>0) {
    if (trackNodes.size()>1) {
      WARNING("file <%s> contains more than one track, only loading the first",gpxFilename.c_str());
    }
    loadTrack=true;
  } else {
    if (routeNodes.size()>0) {
      if (routeNodes.size()>1) {
        WARNING("file <%s> contains more than one route, only loading the first",gpxFilename.c_str());
      }
      loadRoute=true;
    } else {
      ERROR("file <%s> does neither contain a route nor a track",gpxFilename.c_str());
    }
  }

  // Extract data from the metadata section if it exists
  if (GPX11) {
    nodes=findNodes(doc,xpathCtx,"/gpx:gpx/gpx:metadata/*");
    extractInformation(nodes);
  }
  if (GPX10) {
    nodes=findNodes(doc,xpathCtx,"/gpx:gpx/*");
    extractInformation(nodes);
  }

  // Load the points of the path
  if (loadTrack) {
    nodes=findNodes(doc,xpathCtx,"/gpx:gpx/gpx:trk[1]/*");
    extractInformation(nodes);
    nodes=findNodes(doc,xpathCtx,"/gpx:gpx/gpx:trk[1]/gpx:trkseg/gpx:trkpt");
    totalNumberOfPoints=nodes.size();
    processedPoints=0;
    nodes=findNodes(doc,xpathCtx,"/gpx:gpx/gpx:trk[1]/gpx:trkseg");
    numberOfSegments=nodes.size();
    for(Int i=1;i<=numberOfSegments;i++) {
      std::stringstream trksegPath;
      trksegPath<<"/gpx:gpx/gpx:trk[1]/gpx:trkseg["<<i<<"]/gpx:trkpt";
      nodes=findNodes(doc,xpathCtx,trksegPath.str());
      numberOfPoints=nodes.size();
      for(std::list<XMLNode>::iterator j=nodes.begin();j!=nodes.end();j++) {
        XMLNode node=*j;
        std::string error;
        if (pos.readGPX(node,error)) {
          addEndPosition(pos);
        } else {
          error="file <%s> " + error;
          ERROR(error.c_str(),gpxFilename.c_str());
          goto cleanup;
        }
        processedPoints++;
        processedPercentage=processedPoints*100/totalNumberOfPoints;
        if (processedPercentage!=prevProcessedPercentage)
          core->getDialog()->updateProgress(dialog,dialogTitle,processedPercentage);
        prevProcessedPercentage=processedPercentage;
      }
      if (i!=numberOfSegments)
        addEndPosition(NavigationPath::getPathInterruptedPos());
    }
  }
  if (loadRoute) {
    nodes=findNodes(doc,xpathCtx,"/gpx:gpx/gpx:rte[1]/*");
    extractInformation(nodes);
    nodes=findNodes(doc,xpathCtx,"/gpx:gpx/gpx:rte[1]/gpx:rtept");
    numberOfPoints=nodes.size();
    totalNumberOfPoints=numberOfPoints;
    processedPoints=0;
    for(std::list<XMLNode>::iterator j=nodes.begin();j!=nodes.end();j++) {
      XMLNode node=*j;
      std::string error;
      if (pos.readGPX(node,error)) {
        addEndPosition(pos);
      } else {
        error="file <%s> " + error;
        ERROR(error.c_str(),gpxFilename.c_str());
        goto cleanup;
      }
      processedPoints++;
      processedPercentage=processedPoints*100/totalNumberOfPoints;
      if (processedPercentage!=prevProcessedPercentage)
        core->getDialog()->updateProgress(dialog,dialogTitle,processedPercentage);
      prevProcessedPercentage=processedPercentage;
    }
  }

  // Cleanup
  result=true;
cleanup:
  if (xpathCtx) xmlXPathFreeContext(xpathCtx);
  if (doc) xmlFreeDoc(doc);
  core->getDialog()->closeProgress(dialog);
  isStored=true;
  hasChanged=true;
  hasBeenLoaded=true;
  return result;
}

}
