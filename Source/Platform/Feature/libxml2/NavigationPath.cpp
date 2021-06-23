//============================================================================
// Name        : NavigationPath.cpp
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
#include <NavigationPath.h>
#include <MapPosition.h>
#include <NavigationEngine.h>
#include <NavigationPoint.h>
#include <Commander.h>

namespace GEODISCOVERER {

// Namespaces used when reading gpx files
const char *GPX10Namespace="http://www.topografix.com/GPX/1/0";
const char *GPX11Namespace="http://www.topografix.com/GPX/1/1";
const char *GDNamespace="http://www.untouchableapps.de/GeoDiscoverer/GPXExtensions/1/0";

// Writes the path contents into a gpx file
void NavigationPath::writeGPXFile(bool forceStorage, bool skipBackup, bool onlySelectedPath, bool skipExtensions, std::string name, std::string filepath) {

  xmlNodePtr segmentNode;
  xmlNodePtr node;

  // Set the location to use
  if (filepath=="")
    filepath=gpxFilefolder + "/" + gpxFilename;
  //DEBUG("filepath=%s",filepath.c_str());

  // Only store if it is initialized
  if (!getIsInit())
    return;

  // Only store if it has changed
  if (!forceStorage) {
    if (isStored) {
      //DEBUG("path is already stored, skipping write",NULL);
      return;
    } else {
      //DEBUG("path has not been stored, writing to disk",NULL);
    }
  }

  // Copy all the data needed such that we do not need to lock the path too long
  lockAccess(__FILE__, __LINE__);
  if (name=="")
    name=this->name;
  std::string description=this->description;
  std::vector<MapPosition> mapPositions;
  bool reverse=false;
  if (onlySelectedPath) {
    reverse=this->reverse;
    mapPositions=getSelectedPoints();
    if (reverse) {
      std::reverse(mapPositions.begin(), mapPositions.end());
    }
  } else {
    mapPositions=this->mapPositions;
  }
  unlockAccess();

  // Backup the previous file
  if (!skipBackup) {
    if (access(filepath.c_str(),F_OK)==0) {
      std::string backupFilepath=filepath + "~";
      rename(filepath.c_str(),backupFilepath.c_str());
    }
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
   FATAL("can not create gpx namespace",NULL);
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
  //WARNING("current XML date = %s",core->getClock()->getXMLDate().c_str());
  node=xmlNewChild(metadataNode, NULL, BAD_CAST "time", BAD_CAST core->getClock()->getXMLDate(0,false).c_str());
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
  for (std::vector<MapPosition>::iterator i=mapPositions.begin();i!=mapPositions.end();i++) {
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
      pos.writeGPX(segmentNode,skipExtensions);

    }
  }

  // Write the file
  std::string tempFilepath = filepath + "+";
  Int tries;
  for(tries=0;tries<core->getFileOpenForWritingRetries();tries++) {
    if (xmlSaveFormatFileEnc(tempFilepath.c_str(), doc, "UTF-8", 1)!=-1) {
      break;
    }
    usleep(core->getFileOpenForWritingWaitTime());
  }
  bool errorOccured=false;
  if (tries>=core->getFileOpenForWritingRetries()) {
    ERROR("can not write gpx file <%s>",tempFilepath.c_str());
    errorOccured=true;
  } else {

    rename(tempFilepath.c_str(),filepath.c_str());
    //DEBUG("path storing is complete",NULL);

    // Update the cache
    writeCache();
  }

  // Cleanup
  xmlFreeDoc(doc);
  if (!errorOccured) {
    lockAccess(__FILE__, __LINE__);
    if (this->mapPositions.size()==mapPositions.size()) {
      isStored=true;
    }
    unlockAccess();
  }
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

// Extracts information about the path from the given node set
void NavigationPath::extractInformation(std::list<XMLNode> nodes) {
  std::string t;
  for(std::list<XMLNode>::iterator i=nodes.begin();i!=nodes.end();i++) {
    XMLNode node=*i;
    std::string nodeName=(char*)node->name;
    if (nodeName=="name") {
      if (ConfigSection::getNodeText(node,t)) name=t;
    }
    if (nodeName=="desc") {
      if (ConfigSection::getNodeText(node,t)) description=t;
    }
  }
}

// Writes the cache to a file
void NavigationPath::writeCache() {
  std::string filepath=gpxFilefolder + "/" + gpxFilename;
  std::string cacheFilepath=gpxCacheFilefolder + "/" + gpxFilename;
  //DEBUG("cacheFilepath=%s",cacheFilepath.c_str());
  std::ofstream ofs;
  ofs.open(cacheFilepath.c_str(), std::ios::binary);
  if (ofs.fail()) {
    WARNING("can not open <%s> for writing", cacheFilepath.c_str());
    remove(cacheFilepath.c_str());
  } else {
    store(&ofs);
    if (ofs.bad()) {
      WARNING("can not store object into <%s>", cacheFilepath.c_str());
      remove(cacheFilepath.c_str());
    }
    ofs.close();
  }
}

// Reads the path contents from a gpx file
bool NavigationPath::readGPXFile() {

  std::string filepath=gpxFilefolder + "/" + gpxFilename;
  std::string cacheFilepath = gpxCacheFilefolder + "/" + gpxFilename;
  //DEBUG("cacheFilepath=%s",cacheFilepath.c_str());
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
  std::list<XMLNode> waypointNodes;
  bool loadTrack=false;
  bool loadRoute=false;
  MapPosition pos;
  bool result=false;
  std::list<std::string> status;
  std::stringstream progress;
  std::ofstream ofs;
  std::ifstream ifs;
  bool cacheRetrieved;
  struct stat fileStat,cacheStat;

  // Check if we can use the cache
  cacheRetrieved=false;
  if (((core->statFile(filepath,&fileStat))==0)&&(core->statFile(cacheFilepath,&cacheStat)==0)) {

    // Is the cache newer than the file?
    bool isOlder=false;
    if (cacheStat.st_mtime<fileStat.st_mtime) {
      isOlder=true;
    }
    if (!isOlder) {

      // Load the cache
      status.push_back("Loading path from cache (init):");
      status.push_back(getGpxFilename());
      core->getNavigationEngine()->setStatus(status, __FILE__, __LINE__);
      ifs.open(cacheFilepath.c_str(),std::ios::binary);
      if (ifs.fail()) {
        remove(cacheFilepath.c_str());
        WARNING("can not open <%s> for reading",cacheFilepath.c_str());
      } else {

        // Load the complete file into memory
        char *cacheData;
        if (!(cacheData=(char*)malloc(cacheStat.st_size+1))) {
          FATAL("can not allocate memory for cache",NULL);
          return false;
        }
        ifs.read(cacheData,cacheStat.st_size);
        ifs.close();
        cacheData[cacheStat.st_size]=0; // to prevent that strings never end
        Int cacheSize=cacheStat.st_size;

        // Retrieve the objects
        char *cacheData2=cacheData;
        bool success = NavigationPath::retrieve(this,cacheData2,cacheSize);
        if ((cacheSize!=0)||(!success)) {
          if (!core->getQuitCore()) {
            remove(cacheFilepath.c_str());
            WARNING("falling back to full gpx file read because cache is corrupted",NULL);
          }
        } else {
          cacheRetrieved=true;
        }
        free(cacheData);
      }
    }
  }

  // Load the gpx file if cache can not be used
  if ((!cacheRetrieved)&&(!core->getQuitCore())) {

    // Read the document
    status.push_back("Loading path (Init):");
    status.push_back(getGpxFilename());
    core->getNavigationEngine()->setStatus(status, __FILE__, __LINE__);
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
      loadTrack=true;
    } else {
      if (routeNodes.size()>0) {
        loadRoute=true;
      } else {
        ERROR("file <%s> does neither contain a route nor a track",gpxFilename.c_str());
      }
    }

    // Extract data from the metadata section if it exists
    if (GPX11) {
      nodes=findNodes(doc,xpathCtx,"/gpx:gpx/gpx:metadata/*");
      lockAccess(__FILE__, __LINE__);
      extractInformation(nodes);
      unlockAccess();
    }
    if (GPX10) {
      nodes=findNodes(doc,xpathCtx,"/gpx:gpx/*");
      lockAccess(__FILE__, __LINE__);
      extractInformation(nodes);
      unlockAccess();
    }

    // Load the points of the path
    if (loadTrack) {
      nodes=findNodes(doc,xpathCtx,"/gpx:gpx/gpx:trk/gpx:trkseg/gpx:trkpt");
      totalNumberOfPoints=nodes.size();
      processedPoints=0;
      for (Int k=1;k<=trackNodes.size();k++) {
        std::stringstream prefix;
        prefix << "/gpx:gpx/gpx:trk["<<k<<"]";
        if (trackNodes.size()==1) {
          nodes=findNodes(doc,xpathCtx,prefix.str() + "/*");
          extractInformation(nodes);
        }
        nodes=findNodes(doc,xpathCtx,prefix.str() + "/gpx:trkseg");
        numberOfSegments=nodes.size();
        for(Int i=1;i<=numberOfSegments;i++) {
          std::stringstream trksegPath;
          trksegPath<<prefix.str() + "/gpx:trkseg["<<i<<"]/gpx:trkpt";
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
            if (processedPercentage>prevProcessedPercentage) {
              progress.str(""); progress << "Loading path (" << processedPercentage << "%):";
              status.pop_front();
              status.push_front(progress.str());
              core->getNavigationEngine()->setStatus(status, __FILE__, __LINE__);
            }
            prevProcessedPercentage=processedPercentage;
            if (core->getQuitCore())
              goto cleanup;
            //core->getThread()->reschedule();
          }
          if (i!=numberOfSegments) {
            addEndPosition(NavigationPath::getPathInterruptedPos());
          }
          if (core->getQuitCore())
            break;
        }
        if (k!=trackNodes.size()) {
          addEndPosition(NavigationPath::getPathInterruptedPos());
        }
      }
    }
    if (loadRoute) {
      nodes=findNodes(doc,xpathCtx,"/gpx:gpx/gpx:rte/gpx:rtept");
      totalNumberOfPoints=nodes.size();
      processedPoints=0;
      for (Int k=1;k<=routeNodes.size();k++) {
        std::stringstream prefix;
        prefix << "/gpx:gpx/gpx:rte["<<k<<"]";
        nodes=findNodes(doc,xpathCtx,prefix.str() + "/*");
        if (routeNodes.size()==1) {
          extractInformation(nodes);
        }
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
          if (processedPercentage>prevProcessedPercentage) {
            progress.str(""); progress << "Loading path (" << processedPercentage << "%):";
            status.pop_front();
            status.push_front(progress.str());
            core->getNavigationEngine()->setStatus(status, __FILE__, __LINE__);
          }
          prevProcessedPercentage=processedPercentage;
          if (core->getQuitCore())
            break;
          //core->getThread()->reschedule();
        }
        if (k!=routeNodes.size()) {
          addEndPosition(NavigationPath::getPathInterruptedPos());
        }
      }
    }

    // Load wayoints if gpx contains them
    waypointNodes=findNodes(doc,xpathCtx,"/gpx:gpx/gpx:wpt");
    totalNumberOfPoints=waypointNodes.size();
    std::string configPath="Navigation/Route[@name='" + getGpxFilename() + "']";
    importWaypoints=(NavigationPatImportWaypointsType)core->getConfigStore()->getIntValue(configPath,"importWaypoints",__FILE__,__LINE__);
    if (importWaypoints==NavigationPathImportWaypointsUndecided) {
      if (totalNumberOfPoints==0) {
        importWaypoints=NavigationPathImportWaypointsNo;
        core->getConfigStore()->setIntValue(configPath,"importWaypoints",(Int)importWaypoints,__FILE__,__LINE__);
      } else {
        std::stringstream cmd;
        cmd << "decideWaypointImport(" << getGpxFilename() << "," << totalNumberOfPoints << ")";
        core->getCommander()->dispatch(cmd.str());
      }
    } else {
      if (importWaypoints==NavigationPathImportWaypointsYes) {
        processedPoints=0;
        for(std::list<XMLNode>::iterator i=waypointNodes.begin();i!=waypointNodes.end();i++) {
          XMLNode node=*i;
          std::string error;
          std::stringstream defaultName;
          defaultName << "Waypoint " << processedPoints+1;
          NavigationPoint point;
          if (point.readGPX(node,getGpxFilename(),defaultName.str(),error)) {
            core->getNavigationEngine()->addAddressPoint(point);
          } else {
            if (error!="contains a routing waypoint") {
              error="file <%s> " + error;
              WARNING(error.c_str(),gpxFilename.c_str());
            }
          }
          processedPoints++;
          processedPercentage=processedPoints*100/totalNumberOfPoints;
          if (processedPercentage>prevProcessedPercentage) {
            progress.str(""); progress << "Loading wpts (" << processedPercentage << "%):";
            status.pop_front();
            status.push_front(progress.str());
            core->getNavigationEngine()->setStatus(status, __FILE__, __LINE__);
          }
          prevProcessedPercentage=processedPercentage;
          if (core->getQuitCore())
            break;
          //core->getThread()->reschedule();
        }
      }
    }

    // Write the cache
    if ((!core->getQuitCore())&&(importWaypoints!=NavigationPathImportWaypointsUndecided)) {
      status.pop_front();
      status.push_front("Writing cache");
      core->getNavigationEngine()->setStatus(status, __FILE__, __LINE__);
      writeCache();
    }
  }

  // Cleanup
  if (core->getQuitCore())
    result=false;
  else
    result=true;
cleanup:
  if (xpathCtx) xmlXPathFreeContext(xpathCtx);
  if (doc) xmlFreeDoc(doc);
  lockAccess(__FILE__, __LINE__);
  isStored=true;
  hasChanged=true;
  hasBeenLoaded=true;
  unlockAccess();
  status.clear();
  core->getNavigationEngine()->setStatus(status, __FILE__, __LINE__);
  return result;
}

}
