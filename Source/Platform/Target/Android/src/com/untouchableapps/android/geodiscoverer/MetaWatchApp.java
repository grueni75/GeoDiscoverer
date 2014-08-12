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
import java.util.Calendar;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
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
import android.util.Log;

public class MetaWatchApp {

  // Identifies the meta watch app
  final public static String id = "com.untouchableapps.android.geodiscoverer.MetaWatchApp";
  final static String name = "Geo Discoverer";
  
  // Minimum time that must pass between updates
  static int minUpdatePeriodNormal;
  static int minUpdatePeriodTurn;
  static int offRouteVibrateFastPeriod;
  static int offRouteVibrateFastCount;
  static int offRouteVibrateSlowPeriod;
  
  // Time in milliseconds to sleep before vibrating
  static int waitTimeBeforeVibrate;

  // Last time the watch was updated
  static long lastUpdate;
  
  // Last infos used for updating
  static String lastInfosAsSSV = "";
  
  // Distance to turn
  static String currentTurnDistance="-";
  static String lastTurnDistance="-";
  
  // Off route indication
  static boolean currentOffRoute=false;
  static boolean lastOffRoute=false;  
  
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
  static Bitmap target = null;
  
  // Paints
  static Paint compassPaint = null;
  static Paint filledPaint = null;
  
  // Context
  static Context context;
  
  // Thread that manages vibrations
  static final Lock lock = new ReentrantLock();
  static final Condition triggerVibrate = lock.newCondition();
  static int expectedVibrateCount = 0;
  static boolean quitVibrateThread = false;
  static Thread vibrateThread = null;
  
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

    // Init parameters
    minUpdatePeriodNormal = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("MetaWatch", "minUpdatePeriodNormal"));
    minUpdatePeriodTurn = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("MetaWatch", "minUpdatePeriodTurn"));
    waitTimeBeforeVibrate = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("MetaWatch", "waitTimeBeforeVibrate"));
    offRouteVibrateFastPeriod = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("MetaWatch", "offRouteVibrateFastPeriod"));
    offRouteVibrateFastCount = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("MetaWatch", "offRouteVibrateFastCount"));
    offRouteVibrateSlowPeriod = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("MetaWatch", "offRouteVibrateSlowPeriod"));
    
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

    // Start the vibrate thread
    if ((vibrateThread!=null)&&(vibrateThread.isAlive())) {
      boolean repeat=true;
      while(repeat) {
        repeat=false;
        try {
          lock.lock();
          quitVibrateThread=true;
          triggerVibrate.signal();
          lock.unlock();
          vibrateThread.join();
        }
        catch(InterruptedException e) {
          repeat=true;
        }
      }
    }
    MetaWatchApp.context=context;
    quitVibrateThread=false;
    vibrateThread = new Thread(new Runnable() {
      public void run() {
        int currentVibrateCount = 0;
        int fastVibrateCount = 1;
        while (!quitVibrateThread) {
          try {
            lock.lock();
            if (currentVibrateCount==expectedVibrateCount)
              triggerVibrate.await();
            lock.unlock();
            if (quitVibrateThread)
              return;
            do {
                            
              // Ensure that the app is shown
              Intent intent = new Intent("org.metawatch.manager.APPLICATION_START");
              Bundle b = new Bundle();
              b.putString("id", id);
              b.putString("name", name);
              intent.putExtras(b);
              MetaWatchApp.context.sendBroadcast(intent);
              if (MetaWatchApp.bitmap!=null) {
                intent = new Intent("org.metawatch.manager.APPLICATION_UPDATE");
                b = new Bundle();
                b.putString("id", id);
                b.putIntArray("array", makeSendableArray(MetaWatchApp.bitmap));
                intent.putExtras(b);
                MetaWatchApp.context.sendBroadcast(intent);
              }
              
              // Wait a little bit before vibrating
              Thread.sleep(waitTimeBeforeVibrate);
              if (quitVibrateThread)
                return;

              // Skip vibrate if we are not off route anymore and this is 
              // is not the first vibrate
              if ((!currentOffRoute)&&(fastVibrateCount>1))
                break;
              
              // Vibrate
              intent = new Intent("org.metawatch.manager.VIBRATE");
              b = new Bundle();
              b.putInt("vibrate_on", 500);
              b.putInt("vibrate_off", 500);
              b.putInt("vibrate_cycles", 2);
              intent.putExtras(b);
              MetaWatchApp.context.sendBroadcast(intent);
              GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDMetaWatch",String.format("currentOffRoute=%d fastVibrateCount=%d currentVibrateCount=%d expectedVibrateCount=%d", currentOffRoute ? 1 : 0, fastVibrateCount, currentVibrateCount, expectedVibrateCount));
              
              // Repeat if off route 
              // Vibrate fast at the beginning, slow afterwards
              // Quit if a new vibrate is requested or we are on route again
              if (currentOffRoute) {
                int offRouteVibratePeriod;
                if (fastVibrateCount>offRouteVibrateFastCount) {
                  offRouteVibratePeriod=offRouteVibrateSlowPeriod;
                } else {
                  offRouteVibratePeriod=offRouteVibrateFastPeriod;
                  fastVibrateCount++;
                }
                for (int i=0;i<offRouteVibratePeriod/1000;i++) {
                  Thread.sleep(1000);
                  if ((!currentOffRoute)||(currentVibrateCount<expectedVibrateCount-1))
                    break;
                }
              }
              if (quitVibrateThread)
                return;
            }
            while ((currentOffRoute)&&(currentVibrateCount==expectedVibrateCount-1));
            currentVibrateCount++;
            if (!currentOffRoute) 
              fastVibrateCount=1;
          }
          catch(InterruptedException e) {
            ; // repeat
          }
        }
      }
    });   
    vibrateThread.start();
    
    // Inform metawatch about this app
    Intent intent = new Intent("org.metawatch.manager.APPLICATION_ANNOUNCE");
    Bundle b = new Bundle();
    b.putString("id", id);
    b.putString("name", name);
    intent.putExtras(b);
    context.sendBroadcast(intent);
    
  }
  
  public static void start() {
    if (context==null) return;
    Intent intent = new Intent("org.metawatch.manager.APPLICATION_START");
    Bundle b = new Bundle();
    b.putString("id", id);
    b.putString("name", name);
    intent.putExtras(b);
    context.sendBroadcast(intent);
    lastUpdate = 0;
    update(null,true);
    lastUpdate = 0;
  }
  
  private static void drawTriangle(Canvas c, Point p1, Point p2, Point p3) {
    Path path = new Path();
    path.setFillType(Path.FillType.EVEN_ODD);
    path.moveTo(p1.x, p1.y);
    path.lineTo(p2.x, p2.y);
    path.lineTo(p3.x, p3.y);
    path.close();
    c.drawPath(path, filledPaint);
  }
  
  private static void refreshApp(String[] infos) {
    
    float radius,x,y,x2,y2;
        
    // Create a new bitmap
    bitmap = Bitmap.createBitmap(96, 96, Bitmap.Config.RGB_565);
    Canvas c = new Canvas(bitmap); 
    c.drawColor(Color.WHITE);
    
    // Draw the background
    c.drawCircle(48, 48, 42, compassPaint);

    // Obtain the dashboard infos
    float directionBearing=0;
    if (!infos[0].equals("-"))
      directionBearing = Float.parseFloat(infos[0]);
    float targetBearing=0;
    if (!infos[1].equals("-")) {
      targetBearing = (Float.parseFloat(infos[1]) - directionBearing);
    }
    
    // Draw the direction indicator
    if (infos[6].equals("-"))
      drawTriangle(c,new Point(39,26),new Point(57,26),new Point(48,18));

    // Draw the compass
    //Matrix matrix = new Matrix();
    //matrix.setRotate(-directionBearing,48,48);
    //c.drawBitmap(compass, matrix, null);
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
    
    // Is a turn coming?
    if (!infos[6].equals("-")) {
      
    	// Draw the turn
    	float turnAngle = Float.parseFloat(infos[6]);
    	int mirror=1;
    	if (turnAngle<0) {
    	  turnAngle=-turnAngle;
    	  mirror=-1;
    	}
    	String turnDistance = infos[7];
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
  }
  
  public static void update(String infosAsSSV, boolean forceUpdate) {
    
    if (context==null) return;
    
    // Use the last infos if no infos given
    if (infosAsSSV == null) {
      infosAsSSV = lastInfosAsSSV;
    }
    lastInfosAsSSV = infosAsSSV;

    // Obtain the dashboard infos
    if (infosAsSSV.equals(""))
      return;
    String[] infos = infosAsSSV.split(";");
    currentTurnDistance=infos[7];
    currentOffRoute=false;
    if (infos[5].equals("off route!")) {
      infos[5]="off rte!";
      currentOffRoute=true;
    }
    
    // If the turn has appeared or disapperas, force an update
    if ((currentTurnDistance.equals("-"))&&(!lastTurnDistance.equals("-")))
      forceUpdate=true;
    if ((!currentTurnDistance.equals("-"))&&(lastTurnDistance.equals("-")))
      forceUpdate=true;
    if (lastOffRoute!=currentOffRoute) 
      forceUpdate=true;
    
    // Check if the info is updated too fast
    long minUpdatePeriod;
    if (currentTurnDistance.equals("-"))
      minUpdatePeriod = minUpdatePeriodNormal;
    else
      minUpdatePeriod = minUpdatePeriodTurn;
    long t = Calendar.getInstance().getTimeInMillis();
    long diffToLastUpdate = t - lastUpdate;
    if ((!forceUpdate)&&(diffToLastUpdate < minUpdatePeriod)) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDMetaWatch", "Skipped update because last update was too recent");
      return;
    }
        
    // Draw the bitmap
    refreshApp(infos);
    
    // Vibrate if the turn appears the first time or if off route
    boolean vibrate=false;
    if (((!lastOffRoute)&&currentOffRoute)) {
      vibrate=true;
    }
    if ((!currentTurnDistance.equals("-"))&&(lastTurnDistance.equals("-"))) {
      vibrate=true;
    }
    if (vibrate) {
            
      // Inform the vibrate thread to do the work
      lock.lock();
      expectedVibrateCount++;
      triggerVibrate.signal();
      lock.unlock();
      
    } else {

      // Send the bitmap directly
      Intent intent = new Intent("org.metawatch.manager.APPLICATION_UPDATE");
      Bundle b = new Bundle();
      b.putString("id", id);
      b.putIntArray("array", makeSendableArray(bitmap));
      intent.putExtras(b);
      context.sendBroadcast(intent);
      
    }

    // Remember when was updated
    lastUpdate = Calendar.getInstance().getTimeInMillis();
    lastTurnDistance=currentTurnDistance;
    lastOffRoute=currentOffRoute;
  }
  
  public static void stop() {
    if (context==null) return;
    Intent intent = new Intent("org.metawatch.manager.APPLICATION_STOP");
    Bundle b = new Bundle();
    b.putString("id", id);
    intent.putExtras(b);
    context.sendBroadcast(intent);
    bitmap = null;
  }
  
  public static void button(int button, int type) {
    if (context==null) return;
    //update(context,"");
  }

}
