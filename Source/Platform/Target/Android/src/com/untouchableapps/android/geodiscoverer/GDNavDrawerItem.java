//============================================================================
// Name        : GDNavDrawerItem.java
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


package com.untouchableapps.android.geodiscoverer;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

/** Holds the data of an item in the navigation drawer */
public class GDNavDrawerItem {

  int icon;
  String title;
  Bitmap legendBitmap;
  
  public GDNavDrawerItem(int icon, String title) {
    super();
    this.icon=icon;
    this.title=title;
    this.legendBitmap=null;
  }
  
  public GDNavDrawerItem(String legendImageFilename) {
    super();
    this.icon=0;
    this.title=null;
    this.legendBitmap=BitmapFactory.decodeFile(legendImageFilename);
  }
}
