//============================================================================
// Name        : CockpitAppMetaWatch.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

package com.untouchableapps.android.geodiscoverer.cockpit;

import java.io.IOException;
import java.io.InputStream;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.Typeface;
import android.os.Bundle;

import com.untouchableapps.android.geodiscoverer.R;

public class CockpitAppMetaWatch implements CockpitAppInterface {

  // Identifies the meta watch app
  final String id = "com.untouchableapps.android.geodiscoverer.MetaWatchApp";
  final String name = "Geo Discoverer";
  
  // Intent receiver for button events
  BroadcastReceiver metaWatchAppReceiver = null;
  
  // Holds the current bitmap
  Bitmap bitmap = null;
  
  // Handles to the fonts
  Typeface bigFontFace = null;
  Paint bigFontPaint = null;
  int bigFontSize = 16;
  int bigFontRealSize = 11;
  Typeface normalFontFace = null;
  Paint normalFontPaint = null;
  int normalFontSize = 8;
  int normalFontRealSize = 7;
  Typeface smallFontFace = null;
  Paint smallFontPaint = null;
  int smallFontSize = 8;
  int smallFontRealSize = 5;
  
  // Bitmaps
  Bitmap target = null;
  
  // Paints
  Paint compassPaint = null;
  Paint filledPaint = null;
  
  // Context
  Context context;
  
  /** Loads bitmaps from the asset directory */
  Bitmap loadBitmapFromAssets(Context context, String path) {
    try {
      InputStream inputStream = context.getAssets().open(path);
      Bitmap bitmap = BitmapFactory.decodeStream(inputStream);
      inputStream.close();
      return bitmap;
    } catch (IOException e) {
      return null;
    }
  }
  
  /** Creates an array from a bitmap that can be send to metawatch */
  int[] makeSendableArray(final Bitmap bitmap) {
    int pixelArray[] = new int[bitmap.getWidth() * bitmap.getHeight()];
    bitmap.getPixels(pixelArray, 0, bitmap.getWidth(), 0, 0, bitmap.getWidth(), bitmap.getHeight());
    return pixelArray;
  }

  /** Draws a triangle */
  void drawTriangle(Canvas c, Point p1, Point p2, Point p3) {
    Path path = new Path();
    path.setFillType(Path.FillType.EVEN_ODD);
    path.moveTo(p1.x, p1.y);
    path.lineTo(p2.x, p2.y);
    path.lineTo(p3.x, p3.y);
    path.close();
    c.drawPath(path, filledPaint);
  }
  
  /** Constructor */
  public CockpitAppMetaWatch(Context context) {
    super();
    
    // Remember context
    this.context = context;

    // Load bitmaps
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
    filledPaint = new Paint();
    filledPaint.setStyle(Paint.Style.FILL_AND_STROKE);
  }

  /** Inform metawatch about this app */
  public void start() {
    
    // Register metawatch broadcast receiver
    if (metaWatchAppReceiver==null) {
      metaWatchAppReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
          if (intent.getAction()=="org.metawatch.manager.BUTTON_PRESS") {
            inform();
          }
          if (intent.getAction()=="org.metawatch.manager.APPLICATION_DISCOVERY") {
            start();
            inform();
            focus();
          }
        }
      };
      IntentFilter filter = new IntentFilter();
      filter.addAction("org.metawatch.manager.BUTTON_PRESS");
      filter.addAction("org.metawatch.manager.APPLICATION_DISCOVERY");
      context.registerReceiver(metaWatchAppReceiver, filter);
    }
    
    // Tell metawatch that we are there
    update(null);
    Intent intent = new Intent("org.metawatch.manager.APPLICATION_ANNOUNCE");
    Bundle b = new Bundle();
    b.putString("id", id);
    b.putString("name", name);
    intent.putExtras(b);
    context.sendBroadcast(intent);
    inform();
    focus();
  }

  /** Bring the app screen to foreground */
  public void focus() {
    Intent intent = new Intent("org.metawatch.manager.APPLICATION_START");
    Bundle b = new Bundle();
    b.putString("id", id);
    b.putString("name", name);
    intent.putExtras(b);
    context.sendBroadcast(intent);
  }
  
  /** Show the latest bitmap on screen */
  public void inform() {
    if (bitmap!=null) {
      Intent intent = new Intent("org.metawatch.manager.APPLICATION_UPDATE");
      Bundle b = new Bundle();
      b.putString("id", id);
      b.putIntArray("array", makeSendableArray(bitmap));
      intent.putExtras(b);
      context.sendBroadcast(intent);
    }
  }  

  /** Inform the user via vibration */
  public void alert(AlertType type, boolean repeated) {
    Intent intent = new Intent("org.metawatch.manager.VIBRATE");
    Bundle b = new Bundle();
    b.putInt("vibrate_on", 500);
    b.putInt("vibrate_off", 500);
    b.putInt("vibrate_cycles", 2);
    intent.putExtras(b);
    context.sendBroadcast(intent);
  }  
  
  /** Updates the metawatch bitmap with the latest infos (but does not yet show them) */
  public void update(CockpitInfos infos) {
    float radius,x,y,x2,y2;
    
    // Create a new bitmap
    bitmap = Bitmap.createBitmap(96, 96, Bitmap.Config.RGB_565);
    Canvas c = new Canvas(bitmap); 
    c.drawColor(Color.WHITE);
    
    // Draw the background
    c.drawCircle(48, 48, 42, compassPaint);

    // Obtain the dashboard infos
    float directionBearing=0;
    if ((infos!=null)&&(!infos.locationBearing.equals("-")))
      directionBearing = Float.parseFloat(infos.locationBearing);
    float targetBearing=0;
    if ((infos!=null)&&(!infos.targetBearing.equals("-"))) {
      targetBearing = (Float.parseFloat(infos.targetBearing) - directionBearing);
    }
    
    // Draw the direction indicator
    if ((infos==null)||(infos.turnAngle.equals("-")))
      drawTriangle(c,new Point(39,26),new Point(57,26),new Point(48,18));

    // Draw the compass
    //Matrix matrix = new Matrix();
    //matrix.setRotate(-directionBearing,48,48);
    //c.drawBitmap(compass, matrix, null);
    if ((infos!=null)&&(!infos.locationBearing.equals("-"))) {
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
    if ((infos!=null)&&(!infos.targetBearing.equals("-"))) {
      radius = 41;
      x = 48 + radius * (float)Math.sin(Math.toRadians(targetBearing)) - target.getWidth()/2;
      y = 48 - radius * (float)Math.cos(Math.toRadians(targetBearing)) - target.getHeight()/2;
      c.drawBitmap(target, x, y, null);
    }
    
    // Is a turn coming?
    if ((infos!=null)&&(!infos.turnAngle.equals("-"))) {
      
      // Draw the turn
      float turnAngle = Float.parseFloat(infos.turnAngle);
      int mirror=1;
      if (turnAngle<0) {
        turnAngle=-turnAngle;
        mirror=-1;
      }
      String turnDistance = infos.turnDistance;
      float sinOfTurnAngle = (float)Math.sin(Math.toRadians(turnAngle));
      float cosOfTurnAngle = (float)Math.cos(Math.toRadians(turnAngle));
      int turnLineWidth = 12;
      int turnLineArrowOverhang = 6;
      int turnLineArrowHeight = 12;
      int turnLineStartHeight = 17;
      int turnLineMiddleHeight = 7;
      int turnLineStartX = 48;
      int turnLineStartY = 56;
      Point p1 = new Point(turnLineStartX-turnLineWidth/2,turnLineStartY);
      Point p2 = new Point(turnLineStartX+turnLineWidth/2,turnLineStartY);
      Point p3 = new Point(
          p2.x,
          p2.y-turnLineStartHeight);
      Point p4 = new Point(
          p1.x,
          p1.y-turnLineStartHeight);
      Point pm;
      if (mirror>0) {
        pm=p4;
      } else {
        pm=p3;
      }
      Point p5 = new Point(
          pm.x+mirror*(int)(turnLineWidth*cosOfTurnAngle),
          pm.y-(int)(turnLineWidth*sinOfTurnAngle));
      Point p10 = new Point(
          pm.x+mirror*(int)(turnLineWidth/2*cosOfTurnAngle),
          pm.y-(int)(turnLineWidth/2*sinOfTurnAngle));
      p10.x = p10.x-mirror*(int)((turnLineMiddleHeight+turnLineArrowHeight)*sinOfTurnAngle);
      p10.y = p10.y-(int)((turnLineMiddleHeight+turnLineArrowHeight)*cosOfTurnAngle);
      Point p6 = new Point(
          p5.x-mirror*(int)(turnLineMiddleHeight*sinOfTurnAngle),
          p5.y-(int)(turnLineMiddleHeight*cosOfTurnAngle));
      Point p7 = new Point(
          p6.x-mirror*(int)(turnLineWidth*cosOfTurnAngle),
          p6.y+(int)(turnLineWidth*sinOfTurnAngle));
      Point p8 = new Point(
          p7.x-mirror*(int)(turnLineArrowOverhang*cosOfTurnAngle),
          p7.y+(int)(turnLineArrowOverhang*sinOfTurnAngle));
      Point p9 = new Point(
          p6.x+mirror*(int)(turnLineArrowOverhang*cosOfTurnAngle),
          p6.y-(int)(turnLineArrowOverhang*sinOfTurnAngle));
      drawTriangle(c,p1,p2,p3);
      drawTriangle(c,p3,p1,p4);      
      drawTriangle(c,p4,p3,p5);      
      drawTriangle(c,pm,p5,p6);      
      drawTriangle(c,pm,p6,p7);      
      drawTriangle(c,p8,p7,p10);      
      drawTriangle(c,p7,p6,p10);      
      drawTriangle(c,p6,p9,p10);      
      c.drawText(turnDistance,48,70,bigFontPaint);
      
    } else {
    
      // Draw the two lines of information
      x = 48;
      y = 35;
      if (infos!=null) {
        String targetDistance;
        if (infos.offRoute) {
          targetDistance=infos.routeDistance;
        } else {
          targetDistance=infos.targetDistance;
        }
        if (!infos.targetDistance.equals("-")) {
          c.drawText(context.getString(R.string.distance),x,y,smallFontPaint);
          y += bigFontRealSize+2;
          c.drawText(targetDistance,x,y,bigFontPaint);
          y += smallFontRealSize+7;
        }
      }
      c.drawLine(20, 51, 76, 51, smallFontPaint);
      if ((infos!=null)&&(!infos.targetDuration.equals("-"))) {
        c.drawText(context.getString(R.string.duration),x,y,smallFontPaint);
        y += bigFontRealSize+2;
        c.drawText(infos.targetDuration,x,y,bigFontPaint);
      }
    }
  }
  
  /** Informs the metawatch app that this app is not existing anymore */
  public void stop() {
    
    // Tell metawatch that we are not there anymore
    Intent intent = new Intent("org.metawatch.manager.APPLICATION_STOP");
    Bundle b = new Bundle();
    b.putString("id", id);
    intent.putExtras(b);
    context.sendBroadcast(intent);
    bitmap = null;
    
    // Unregister the event receiver
    context.unregisterReceiver(metaWatchAppReceiver);
    metaWatchAppReceiver=null;
  }

}
