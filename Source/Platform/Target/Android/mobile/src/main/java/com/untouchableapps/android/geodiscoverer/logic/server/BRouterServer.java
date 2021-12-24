//============================================================================
// Name        : BRouterServer.java
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

package com.untouchableapps.android.geodiscoverer.logic.server;

import com.untouchableapps.android.geodiscoverer.GDApplication;
import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;
import com.untouchableapps.android.geodiscoverer.core.GDCore;

import btools.server.RouteServer;

public class BRouterServer {

  GDCore coreObject=null;
  String brouterPath=null;
  Thread brouterServerThread=null;
  boolean quitBrouterServerThread=false;

  /** Constructor */
  public BRouterServer(GDCore coreObject) {
    super();
    this.coreObject=coreObject;
    this.brouterPath=coreObject.homePath+"/Server/BRouter/Backend";
  }

  /** Starts the tile server */
  public boolean start() {

    quitBrouterServerThread=false;
    brouterServerThread = new Thread() {
      @Override
      public void run() {
        super.run();
        try {
          coreObject.setThreadPriority(3);
          String args[] = new String[] {
              brouterPath+"/Segments4",
              brouterPath+"/Profiles2",
              brouterPath+"/CustomProfiles",
              "17777",
              "1"
          };
          RouteServer.main(args);
        }
        catch (Exception e) {
          if (!quitBrouterServerThread)
            GDApplication.addMessage(GDAppInterface.FATAL_MSG,"GDApp",e.getMessage());
        }
      }
    };
    brouterServerThread.start();
    return true;
  }

  /** Stops the tile server */
  public void stop() {
    if (brouterServerThread!=null) {
      quitBrouterServerThread=true;
      brouterServerThread.interrupt();
      brouterServerThread=null;
    }
  }

}
