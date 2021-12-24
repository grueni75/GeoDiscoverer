//============================================================================
// Name        : GetGoogleMapPlaceList.java
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
import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.inputmethod.BaseInputConnection;
import android.webkit.ConsoleMessage;
import android.webkit.CookieManager;
import android.webkit.CookieSyncManager;
import android.webkit.JavascriptInterface;
import android.webkit.WebChromeClient;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import com.untouchableapps.android.geodiscoverer.GDApplication;
import com.untouchableapps.android.geodiscoverer.ui.component.GDActivity;

import java.util.LinkedList;

public class GetGoogleMapPlaceList extends GDActivity {

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
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      CookieManager.getInstance().setAcceptThirdPartyCookies(webview, true);
    } else {
      CookieManager.getInstance().setAcceptCookie(true);
    }

    // Configure the webview
    webview.getSettings().setJavaScriptEnabled(true);
    webview.getSettings().setSupportZoom(false);
    webview.getSettings().setBuiltInZoomControls(false);
    webview.addJavascriptInterface(new JavaScriptInterface(this), "jsif");
    WebView.setWebContentsDebuggingEnabled(true);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
      webview.getSettings().setDisplayZoomControls(false);
    }
    webview.setWebChromeClient(new WebChromeClient() {
      @Override
      public boolean onConsoleMessage(ConsoleMessage consoleMessage) {
          GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp",consoleMessage.message());
          return true;
      }
    });
    webview.setWebViewClient(new WebViewClient() {
      public void onPageFinished(WebView view, String url) {
        webview.loadUrl(
            "javascript:\n"+
            "var addressPointList=document.querySelector('.ml-place-list-details-listbox');"+
            "var addressPointNames=document.querySelectorAll('[class*=title]');"+
            "for (var i=0;i<addressPointNames.length;i++) {"+
            "  console.log(i);"+
            "  if (jsif.processAddressPoint(String.valueOf(i))) {"+
            "    addressPointList.children[i].scrollIntoView();"+
            "    addressPointList.onscroll();"+
            "  }"+
            "}"
            /*"var addressPoints=document.querySelectorAll('.mapsLiteJsUiutilEntitylistitem__ml-entity-list-item-title');"+
            "addressPoints[19].scrollIntoView();"+
            "addressPoints[19].scroll();"
            "addressPointList.scroll();"*/
            /*"for (var i=0; i<addressPoints.length; i++) {"+
            "  console.log(i);"+
            "  jsif.cursorDown();"+
            "}"*/
        );
        /*webview.loadUrl(
            "javascript:\n"+
                "var addressPoints = document.querySelectorAll('.mapsLiteJsUiutilEntitylistitem__ml-entity-list-item-title');"+
                "addressPoints[19].scrollIntoView();"+
                "for (var i=0; i<addressPoints.length; i++) {"+
                "  console.log(i);"+
                "  jsif.cursorDown();"+
                "}"
        );*/

        GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","page load finished");
      }
    });

    // Load the brouter
    webview.loadUrl("https://www.google.com/maps/@52.5408516,12.9367308,10z/data=!3m1!4b1!4m3!11m2!2ssHiG-_lnZOBUEoGQomN1WhheBej-zw!3e3");
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

  // Called by webview javascript
  public class JavaScriptInterface {

    private Activity activity;
    LinkedList<String> knownAddressPoints = new LinkedList<String>();

    public JavaScriptInterface(Activity activity) {
      this.activity = activity;
    }

    @JavascriptInterface
    public void cursorDown() {
      BaseInputConnection mInputConnection = new BaseInputConnection( webview, true);
      KeyEvent kd = new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DPAD_DOWN);
      KeyEvent ku = new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_DPAD_DOWN);
      mInputConnection.sendKeyEvent(kd);
      mInputConnection.sendKeyEvent(ku);
    }

    @JavascriptInterface
    public boolean processAddressPoint(String name) {
      if (!knownAddressPoints.contains(name)) {
        knownAddressPoints.add(name);
        return true;
      } else {
        return false;
      }
    }
  }
}
