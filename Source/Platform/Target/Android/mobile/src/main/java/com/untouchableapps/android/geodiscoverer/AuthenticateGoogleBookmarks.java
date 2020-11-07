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


package com.untouchableapps.android.geodiscoverer;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.Window;
import android.webkit.CookieManager;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import com.untouchableapps.android.geodiscoverer.core.GDCore;

public class AuthenticateGoogleBookmarks extends GDActivity {

  // Web view showing the help
  WebView webview;
  
  /** Called when the activity is first created. */
  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  @SuppressLint("SetJavaScriptEnabled")
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    
    // Create the web view
    webview = new WebView(this);
    setContentView(webview);
    //getWindow().setFeatureDrawableResource(Window.FEATURE_LEFT_ICON, R.drawable.icon);

    // Set the handle to extract the cookies
    webview.getSettings().setJavaScriptEnabled(true);
    webview.setWebViewClient(new WebViewClient() {
      public void onPageFinished(WebView view, String url) {
        String cookies = CookieManager.getInstance().getCookie(url);
        if ((cookies.contains("SID="))&&(!cookies.contains("ACCOUNT_CHOOSER="))) {
          GDApplication.coreObject.executeCoreCommand("setGoogleBookmarksCookie",cookies);
          finish();
        }

        Log.d("GDApp", "All the cookies in a string:" + cookies);
      }
    });

    // Load the bookmark page to log in
    webview.loadUrl("https://www.google.com/bookmarks");
  }
  
}
