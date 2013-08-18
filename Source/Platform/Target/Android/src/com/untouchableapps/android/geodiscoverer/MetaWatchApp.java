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

public class MetaWatchApp {

  // Identifies the meta watch app
  final public static String id = "com.untouchableapps.android.geodiscoverer.MetaWatchApp";
  final static String name = "Geo Discoverer";
  
  // Minimum time that must pass between updates
  static int minUpdatePeriodNormal;
  static int minUpdatePeriodTurn;
  
  // Time in milliseconds to sleep before vibrating
  static int waitTimeBeforeVibrate;

  // Last time the watch was updated
  static long lastUpdate;
  
  // Last infos used for updating
  static String lastInfosAsSSV = "";
  
  // Distance to turn
  static String currentTurnDistance="-";
  static String lastTurnDistance="-";
  
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
    lastUpdate = 0;
    update(context,null,true);
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
  
  private static void refreshApp(Context context, String[] infos) {
    
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
  
  public static void update(final Context context, String infosAsSSV, boolean forceUpdate) {
    
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
    
    // If the turn has appeared or disapperas, force an update
    if ((currentTurnDistance.equals("-"))&&(!lastTurnDistance.equals("-")))
      forceUpdate=true;
    if ((!currentTurnDistance.equals("-"))&&(lastTurnDistance.equals("-")))
      forceUpdate=true;
    
    // Check if the info is updated too fast
    long minUpdatePeriod;
    if (currentTurnDistance.equals("-"))
      minUpdatePeriod = minUpdatePeriodNormal;
    else
      minUpdatePeriod = minUpdatePeriodTurn;
    long diffToLastUpdate = Calendar.getInstance().getTimeInMillis() - lastUpdate;
    if ((!forceUpdate)&&(diffToLastUpdate < minUpdatePeriod)) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDMetaWatch", "Skipped update because last update was too recent");
      return;
    }
        
    // Draw the bitmap
    refreshApp(context,infos);
    
    // Inform metawatch app
    Intent intent = new Intent("org.metawatch.manager.APPLICATION_UPDATE");
    Bundle b = new Bundle();
    b.putString("id", id);
    b.putIntArray("array", makeSendableArray(bitmap));
    intent.putExtras(b);
    context.sendBroadcast(intent);

    // Vibrate if the turn appears the first time
    if ((!currentTurnDistance.equals("-"))&&(lastTurnDistance.equals("-"))) {
      
      // Ensure that the app is shown
      intent = new Intent("org.metawatch.manager.APPLICATION_START");
      b = new Bundle();
      b.putString("id", id);
      b.putString("name", name);
      intent.putExtras(b);
      context.sendBroadcast(intent);
      
      // Delay the vibrate to ensure that watch shows the turn
      new Thread(new Runnable() {
        public void run() {
          try {
            Thread.sleep(waitTimeBeforeVibrate);
          } 
          catch (Exception e) {
          }
          Intent intent = new Intent("org.metawatch.manager.VIBRATE");
          Bundle b = new Bundle();
          b.putInt("vibrate_on", 500);
          b.putInt("vibrate_off", 500);
          b.putInt("vibrate_cycles", 2);
          intent.putExtras(b);
          context.sendBroadcast(intent);
        }
      }).start();      
    }

    // Remember when was updated
    lastUpdate = Calendar.getInstance().getTimeInMillis();
    lastTurnDistance=currentTurnDistance;
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
    //update(context,"");
  }

}
