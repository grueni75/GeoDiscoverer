//============================================================================
// Name        : GDGLSurfaceView.java
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

package com.untouchableapps.android.geodiscoverer.core;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.util.AttributeSet;
import android.view.MotionEvent;

import com.untouchableapps.android.geodiscoverer.core.GDCore;

/** Does the rendering with the OpenGL API */
public class GDMapSurfaceView extends GLSurfaceView {

  /** Interface to the native C++ core */
  protected GDCore coreObject=null;

  /** Constructor */
  @SuppressLint("NewApi")
  public GDMapSurfaceView(Context context, AttributeSet attrs) {
    super(context, attrs);

    // Use OpenGL ES 2.0
    setEGLContextClientVersion(2);

    // Set the framebuffer
    setEGLConfigChooser(8,8,8,8,16,0);

    // Preserve the context if possible
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
      setPreserveEGLContextOnPause(true);
    }
  }

  /** Sets the core object */
  public void setCoreObject(GDCore coreObject) {
    this.coreObject=coreObject;
    setRenderer(coreObject);
    //setRenderMode(RENDERMODE_WHEN_DIRTY);
    //requestRender();
  }

  /** Called if the surface is touched */
  @SuppressLint("ClickableViewAccessibility")
  @TargetApi(Build.VERSION_CODES.ECLAIR)
  public boolean onTouchEvent(final MotionEvent event) {
    return coreObject.onTouchEvent(event);
  }
  
}
