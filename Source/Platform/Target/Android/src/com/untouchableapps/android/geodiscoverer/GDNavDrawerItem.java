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

import android.widget.ImageView;
import android.widget.TextView;

/** Holds the data of an item in the navigation drawer */
public class GDNavDrawerItem {

  // Choices for IDs
  public static final int ID_PREFERENCES = 0;
  public static final int ID_RESET = 1;
  public static final int ID_EXIT = 2;
  public static final int ID_TOGGLE_MESSAGES = 3;
  public static final int ID_ADD_TRACKS_AS_ROUTES = 4;
  public static final int ID_REMOVE_ROUTES = 5;
  public static final int ID_SHOW_LEGEND = 6;
  public static final int ID_APP_INFO = 7;
  public static final int ID_SHOW_HELP = 8;
  
  // Fields
  int id;
  int icon;
  String title;
  
  // Views
  ImageView iconView=null;
  TextView titleView=null;
  TextView appVersionView=null;
  
  // Constructor
  public GDNavDrawerItem(int id, int icon, String title) {
    super();
    this.id=id;
    this.icon=icon;
    this.title=title;
  }
}
