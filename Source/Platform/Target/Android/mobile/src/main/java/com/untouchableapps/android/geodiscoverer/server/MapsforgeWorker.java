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

import android.os.Process;

import com.untouchableapps.android.geodiscoverer.GDApplication;
import com.untouchableapps.android.geodiscoverer.core.GDAppInterface;

import org.mapsforge.core.graphics.Bitmap;
import org.mapsforge.core.graphics.TileBitmap;
import org.mapsforge.core.model.Tile;
import org.mapsforge.core.util.Parameters;
import org.mapsforge.map.android.graphics.AndroidGraphicFactory;
import org.mapsforge.map.datastore.MultiMapDataStore;
import org.mapsforge.map.layer.renderer.DirectRenderer;
import org.mapsforge.map.layer.renderer.RendererJob;
import org.mapsforge.map.model.DisplayModel;
import org.mapsforge.map.reader.MapFile;
import org.mapsforge.map.rendertheme.ExternalRenderTheme;
import org.mapsforge.map.rendertheme.InternalRenderTheme;
import org.mapsforge.map.rendertheme.XmlRenderTheme;
import org.mapsforge.map.rendertheme.XmlRenderThemeMenuCallback;
import org.mapsforge.map.rendertheme.XmlRenderThemeStyleLayer;
import org.mapsforge.map.rendertheme.XmlRenderThemeStyleMenu;
import org.mapsforge.map.rendertheme.rule.RenderThemeFuture;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.List;
import java.util.Set;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class MapsforgeWorker {

  // Locks and signals
  static final Lock lock = new ReentrantLock();
  static final Condition updated = lock.newCondition();

  // Objects required for rendering
  protected final int tileRenderSize = 256;
  String themeFilePath=null;
  String themeFileStyle=null;
  String themeFileOverlays=null;
  String[] themeFileOverlaysArray=null;
  MultiMapDataStore multiMapDataStore=null;
  DisplayModel displayModel=null;
  DirectRenderer directRenderer=null;
  XmlRenderTheme xmlRenderTheme=null;
  RenderThemeFuture renderThemeFuture=null;
  float textScale;
  boolean busy=false;
  String mapsforgePath=null;
  String preferredLanguage=null;

  // Constructor
  MapsforgeWorker(String mapsforgePath, List<File> mapFiles, float userScale, float textScale, String preferredLanguage) {
    multiMapDataStore = new MultiMapDataStore(MultiMapDataStore.DataPolicy.RETURN_ALL);
    for (File mapFile : mapFiles) {
      multiMapDataStore.addMapDataStore(new MapFile(mapFile, preferredLanguage), true, true);
    }
    displayModel = new DisplayModel();
    displayModel.setUserScaleFactor(userScale);
    directRenderer = new DirectRenderer(multiMapDataStore, AndroidGraphicFactory.INSTANCE, true, null);
    this.textScale = textScale;
    this.mapsforgePath = mapsforgePath;
    this.preferredLanguage = preferredLanguage;
  }

  // Indicates that this worker is not working anymore
  void resetBusy() {
    MapsforgeWorker.lock.lock();
    busy=false;
    MapsforgeWorker.updated.signal();
    MapsforgeWorker.lock.unlock();
  }

  // Update the render theme
  boolean updateRenderThemeFuture(String themeFilePath, String themeFileStyle, String themeFileOverlays) {

    // Nothing changed?
    boolean updateTheme=false;
    if ((this.themeFilePath!=null)&&(this.themeFileStyle!=null)&&(this.themeFileOverlays!=null))
      if ((this.themeFilePath.equals(themeFilePath))&&(this.themeFileStyle.equals(themeFileStyle))&&(this.themeFileOverlays.equals(themeFileOverlays))) {
        return true;
      }

    // Update the fields
    this.themeFilePath = themeFilePath;
    this.themeFileStyle = themeFileStyle;
    this.themeFileOverlays = themeFileOverlays;

    // Check if the theme file exists
    File themeFile=new File(mapsforgePath+"/Theme/"+themeFilePath);
    if (!themeFile.isFile()) {
      return false;
    }

    // Generate the overlays to activate
    themeFileOverlaysArray=null;
    if (!themeFileOverlays.equals("")) {
      themeFileOverlaysArray = themeFileOverlays.trim().split(",");
      for (int i = 0; i < themeFileOverlaysArray.length; i++) {
        themeFileOverlaysArray[i] = themeFileOverlaysArray[i].trim();
      }
    }

    // Update the render theme
    if (themeFile == null) {
      xmlRenderTheme = InternalRenderTheme.OSMARENDER;
    } else {
      try {
        XmlRenderThemeMenuCallback callBack = new XmlRenderThemeMenuCallback() {
          @Override
          public Set<String> getCategories(XmlRenderThemeStyleMenu styleMenu) {
            String id = null;
            if (themeFileStyle != null) {
              id = themeFileStyle;
            } else {
              id = styleMenu.getDefaultValue();
            }

            XmlRenderThemeStyleLayer baseLayer = styleMenu.getLayer(id);
            Set<String> result = baseLayer.getCategories();
            for (XmlRenderThemeStyleLayer overlay : baseLayer.getOverlays()) {
              String overlayId = overlay.getId();
              boolean overlayEnabled = false;
              if (themeFileOverlaysArray == null) {
                overlayEnabled = overlay.isEnabled();
              } else {
                for (int i = 0; i < themeFileOverlaysArray.length; i++) {
                  if (themeFileOverlaysArray[i].equals(overlayId))
                    overlayEnabled = true;
                }
              }
              GDApplication.addMessage(GDAppInterface.DEBUG_MSG, "MapTileServer", "Overlay id=\"" + overlayId + "\" enabled=\"" + Boolean.toString(overlayEnabled)
                  + "\" title=\"" + overlay.getTitle(preferredLanguage) + "\"");

              if (overlayEnabled) {
                result.addAll(overlay.getCategories());
              }
            }
            return result;
          }
        };
        xmlRenderTheme = new ExternalRenderTheme(themeFile, callBack);
      }
      catch (FileNotFoundException e) {
        GDApplication.addMessage(GDAppInterface.DEBUG_MSG,"MapTileServer",e.getMessage());
        return false;
      }
    }

    renderThemeFuture = new RenderThemeFuture(AndroidGraphicFactory.INSTANCE, xmlRenderTheme, displayModel);
    Thread renderThemeFutureThread = new Thread(renderThemeFuture);
    renderThemeFutureThread.setPriority(Process.THREAD_PRIORITY_BACKGROUND);
    renderThemeFutureThread.start();
    return true;
  }

  // Draws a map tile
  Bitmap renderTile(int x, int y, int z) {
    RendererJob job;
    Tile tile = new Tile(x, y, (byte) z, tileRenderSize);
    job = new RendererJob(tile, multiMapDataStore, renderThemeFuture, displayModel, textScale, false, false);
    return (TileBitmap) directRenderer.executeJob(job);
  }

}
