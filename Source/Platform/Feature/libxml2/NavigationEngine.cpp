//============================================================================
// Name        : NavigationEngine.cpp
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
#include <NavigationEngine.h>
#include <Commander.h>

namespace GEODISCOVERER {

// Searches for a node with the given name in the given list of nodes
XMLNode NavigationEngine::findNode(XMLNode node, const char *name) {
  while (node!=NULL) {
    if ((node->name!=NULL)&&(node->children!=NULL)) {
      if (strcmp((const char *)node->name,name)==0) {
        return node;
      }
    }
    node=node->next;
  }
  return node;
}

// Synchronizes Google Maps bookmarks with address points
void NavigationEngine::synchronizeGoogleBookmarks() {
  
  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundLow);

  // Get important parameters
  std::string apiKey=core->getConfigStore()->getStringValue("GoogleBookmarksSync","placesApiKey",__FILE__,__LINE__);
  if (apiKey=="Please set your API key obtained via Google Developers Console") {
    ERROR("you need to set your Google Places API key in the Preferences to synchronize Google Maps Bookmarks",NULL);
    return;
  }
  std::string group=core->getConfigStore()->getStringValue("GoogleBookmarksSync","group",__FILE__,__LINE__);

  // Do an endless loop
  bool firstRun=true;
  while(!quitSynchronizeGoogleBookmarksThread) {

    // Wait for an compute trigger
    if (!firstRun)
      core->getThread()->waitForSignal(synchronizeGoogleBookmarksSignal);
    firstRun=false;

    // Shall we quit?
    if (quitSynchronizeGoogleBookmarksThread)
      return;    

    // Download the current bookmarks and process them
    DEBUG("updating google bookmarks",NULL);
    std::list<NavigationPoint> newNavigationPoints;
    std::list<std::string> allBookmarkTitles;
    DownloadResult result;
    UInt size;
    std::string cookies=core->getConfigStore()->getStringValue("GoogleBookmarksSync","cookies",__FILE__,__LINE__);
    UByte *data=core->downloadURL("https://www.google.com/bookmarks/?output=xml",result,size,true,false,NULL,&cookies);
    if (data!=NULL) {
      
      // Did we get a XML file?
      if (strncmp((const char *)data,"<?xml version",13)!=0) {
        
        // Ask for updating the cookie
        DEBUG("requesting google bookmarks authentification",NULL);
        core->getCommander()->dispatch("authenticateGoogleBookmarks()");
        continue;
        
      }
      
      // Read the XML file
      XMLDocument doc = xmlReadDoc(data, NULL, "iso-8859-1", 0);
      if (!doc) {
        WARNING("google bookmarks could not be read",NULL);
        free(data);
        continue;
      }
      
      //FATAL("do we need to encode the strings inside the address point?",NULL);
      
      // Go through all bookmarks and create a list of navigation points
      XMLNode bookmark=doc->children;
      if (!bookmark) {
        WARNING("google bookmarks could not be read",NULL);
        xmlFreeDoc(doc);
        free(data);
        continue;
      }
      bookmark=bookmark->children;
      if (!bookmark) {
        WARNING("google bookmarks could not be read",NULL);
        xmlFreeDoc(doc);
        free(data);
        continue;
      }
      bookmark=bookmark->children;
      while (bookmark!=NULL) {
        XMLNode field=bookmark->children;
        std::string title="";
        std::string url="";        
        std::string timestamp="";      
        std::string address="";
        std::string lat="";
        std::string lng="";
        while (field!=NULL) {
          if ((field->name!=NULL)&&(field->children!=NULL)&&(field->children->content!=NULL)) {
            if (strcmp((const char *)field->name,"title")==0) 
              title=(const char *)field->children->content;
            if (strcmp((const char *)field->name,"url")==0) {
              url=(const char *)field->children->content;
              
              // Check if the url contains already coordinates
              DEBUG("url=%s",url.c_str());
              size_t pos;
              if ((url.find("ftid=")==std::string::npos)&&(url.find("cid=")==std::string::npos)) {
                pos=url.find("q=");
                if (pos!=std::string::npos) {
                  url=url.substr(pos+2);
                  pos=url.find(",");
                  lat=url.substr(0,pos);
                  lng=url.substr(pos+1);
                  //DEBUG("lat=%s lng=%s",lat.c_str(),lng.c_str());
                }
              } else {
                pos=url.find("cid=");                
                if (pos!=std::string::npos) {
                  address=url.substr(pos);
                } else {
                  pos=url.find("q=");
                  if (pos!=std::string::npos) {
                    address=url.substr(pos);
                  }
                }
                DEBUG("address: %s",address.c_str());
              }
            }
            if (strcmp((const char *)field->name,"timestamp")==0) 
              timestamp=(const char *)field->children->content;
          }
          field=field->next;
        }
        if ((title=="")||((address=="")&&((lng=="")||(lat=="")))||(timestamp=="")) {
          DEBUG("title=%s url=%s",title.c_str(),url.c_str());
          if (title=="") {
            WARNING("can not import Google Bookmark with unkown title",NULL);
          } else {
            WARNING("can not import Google Bookmark <%s>",title.c_str());
          }
        } else {
        
          // Remember the title for deleting not existing googlemarks later
          allBookmarkTitles.push_back(title);
          
          // Check if this bookmark is not yet known or updated
          NavigationPoint p;
          p.setName(title);
          p.setGroup(group);
          bool addPoint=false;
          if (!p.readFromConfig("Navigation/AddressPoint"))           
            addPoint=true;
          else {
            if (p.getForeignTimestamp()!=timestamp) {
              addPoint=true;
            }
          }
          if (addPoint) {
            
            // If we have the coordinates already, finish this point
            p.setForeignTimestamp(timestamp);
            if (address=="") {
              DEBUG("adding/updating google bookmark <%s>",p.getName().c_str());
              p.setLat(atof(lat.c_str()));
              p.setLng(atof(lng.c_str()));
              p.writeToConfig("Navigation/AddressPoint",0); 
            } else {
              p.setAddress(address);
              newNavigationPoints.push_back(p);
            }            
            DEBUG("adding <%s> for processing",p.getName().c_str());
          } else {
            DEBUG("skipping google bookmark <%s> (already known and up-to-date)",p.getName().c_str());
          }
        }        
        bookmark=bookmark->next;
      }
      xmlFreeDoc(doc);
      free(data);
    }    
    
    // Get more information from each bookmark
    bool addressPointsChanged=false;
    for (std::list<NavigationPoint>::iterator i=newNavigationPoints.begin();i!=newNavigationPoints.end();i++) {
      
      // Find out what type of address it is and set the service to use
      //DEBUG("address=%s",(*i).getAddress().c_str());
      std::string url="";
      if ((*i).getAddress().rfind("q=",0)==0) {
        url+="https://maps.googleapis.com/maps/api/geocode/xml?";
        url+="address="+(*i).getAddress().substr(2);                
      }
      if ((*i).getAddress().rfind("cid=",0)==0) {
        url+="https://maps.googleapis.com/maps/api/place/details/xml?";
        url+=(*i).getAddress();        
      }
      
      // Get the result of the service
      url+="&key="+apiKey;      
      //DEBUG("url=%s",url.c_str());
      UByte *data=core->downloadURL(url,result,size,true,false);
      if (data!=NULL) {
        
        // Read the XML file
        XMLDocument doc = xmlReadDoc(data, NULL, NULL, 0);
        if (!doc) {
          WARNING("google place with cid <%s> could not be read",(*i).getAddress().c_str());
          free(data);
          continue;
        }

        // Extract details from the google place
        XMLNode result=doc->children;
        if (!result) {
          WARNING("google place for bookmark <%s> could not be read",(*i).getName().c_str());
          xmlFreeDoc(doc);
          free(data);
          continue;
        }        
        result=findNode(result->children,"result");        
        if (!result) {
          WARNING("google place for bookmark <%s> could not be read",(*i).getName().c_str());
          xmlFreeDoc(doc);
          free(data);
          continue;
        }
        XMLNode element=findNode(result->children,"formatted_address");        
        if ((element)&&(element->children)&&(element->children->content)) {
          (*i).setAddress((const char *)element->children->content);
        }
        result=findNode(result->children,"geometry");        
        if (!result) {
          WARNING("google place for bookmark <%s> could not be read",(*i).getName().c_str());
          xmlFreeDoc(doc);
          free(data);
          continue;
        }
        result=findNode(result->children,"location");        
        std::stringstream value;
        bool latSet=false;
        bool lngSet=false;
        while (result!=NULL) {
          element=result->children;
          while (element!=NULL) {
            if ((element->name!=NULL)&&(element->children!=NULL)&&(element->children->content!=NULL)) {
              if (strcmp((const char *)element->name,"lat")==0) {
                (*i).setLat(atof((const char *)element->children->content));
                latSet=true;
              }
              if (strcmp((const char *)element->name,"lng")==0) {
                (*i).setLng(atof((const char *)element->children->content));
                lngSet=true;
              }
            }
            element=element->next;
          }
          result=result->next;
        }
        xmlFreeDoc(doc);
        free(data);
        if ((!lngSet)||(!latSet)) {
          WARNING("google place for bookmark <%s> does not contain latitude and longitude",(*i).getName().c_str());
          continue;
        }
        
        // Store the new bookmark
        DEBUG("adding/updating google bookmark <%s>",(*i).getName().c_str());
        (*i).writeToConfig("Navigation/AddressPoint",0);     
        addressPointsChanged=true;
      }          
    }
    
    // Go through all existing bookmarks
    std::string path = "Navigation/AddressPoint";
    std::list<std::string> names = core->getConfigStore()->getAttributeValues(path,"name",__FILE__,__LINE__);
    for (std::list<std::string>::iterator i=names.begin();i!=names.end();i++) {
      
      // Check if the address point is a google bookmark
      //DEBUG("name=%s",(*i).c_str());
      NavigationPoint p;
      p.setName(*i);
      if (!p.readFromConfig("Navigation/AddressPoint")) 
        FATAL("can not read existing address point",NULL);
      if (p.getForeignTimestamp()=="0")
        continue;
        
      // Now check if this one is in the current list from google
      bool found=false;
      for (std::list<std::string>::iterator j=allBookmarkTitles.begin();j!=allBookmarkTitles.end();j++) {
        if (*i==*j) {
          found=true;
          break;
        }
      }
      
      // Remove it if it's not in the list
      if (!found) {
        DEBUG("removing google bookmark <%s>",(*i).c_str());
        std::string path = "Navigation/AddressPoint[@name='" + *i + "']";
        core->getConfigStore()->removePath(path);
        addressPointsChanged=true;
      }
    }
    
    // Trigger an update if necessary
    if (addressPointsChanged) {
      core->getNavigationEngine()->initAddressPoints();
    }
  }
}

}

