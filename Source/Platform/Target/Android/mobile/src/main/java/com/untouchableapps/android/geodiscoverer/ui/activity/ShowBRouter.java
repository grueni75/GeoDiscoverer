//============================================================================
// Name        : ShowHelp.java
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2016 Matthias Gruenewald
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


package com.untouchableapps.android.geodiscoverer.ui.activity;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.os.Build;
import android.os.Bundle;
import android.webkit.CookieManager;
import android.webkit.CookieSyncManager;
import android.webkit.WebView;

import androidx.appcompat.app.AppCompatActivity;

public class ShowBRouter extends AppCompatActivity {

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

    // Ensure that cookies are saved
    CookieSyncManager.createInstance(getBaseContext());
    if (android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      CookieManager.getInstance().setAcceptThirdPartyCookies(webview, true);
    } else {
      CookieManager.getInstance().setAcceptCookie(true);
    }

    // Configure the webview
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

    // Load the brouter
    webview.loadUrl("http://localhost:8383/brouter-web/index.html");
  }

  @Override
  protected void onResume() {
    super.onResume();
    CookieSyncManager.getInstance().startSync();
  }

  @Override
  protected void onPause() {
    super.onPause();
    CookieSyncManager.getInstance().sync();
    CookieSyncManager.getInstance().stopSync();
  }

}
