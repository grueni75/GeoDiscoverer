//============================================================================
// Name        : ShowHelp.java
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

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.res.Resources;
import android.os.Build;
import android.os.Bundle;
import android.view.Window;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings.TextSize;
import android.webkit.WebView;

public class ShowHelp extends GDActivity {

  // Web view showing the help
  WebView webview;
  
  /** Called when the activity is first created. */
  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
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
    webview.getSettings().setSupportZoom(false);
    webview.getSettings().setBuiltInZoomControls(false);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
      webview.getSettings().setDisplayZoomControls(false);
    }
    /*webview.setWebChromeClient(new WebChromeClient() {
      public void onProgressChanged(WebView view, int progress) {
        activity.setProgress(progress*100);
      }
    });*/
    
    // Load the help
    webview.loadUrl("file://" + GDApplication.getHomeDirPath() + "/Help/index.html");

  }
  
}
