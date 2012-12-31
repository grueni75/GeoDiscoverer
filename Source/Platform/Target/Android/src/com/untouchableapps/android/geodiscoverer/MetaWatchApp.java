//============================================================================
// Name        : MetaWatchApp.java
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

import java.io.IOException;
import java.io.InputStream;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.Typeface;
import android.os.Bundle;

public class MetaWatchApp {

  // Identifies the meta watch app
  final public static String id = "com.untouchableapps.android.geodiscoverer.MetaWatchApp";
  final static String name = "Geo Discoverer";
  
  // Holds the current bitmap
  static Bitmap bitmap = null;
  
  // Handles to the fonts
  static Typeface bigFontFace = null;
  static Paint bigFontPaint = null;
  static int bigFontSize = 16;
  static int bigFontRealSize = 11;
  static Typeface normalFontFace = null;
  static Paint normalFontPaint = null;
  static int normalFontSize = 8;
  static int normalFontRealSize = 7;
  static Typeface smallFontFace = null;
  static Paint smallFontPaint = null;
  static int smallFontSize = 8;
  static int smallFontRealSize = 5;
  
  // Bitmaps
  static Bitmap background = null;
  static Bitmap compass = null;
  static Bitmap target = null;
  
  // Paints
  static Paint compassPaint = null;
  
  static Bitmap loadBitmapFromAssets(Context context, String path) {
    try {
      InputStream inputStream = context.getAssets().open(path);
      Bitmap bitmap = BitmapFactory.decodeStream(inputStream);
      inputStream.close();
      return bitmap;
    } catch (IOException e) {
      return null;
    }
  }
  
  static int[] makeSendableArray(final Bitmap bitmap) {
    int pixelArray[] = new int[bitmap.getWidth() * bitmap.getHeight()];
    bitmap.getPixels(pixelArray, 0, bitmap.getWidth(), 0, 0, bitmap.getWidth(), bitmap.getHeight());
    return pixelArray;
  }

  public static void announce(Context context) {

    // Load bitmaps
    background = loadBitmapFromAssets(context, "MetaWatchApp/background.png");
    compass = loadBitmapFromAssets(context, "MetaWatchApp/compass.png");
    target = loadBitmapFromAssets(context, "MetaWatchApp/target.png");

    // Load fonts
    bigFontFace = Typeface.createFromAsset(context.getAssets(), "MetaWatchApp/metawatch_16pt_11pxl.ttf");
    bigFontPaint = new Paint();
    bigFontPaint.setColor(Color.BLACK);    
    bigFontPaint.setTextSize(bigFontSize); 
    bigFontPaint.setTypeface(bigFontFace);
    bigFontPaint.setTextAlign(Align.CENTER);
    normalFontFace = Typeface.createFromAsset(context.getAssets(), "MetaWatchApp/metawatch_8pt_7pxl_CAPS.ttf");
    normalFontPaint = new Paint();
    normalFontPaint.setColor(Color.BLACK);    
    normalFontPaint.setTextSize(normalFontSize); 
    normalFontPaint.setTypeface(normalFontFace);
    normalFontPaint.setTextAlign(Align.CENTER);
    smallFontFace = Typeface.createFromAsset(context.getAssets(), "MetaWatchApp/metawatch_8pt_5pxl_CAPS.ttf");
    smallFontPaint = new Paint();
    smallFontPaint.setColor(Color.BLACK);    
    smallFontPaint.setTextSize(smallFontSize); 
    smallFontPaint.setTypeface(smallFontFace);
    smallFontPaint.setTextAlign(Align.CENTER);
    
    // Create other paints
    compassPaint = new Paint();
    compassPaint.setColor(Color.BLACK);
    compassPaint.setStrokeWidth(2);
    compassPaint.setStyle(Paint.Style.STROKE);
    
    // Inform metawatch about this app
    Intent intent = new Intent("org.metawatch.manager.APPLICATION_ANNOUNCE");
    Bundle b = new Bundle();
    b.putString("id", id);
    b.putString("name", name);
    intent.putExtras(b);
    context.sendBroadcast(intent);
    
  }
  
  public static void start(Context context) {
    Intent intent = new Intent("org.metawatch.manager.APPLICATION_START");
    Bundle b = new Bundle();
    b.putString("id", id);
    b.putString("name", name);
    intent.putExtras(b);
    context.sendBroadcast(intent);
    update(context);
  }
  
  private static void refreshApp(Context context) {
    
    float radius,x,y,x2,y2;
    
    // Create a new bitmap
    bitmap = Bitmap.createBitmap(96, 96, Bitmap.Config.RGB_565);
    Canvas c = new Canvas(bitmap);
    c.drawColor(Color.WHITE);
    
    // Draw the background
    c.drawBitmap(background,0,0,null);

    // Obtain the dashboard infos
    String t = GDApplication.coreObject.executeCoreCommand("getDashboardInfos()");
    if (t.equals(""))
      return;
    String[] infos = t.split(";");
    float directionBearing=0;
    if (!infos[0].equals("-"))
      directionBearing = Float.parseFloat(infos[0]);
    float targetBearing=0;
    if (!infos[1].equals("-")) {
      targetBearing = (Float.parseFloat(infos[1]) - directionBearing);
    }
    
    // Draw the compass
    //Matrix matrix = new Matrix();
    //matrix.setRotate(-directionBearing,48,48);
    //c.drawBitmap(compass, matrix, null);
    c.drawCircle(48, 48, 42, compassPaint);
    if (!infos[0].equals("-")) {
      float alpha = -directionBearing;
      for (int i=0;i<8;i++) {
        radius = 33;
        x = 48 + radius * (float)Math.sin(Math.toRadians(alpha));
        y = 48 - radius * (float)Math.cos(Math.toRadians(alpha)) + smallFontRealSize/2;
        if (i==0) 
          c.drawText("N", x, y, smallFontPaint);
        if (i==2) 
          c.drawText("E", x, y, smallFontPaint);
        if (i==4) 
          c.drawText("S", x, y, smallFontPaint);
        if (i==6) 
          c.drawText("W", x, y, smallFontPaint);
        radius = 42;
        x = 48 + radius * (float)Math.sin(Math.toRadians(alpha));
        y = 48 - radius * (float)Math.cos(Math.toRadians(alpha));
        radius = 37;
        x2 = 48 + radius * (float)Math.sin(Math.toRadians(alpha));
        y2 = 48 - radius * (float)Math.cos(Math.toRadians(alpha));
        c.drawLine(x, y, x2, y2, compassPaint);
        alpha += 360/8;
      }
    }
    
    // Draw the target
    if (!infos[1].equals("-")) {
      radius = 41;
      x = 48 + radius * (float)Math.sin(Math.toRadians(targetBearing)) - target.getWidth()/2;
      y = 48 - radius * (float)Math.cos(Math.toRadians(targetBearing)) - target.getHeight()/2;
      c.drawBitmap(target, x, y, null);
    }
    
    // Draw the first line
    x = 48;
    y = 35;
    if (!infos[2].equals("-")) {
      c.drawText(infos[2],x,y,smallFontPaint);
      y += bigFontRealSize+2;
      c.drawText(infos[3],x,y,bigFontPaint);
      y += smallFontRealSize+7;
    }
    c.drawLine(20, 51, 76, 51, smallFontPaint);
    if (!infos[4].equals("-")) {
      c.drawText(infos[4],x,y,smallFontPaint);
      y += bigFontRealSize+2;
      c.drawText(infos[5],x,y,bigFontPaint);
    }
  }
  
  public static void update(Context context) {
    refreshApp(context);
    
    Intent intent = new Intent("org.metawatch.manager.APPLICATION_UPDATE");
    Bundle b = new Bundle();
    b.putString("id", id);
    b.putIntArray("array", makeSendableArray(bitmap));
    intent.putExtras(b);

    context.sendBroadcast(intent);
  }
  
  public static void stop(Context context) {
    Intent intent = new Intent("org.metawatch.manager.APPLICATION_STOP");
    Bundle b = new Bundle();
    b.putString("id", id);
    intent.putExtras(b);
    context.sendBroadcast(intent);
    bitmap = null;
  }
  
  public static void button(Context context, int button, int type) {
    update(context);
  }
}
