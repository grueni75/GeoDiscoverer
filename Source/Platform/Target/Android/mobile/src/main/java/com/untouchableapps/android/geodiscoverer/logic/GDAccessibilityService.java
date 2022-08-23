//============================================================================
// Name        : GDAccessibilityService.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2021 Matthias Gruenewald
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

package com.untouchableapps.android.geodiscoverer.logic;

import android.accessibilityservice.AccessibilityService;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.graphics.BitmapFactory;
import android.os.Build;

import androidx.core.app.NotificationCompat;

import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

import com.untouchableapps.android.geodiscoverer.GDApplication;
import com.untouchableapps.android.geodiscoverer.R;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class GDAccessibilityService extends AccessibilityService {

  // Managers
  NotificationManager notificationManager;

  // State
  enum ScanState {
    NORMAL,
    NEXT_DESCRIPTION_TEXT_VIEW_IS_ADDRESS
  }
  ScanState bookingScanState = ScanState.NORMAL;

  // Regular expressions
  Pattern plusCodePattern = Pattern.compile("^[A-Z0-9]+\\+[A-Z0-9]+");

  // Information about a detected place
  class AddressPoint {

    // Definition of the address point
    String name=null;
    String address=null;
    String plusCode=null;

    // Compares two string objects
    boolean stringEquals(String a, String b) {
      if (a==null) {
        if (b!=null)
          return false;
      } else {
        if (b==null)
          return false;
        if (!a.equals(b))
          return false;
      }
      return true;
    }

    // Compares this object with another
    @Override
    public boolean equals(Object o) {
      if (this == o) return true;
      if (o == null || getClass() != o.getClass()) return false;
      AddressPoint that = (AddressPoint) o;
      return stringEquals(name,that.name) && stringEquals(address,that.address) && stringEquals(plusCode,that.plusCode);
    }
  }
  AddressPoint currentAddressPoint=null;
  AddressPoint prevAddressPoint=null;
  //LinkedList<AddressPoint> grabbedAddressPoints = new LinkedList<AddressPoint>();

  // Thread that handles the notification creation
  Thread notificationThread=null;

  @Override
  public void onCreate() {
    super.onCreate();
    notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
    if (Build.VERSION.SDK_INT > Build.VERSION_CODES.O) {
      NotificationChannel channel = new NotificationChannel("addressPointGrabber",
          getString(R.string.notification_channel_address_point_grabber_name),
          NotificationManager.IMPORTANCE_HIGH);
      channel.setDescription(getString(R.string.notification_channel_address_point_grabber_description));
      notificationManager.createNotificationChannel(channel);
    }
  }

  @Override
  protected void onServiceConnected() {
    super.onServiceConnected();
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","onServiceConnected called");
  }

  @Override
  public void onInterrupt() {
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","onInterrupt called");
  }

  // Tries to find places in the current view
  private void findPlace(AccessibilityNodeInfo info, int indentLevel) {
    if (info==null)
      return;
    if (Build.VERSION.SDK_INT > Build.VERSION_CODES.JELLY_BEAN_MR2) {

      /* Output debug infos
      String infoDesc="";
      if (info.getClassName()!=null) {
        infoDesc=infoDesc+"C:"+info.getClassName().toString()+" ";
      }
      if (info.getViewIdResourceName()!=null) {
        infoDesc=infoDesc+"I:"+info.getViewIdResourceName()+" ";
      }
      if (info.getText()!=null) {
        infoDesc=infoDesc+"T:"+info.getText().toString()+" ";
      }
      if (info.getContentDescription()!=null) {
        infoDesc=infoDesc+"D:"+info.getContentDescription().toString()+" ";
      }
      if (infoDesc.compareTo("")!=0) {
        String indent="";
        for (int i=0; i<indentLevel; i++)
          indent+="*";
        if (indentLevel>0)
          indent+=" ";
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", indent+infoDesc);
      }*/

      // Name detected?
      if ((info.getViewIdResourceName()!=null)&&(info.getViewIdResourceName().compareTo("com.google.android.apps.maps:id/title")==0)) {
        currentAddressPoint.name=info.getText().toString();
      }
      if ((info.getViewIdResourceName()!=null)&&(info.getViewIdResourceName().compareTo("com.booking:id/hotel_name")==0)) {
        currentAddressPoint.name=info.getText().toString();
        //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "name="+currentAddressPoint.name);
      }

      // Plus code detected?y
      if (info.getText()!=null) {
        String potentialPlusCode=info.getText().toString();
        Matcher m = plusCodePattern.matcher(potentialPlusCode);
        if (m.find()) {
          currentAddressPoint.plusCode=info.getText().toString();
        }
      }

      // Address detected?
      if (info.getContentDescription()!=null) {
        if (info.getContentDescription().toString().startsWith(getString(R.string.address_point_grabbing_gmap_address_tag))) {
          currentAddressPoint.address=info.getContentDescription().toString().substring(9);
        }
      }
      if ((info.getViewIdResourceName()!=null)&&(info.getViewIdResourceName().compareTo("com.booking:id/titleTextView")==0)) {
        if (info.getText().toString().compareTo(getString(R.string.address_point_grabbing_booking_address_tag))==0) {
          bookingScanState = ScanState.NEXT_DESCRIPTION_TEXT_VIEW_IS_ADDRESS;
          //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "next description text view is address");
        } else {
          bookingScanState = ScanState.NORMAL;
        }
      }
      if ((info.getViewIdResourceName()!=null)&&(info.getViewIdResourceName().compareTo("com.booking:id/descriptionTextView")==0)) {
        if (bookingScanState == ScanState.NEXT_DESCRIPTION_TEXT_VIEW_IS_ADDRESS) {
          currentAddressPoint.address=info.getText().toString();
          //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "address="+currentAddressPoint.address);
        }
        bookingScanState = ScanState.NORMAL;
      }

      // Go through all children recursively
      for (int i = 0; i < info.getChildCount(); i++) {
        if (info.getChild(i) != null)
          findPlace(info.getChild(i),indentLevel+1);
      }
    }
  }

  @Override
  public void onAccessibilityEvent(AccessibilityEvent event) {

    // coreObject active?
    if (GDApplication.coreObject==null) {
      return;
    }

    // Get the info
    AccessibilityNodeInfo source = event.getSource();
    if (source == null) {
      return;
    }

    // Check if a place can be found in the window
    if (Build.VERSION.SDK_INT > Build.VERSION_CODES.JELLY_BEAN_MR2) {

      // Output all IDs of the view hierarchy
      currentAddressPoint=new AddressPoint();
      findPlace(getRootInActiveWindow(),0);

      // Place detected?
      if ((currentAddressPoint.address!=null)||(currentAddressPoint.plusCode!=null)) {
        if (currentAddressPoint.name==null) {
          if (currentAddressPoint.address!=null)
            currentAddressPoint.name=currentAddressPoint.address;
          else
            currentAddressPoint.name=currentAddressPoint.plusCode;
        }
        GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","New address point grabbed: "+currentAddressPoint.name);

        // Already seen?
        if ((prevAddressPoint!=null)&&(prevAddressPoint.equals(currentAddressPoint))) {
          GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","Address point already seen, dropping it");
          return;
        }
        prevAddressPoint=currentAddressPoint;

        // Stop any previous thread
        if (notificationThread!=null) {
          notificationThread.interrupt();
          notificationThread=null;
        }

        // Check if the address point already exists
        final String selectedGroupName = GDApplication.coreObject.configStoreGetStringValue("Navigation","selectedAddressPointGroup");
        String names[] = GDApplication.coreObject.configStoreGetAttributeValues("Navigation/AddressPoint","name");
        boolean found=false;
        for (int i=0;i<names.length;i++) {
          //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","name="+names[i]);
          if (names[i].equals(currentAddressPoint.name)) {
            found=true;
            break;
          }
        }
        if (found) {
          GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","Found address point exists already!");
          return;
        }

        // Schedule the thread to create the notification
        //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","Starting notification thread");
        notificationThread=new Thread(new Runnable() {

          AddressPoint addressPoint=currentAddressPoint;
          String group=selectedGroupName;

          @Override
          public void run() {

            // Wait a bit in case a newer address point is detected
            try {
              Thread.sleep(1000);
            }
            catch (InterruptedException e) {
              return;
            }
            notificationThread=null;

            // Create the notification
            //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","Creating notification");
            Intent notificationIntent = new Intent(GDAccessibilityService.this, GDService.class);
            notificationIntent.setAction("addAddressPoint");
            notificationIntent.putExtra("name",addressPoint.name);
            notificationIntent.putExtra("group",group);
            if (addressPoint.plusCode!=null)
              notificationIntent.putExtra("address",addressPoint.plusCode);
            else
              notificationIntent.putExtra("address",addressPoint.address);
            PendingIntent pi = PendingIntent.getService(GDAccessibilityService.this, 0, notificationIntent, PendingIntent.FLAG_CANCEL_CURRENT|PendingIntent.FLAG_CANCEL_CURRENT);
            String notificationTitle=getString(R.string.notification_address_point_grabbed_title,addressPoint.name);
            NotificationCompat.Builder builder = new NotificationCompat.Builder(GDAccessibilityService.this, "addressPointGrabber")
                .setContentTitle(notificationTitle)
                .setContentText(getString(R.string.notification_address_point_grabbed_text))
                .setContentIntent(pi)
                .setLargeIcon(BitmapFactory.decodeResource(getResources(), R.drawable.address_point))
                .setSmallIcon(R.drawable.notification_running)
                .setDefaults(Notification.DEFAULT_ALL)
                .setAutoCancel(true)
                .setPriority(NotificationCompat.PRIORITY_HIGH);
            notificationManager.notify(addressPoint.name.hashCode(), builder.build());
          }
        });
        notificationThread.start();
      }
    }

  }

}
