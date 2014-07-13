//============================================================================
// Name        : GDNavDrawerAdapter.java
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

import java.util.ArrayList;
import android.annotation.SuppressLint;
import android.content.Context;
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

  private final ArrayList<GDNavDrawerItem> entries;
  private final LayoutInflater inflater;

  public GDNavDrawerAdapter(Context context, ArrayList<GDNavDrawerItem> entries) {
      super(context, R.layout.nav_drawer_item, entries);
      this.inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
      this.entries = entries;
  }

  @Override
  public View getView(int position, View convertView, final ViewGroup parent) {
      // todo: implement view holder design pattern to speed up list view
      View rowView = null;
      GDNavDrawerItem item = entries.get(position);
      if (item.legendBitmap==null) {
        rowView = inflater.inflate(R.layout.nav_drawer_item, parent, false);
        ImageView iconView = (ImageView) rowView.findViewById(R.id.nav_drawer_item_icon);
        TextView titleView = (TextView) rowView.findViewById(R.id.nav_drawer_item_title);
        iconView.setImageResource(item.icon);
        titleView.setText(item.title);
      } else {
        rowView = inflater.inflate(R.layout.nav_drawer_legend, parent, false);
        ScrollView verticalScrollView = (ScrollView) rowView.findViewById(R.id.nav_drawer_legend_vertical_scroller);
        ImageView imageView = (ImageView) rowView.findViewById(R.id.nav_drawer_legend_image);
        imageView.setImageBitmap(item.legendBitmap);
        verticalScrollView.setOnTouchListener(new OnTouchListener() {
          @SuppressLint("ClickableViewAccessibility")
          public boolean onTouch(View v, MotionEvent event) {
            v.getParent().requestDisallowInterceptTouchEvent(true);
            return false;
          }
        });
      }
      return rowView;
  }
}

