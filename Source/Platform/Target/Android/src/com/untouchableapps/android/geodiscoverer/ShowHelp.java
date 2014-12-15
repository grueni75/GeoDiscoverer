//============================================================================
// Name        : ShowHelp.java
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

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.res.Resources;
import android.os.Bundle;
import android.view.Window;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings.TextSize;
import android.webkit.WebView;

public class ShowHelp extends GDActivity {

  // Web view showing the help
  WebView webview;
  
  /** Called when the activity is first created. */
  @SuppressLint("SetJavaScriptEnabled")
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    
    // Create the web view
    //getWindow().requestFeature(Window.FEATURE_PROGRESS);
    //getWindow().setFeatureInt(Window.FEATURE_PROGRESS,Window.PROGRESS_VISIBILITY_ON);
    webview = new WebView(this);
    setContentView(webview);
    
    // Let's display the progress in the activity title bar, like the
    // browser app does.
    //setProgressBarIndeterminateVisibility(true);
    //setProgressBarVisibility(true);
    webview.getSettings().setJavaScriptEnabled(true);
    webview.getSettings().setBuiltInZoomControls(true);
    final Activity activity = this;
    /*webview.setWebChromeClient(new WebChromeClient() {
      public void onProgressChanged(WebView view, int progress) {
        activity.setProgress(progress*100);
      }
    });*/
    
    // Load the help
    webview.loadUrl("file://" + GDApplication.getHomeDirPath() + "/Help/index.html");

  }
  
}
