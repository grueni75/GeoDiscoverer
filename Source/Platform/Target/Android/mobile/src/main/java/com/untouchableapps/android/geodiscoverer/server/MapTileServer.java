//============================================================================
// Name        : MapTileServer.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2021 Matthias Gruenewald
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

package com.untouchableapps.android.geodiscoverer.server;

import com.untouchableapps.android.geodiscoverer.GDApplication;
import com.untouchableapps.android.geodiscoverer.core.GDCore;

import org.mapsforge.map.android.graphics.AndroidGraphicFactory;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.List;

import fi.iki.elonen.NanoHTTPD;

public class MapTileServer {

  GDCore coreObject=null;
  MapTileServerHandler server=null;

  /** Constructor */
  public MapTileServer(GDApplication app, GDCore coreObject) {
    super();
    this.coreObject=coreObject;
    AndroidGraphicFactory.createInstance(app);
  }

  /** Starts the tile server */
  public boolean start() {

    // Find all available maps
    String mapsPath=coreObject.homePath+"/Server/Mapsforge/Map";
    File mapsDir = new File(mapsPath);
    File[] children = mapsDir.listFiles();
    List<File> mapFiles = new ArrayList<File>();
    if (children != null) {
      for (File child : children) {
        if ((child.isFile())&&(child.getPath().endsWith(".map"))) {
          mapFiles.add(child);
        }
      }
    }
    if (mapFiles.size()==0) {
      coreObject.executeAppCommand("errorDialog(\"No maps installed in <" + mapsPath + ">!\")");
      return false;
    }

    // Get the global options
    String language=coreObject.configStoreGetStringValue("MapTileServer","language");
    Integer port=Integer.valueOf(coreObject.configStoreGetStringValue("MapTileServer","port"));
    float userScale=Float.valueOf(coreObject.configStoreGetStringValue("MapTileServer","userScale"));
    float textScale=Float.valueOf(coreObject.configStoreGetStringValue("MapTileServer","textScale"));
    //int workerCount=Integer.valueOf(coreObject.configStoreGetStringValue("MapTileServer","workerCount"));
    int workerCount=1;

    // Create the mapsforger instance
    server = null;
    try {
      server = new MapTileServerHandler(coreObject, port, workerCount, mapFiles, language, userScale, textScale);
    }
    catch (FileNotFoundException e) {
      coreObject.executeAppCommand("errorDialog(\"" + e.getMessage() + "\")");
      return false;
    }

    // Create the tile server
    try {
      server.start(NanoHTTPD.SOCKET_READ_TIMEOUT, false);
    } catch (Exception e) {
      coreObject.executeAppCommand("errorDialog(\"" + e.getMessage() + "\")");
      return false;
    }
    return true;
  }

  /** Stops the tile server */
  public void stop() {
    if (server!=null) {
      try {
        server.stop();
      } catch(Exception e) {
        coreObject.executeAppCommand("errorDialog(\"" + e.getMessage() + "\")");
      }
      server=null;
    }
  }
}
