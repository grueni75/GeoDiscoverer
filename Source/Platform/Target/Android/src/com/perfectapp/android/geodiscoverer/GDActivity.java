//============================================================================
// Name        : GDActivity.java
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

package com.perfectapp.android.geodiscoverer;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.SystemClock;
import android.support.v4.app.FragmentActivity;
import android.widget.Toast;

/** Base class for all activities */
public class GDActivity extends FragmentActivity {

  /** Current dialog */
  AlertDialog alertDialog = null;
  
  /** Current toast */
  Toast warningToast = null;
  
  /** Time the last toast was shown */
  long lastToastTimestamp;
  
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
      if (warningToast!=null) {
        long diff=SystemClock.uptimeMillis()-lastToastTimestamp;
        if (diff<=toastDistance) {
          GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "skipping dialog request <" + message + "> because toast is still visible");
          return;
        }
        warningToast.cancel();
      }
      warningToast=Toast.makeText(getApplicationContext(), message, Toast.LENGTH_LONG);
      warningToast.show();
      lastToastTimestamp=SystemClock.uptimeMillis();
    } else if (kind==INFO_DIALOG) {      
        Toast.makeText(getApplicationContext(), message, Toast.LENGTH_LONG).show();
    } else {
      if (alertDialog == null) {
        alertDialog = new AlertDialog.Builder(this).create();
        alertDialog.setTitle(getTitle());
        alertDialog.setIcon(android.R.drawable.ic_dialog_alert);
        alertDialog.setMessage(message);
        alertDialog.setCancelable(false);
        if (kind == FATAL_DIALOG) {
          alertDialog.setButton(getString(R.string.button_label_exit), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
              alertDialog=null;
              finish();
            } 
          }); 
        } else {
          alertDialog.setButton(getString(R.string.button_label_ok), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
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
