//============================================================================
// Name        : MapTileServerHandler.java
// Author      : r_x, Thomas Theussing and Contributors, Matthias Gruenewald
// Copyright   : Copyright 2016 r_x
// Copyright   : Copyright 2019-2020 Thomas Theussing and Contributors
// Copyright   : Copyright 2021 Matthias Gruenewald
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

import org.mapsforge.core.graphics.Bitmap;
import org.mapsforge.core.util.Parameters;
import org.mapsforge.map.android.graphics.AndroidGraphicFactory;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.SortedSet;
import java.util.TreeSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import fi.iki.elonen.NanoHTTPD;

public class MapTileServerHandler extends NanoHTTPD {

  // Variables
  protected MapsforgeWorker mapsforgeWorker[];
  protected final List<File> mapFiles;
  protected GDCore coreObject=null;
  protected String mapsforgePath=null;
  private static final Pattern P = Pattern.compile("/(.+)/(\\d+)/(\\d+)/(\\d+)\\.(.*)");

  protected class RuntimeMeasurement {
    String function;
    long accumulatedRuntime=0;
    int executionCount=0;
    public RuntimeMeasurement(String function) {
      this.function=function;
    }
    String getStats() {
      return function+": "+((float)accumulatedRuntime/(float)executionCount)+ " ms";
    }
  }
  HashMap<String, RuntimeMeasurement> runtimeStats = new HashMap<String, RuntimeMeasurement>();

  public MapTileServerHandler(GDCore coreObject, Integer port, int workerCount, List<File> mapFiles, String preferredLanguage, float userScale, float textScale) throws FileNotFoundException {
    super(port);
    //Parameters.NUMBER_OF_THREADS=Runtime.getRuntime().availableProcessors() + 1;
    this.mapFiles = mapFiles;
    this.coreObject = coreObject;
    this.mapsforgePath = coreObject.homePath+"/Server/Mapsforge";

    // CLeanup any left over files
    File hillshadeDir = new File(coreObject.homePath+"/Server/Hillshade");
    File[] children = hillshadeDir.listFiles();
    if (children != null) {
      for (File child : children) {
        if ((child.isFile())&&(child.getName().startsWith("hillshade_"))) {
          child.delete();
        }
      }
    }

    // Init data for all mapsforge threads
    mapsforgeWorker = new MapsforgeWorker[workerCount];
    for (int i=0;i<workerCount;i++) {
      mapsforgeWorker[i]=new MapsforgeWorker(mapsforgePath, mapFiles, userScale, textScale, preferredLanguage);
    };
  }

  protected void addRuntime(String uri, String function, int z, long startTime) {
    String key=String.format("%s@%02d",function,z);
    long runtime=System.currentTimeMillis()-startTime;
    RuntimeMeasurement m;
    if (runtimeStats.containsKey(key)) {
      m=runtimeStats.get(key);
    } else {
      m=new RuntimeMeasurement(key);
      runtimeStats.put(key,m);
    }
    m.accumulatedRuntime+=runtime;
    m.executionCount++;
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"MapTileServer",uri+" ("+String.valueOf(runtime)+" ms)");
    //m.print();
  }

  protected Response serveError(String msg) {
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"MapTileServer",msg);
    return newFixedLengthResponse(Response.Status.BAD_REQUEST, NanoHTTPD.MIME_PLAINTEXT, msg);
  }

  @Override
  public Response serve(IHTTPSession session) {

    // Start time measurement
    long startTime=System.currentTimeMillis();

    // Check if status is expected?
    if (session.getUri().equals("/status.txt")) {
      return newFixedLengthResponse(Response.Status.OK, NanoHTTPD.MIME_PLAINTEXT, "tile server is ready");
    }

    // Check if stats is expected?
    if (session.getUri().equals("/stats.txt")) {
      String stats="";
      SortedSet<String> keys = new TreeSet<>(runtimeStats.keySet());
      for (String key : keys) {
        RuntimeMeasurement m=runtimeStats.get(key);
        stats+=m.getStats()+"\n";
      }
      return newFixedLengthResponse(Response.Status.OK, NanoHTTPD.MIME_PLAINTEXT, stats);
    }

    // Get parameters
    Map<String, List<String>> params = session.getParameters();
    String themeFilePath="",themeFileStyle="",themeFileOverlays="";
    for (Map.Entry<String, List<String>> name : params.entrySet()) {
      if (name.getKey().equals("themePath"))
        themeFilePath=name.getValue().get(0);
      if (name.getKey().equals("themeStyle"))
        themeFileStyle=name.getValue().get(0);
      if (name.getKey().equals("themeOverlays"))
        themeFileOverlays=name.getValue().get(0);
    }

    // Get the tile type and number
    String uri = session.getUri();
    String type;
    int x, y, z;
    Matcher m = P.matcher(session.getUri());
    if (m.matches()) {
      type = m.group(1);
      z = Integer.parseInt(m.group(2));
      x = Integer.parseInt(m.group(3));
      y = Integer.parseInt(m.group(4));
      if (!m.group(5).equals("png")) {
        return serveError("unsupported image format");
      }
    } else {
      return serveError("missing tile type and numbers");
    }

    // Render the tile depending on the type
    if (type.equals("mapsforge")) {

      // Select the thread info to use
      MapsforgeWorker worker=null;
      MapsforgeWorker.lock.lock();
      while (true) {
        boolean found = false;
        for (int i = 0; i < mapsforgeWorker.length; i++) {
          if (!mapsforgeWorker[i].busy) {
            worker = mapsforgeWorker[i];
            worker.busy = true;
            found = true;
            //GDApplication.addMessage(GDApplication.DEBUG_MSG,"MapTileServer","mapsforge worker " + i + " selected");
            break;
          }
        }
        if (found)
          break;
        try {
          //GDApplication.addMessage(GDApplication.DEBUG_MSG,"MapTileServer","waiting for available data structure");
          MapsforgeWorker.updated.await();
        }
        catch (InterruptedException e) {
        }
      }
      MapsforgeWorker.lock.unlock();

      // Update the theme (if required)
      if (!worker.updateRenderThemeFuture(themeFilePath,themeFileStyle,themeFileOverlays)) {
        worker.resetBusy();
        return serveError("render theme error");
      }

      // Draw the tile
      Bitmap tileBitmap=worker.renderTile(x,y,z);
      if (tileBitmap==null) {
        worker.resetBusy();
        return serveError("no bitmap received (no map data at tile?)");
      }
      android.graphics.Bitmap image = AndroidGraphicFactory.getBitmap(tileBitmap);

      // Return it as an PNG image
      ByteArrayOutputStream os = new ByteArrayOutputStream();
      image.compress(android.graphics.Bitmap.CompressFormat.PNG, 100, os);
      InputStream fis = new ByteArrayInputStream(os.toByteArray());
      addRuntime(uri+"?"+session.getQueryParameterString(),"mapsforge",z,startTime);
      worker.resetBusy();
      return newFixedLengthResponse(Response.Status.OK, "image/png", fis, os.size());
    }
    if (type.equals("hillshade")) {

      // Ask the core to do the rendering
      String hillshadeTileFilepath=coreObject.homePath+"/Server/Hillshade/hillshade_"+z+"_"+y+"_"+x+".png";
      coreObject.executeCoreCommand("renderHillshadeTile",String.valueOf(z),String.valueOf(y),String.valueOf(x),hillshadeTileFilepath);

      // Read the png and return it to the caller
      File hillshadeTileFile = new File(hillshadeTileFilepath);
      byte[] hillshadeImage;
      try {
        FileInputStream fin = new FileInputStream(hillshadeTileFile);
        hillshadeImage = new byte[(int)hillshadeTileFile.length()];
        fin.read(hillshadeImage);
        fin.close();
        hillshadeTileFile.delete();
        File hillshadeTileFileAuxXml = new File(hillshadeTileFilepath+".aux.xml");
        hillshadeTileFileAuxXml.delete();
      }
      catch (Exception e) {
        return serveError("hillshade tile can not be read");
      }
      ByteArrayOutputStream os = new ByteArrayOutputStream();
      InputStream fis = new ByteArrayInputStream(hillshadeImage);
      addRuntime(uri,"hillshade",z,startTime);
      return newFixedLengthResponse(Response.Status.OK, "image/png", fis, hillshadeImage.length);
    }
    return serveError("tile type <" + type + "> not supported");
  }
}