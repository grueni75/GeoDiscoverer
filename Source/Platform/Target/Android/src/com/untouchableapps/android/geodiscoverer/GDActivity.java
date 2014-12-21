//============================================================================
// Name        : GDActivity.java
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

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.SystemClock;
import android.support.v4.app.FragmentActivity;

/** Base class for all activities */
public class GDActivity extends FragmentActivity {

  /** Current dialog */
  AlertDialog alertDialog = null;
    
  /** Time the last toast was shown */
  long lastToastTimestamp = 0;
  
  /** Minimum distance between two toasts in milliseconds */
  int toastDistance = 5000;
  
  // Types of dialogs
  static final int FATAL_DIALOG = 0;
  static final int ERROR_DIALOG = 2;
  static final int WARNING_DIALOG = 1;
  static final int INFO_DIALOG = 3;
    
  /** Shows a dialog  */
  public synchronized void dialog(int kind, String message) {  
    if (kind==WARNING_DIALOG) {      
      long diff=SystemClock.uptimeMillis()-lastToastTimestamp;
      if (diff<=toastDistance) {
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "skipping dialog request <" + message + "> because toast is still visible");
        return;
      }
      GDApplication.showMessageBar(this, message, GDApplication.MESSAGE_BAR_DURATION_LONG);
      lastToastTimestamp=SystemClock.uptimeMillis();
    } else if (kind==INFO_DIALOG) {      
      GDApplication.showMessageBar(this, message, GDApplication.MESSAGE_BAR_DURATION_LONG);
    } else {
      if (alertDialog == null) {
        alertDialog = new AlertDialog.Builder(this).create();
        alertDialog.setTitle(getTitle());
        alertDialog.setIcon(android.R.drawable.ic_dialog_alert);
        alertDialog.setMessage(message);
        alertDialog.setCancelable(false);
        if (kind == FATAL_DIALOG) {
          alertDialog.setButton(getString(R.string.button_label_exit), new DialogInterface.OnClickListener() {
            public void onClick(final DialogInterface dialog, int which) {
              alertDialog=null;
              finish();
            } 
          }); 
        } else {
          alertDialog.setButton(getString(R.string.button_label_ok), new DialogInterface.OnClickListener() {
            public void onClick(final DialogInterface dialog, int which) {
              alertDialog=null;
            }
          }); 
        }
        alertDialog.show();
      } else {
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "skipping dialog request <" + message + "> because alert dialog is already visible");
      }
    }
  }

  /** Shows a fatal dialog and quits the applications */
  public void fatalDialog(String message) {
    dialog(FATAL_DIALOG,message);
  }

  /** Shows an error dialog without quitting the application */
  public void errorDialog(String message) {    
    dialog(ERROR_DIALOG,message);
  }

  /** Shows a warning dialog without quitting the application */
  public void warningDialog(String message) {    
    dialog(WARNING_DIALOG,message);
  }

  /** Shows an info dialog without quitting the application */
  public void infoDialog(String message) {    
    dialog(INFO_DIALOG,message);
  }
    
}
