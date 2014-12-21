//============================================================================
// Name        : GDNavDrawerAdapter.java
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

import java.util.ArrayList;

import android.annotation.SuppressLint;
import android.content.ContentValues;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Color;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ScrollView;
import android.widget.TextView;

/** Generates entries for the navigation drawer */
public class GDNavDrawerAdapter extends ArrayAdapter<GDNavDrawerItem> {

  public final ArrayList<GDNavDrawerItem> entries;
  private final LayoutInflater inflater;
  private String appVersion;
  
  public GDNavDrawerAdapter(Context context, ArrayList<GDNavDrawerItem> entries) {
    super(context, R.layout.nav_drawer_item, entries);
    this.inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    this.entries = entries;
    PackageManager packageManager = context.getPackageManager();
    try {
      appVersion = "Version " + packageManager.getPackageInfo(context.getPackageName(), 0).versionName;
    }
    catch(NameNotFoundException e) {
      appVersion = "Version ?";
    }
  }
  
  // Tell the view recycler that we have two different row layouts
  @Override
  public int getItemViewType(int position) {
    GDNavDrawerItem item = entries.get(position);
    if (item.id==GDNavDrawerItem.ID_APP_INFO) 
      return 0;
    else
      return 1;
  }
  @Override
  public int getViewTypeCount() {
      return 2;
  }

  @Override
  public View getView(int position, View convertView, final ViewGroup parent) {
    GDNavDrawerItem item;
    if (convertView==null) {
      item = entries.get(position);
      if (item.id!=GDNavDrawerItem.ID_APP_INFO) {
        convertView = inflater.inflate(R.layout.nav_drawer_item, parent, false);
        item.iconView = (ImageView) convertView.findViewById(R.id.nav_drawer_item_icon);
        item.titleView = (TextView) convertView.findViewById(R.id.nav_drawer_item_title);
      } else {
        convertView = inflater.inflate(R.layout.nav_drawer_app_info, parent, false);
        item.appVersionView = (TextView) convertView.findViewById(R.id.nav_drawer_app_version);
      }
      convertView.setTag(item);
    } else {
      item=(GDNavDrawerItem) convertView.getTag();
    }
    if (item.id!=GDNavDrawerItem.ID_APP_INFO) {
      item.iconView.setImageResource(item.icon);
      item.titleView.setText(item.title);
    } else {
      item.appVersionView.setText(appVersion);
    }
    return convertView;
  }
  
}

