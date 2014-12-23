//============================================================================
// Name        : GDNavDrawerItem.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
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
  public static final int ID_SEND_LOGS = 9;
  public static final int ID_DONATE = 10;
  
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
