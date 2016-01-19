//============================================================================
// Name        : ViewMap.java
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
import android.content.Context;
import android.opengl.EGLConfig;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.support.wearable.activity.WearableActivity;
import android.support.wearable.view.BoxInsetLayout;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.widget.TextView;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

import javax.microedition.khronos.opengles.GL10;

public class ViewMap extends WearableActivity {

  private static final SimpleDateFormat AMBIENT_DATE_FORMAT =
      new SimpleDateFormat("HH:mm", Locale.US);

  private BoxInsetLayout mContainerView;
  private TextView mTextView;
  private TextView mClockView;

  /** Does the rendering with the OpenGL API */
  public class GDMapSurfaceView extends GLSurfaceView {

    /** Constructor */
    @SuppressLint("NewApi")
    public GDMapSurfaceView(Context context, AttributeSet attrs) {
      super(context, attrs);

      // Set the framebuffer
      setEGLConfigChooser(8,8,8,8,16,0);

      // Use OpenGL ES 2.0
      setEGLContextClientVersion(2);

      // Preserve the context if possible
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
        setPreserveEGLContextOnPause(true);
      }
    }
  }

  GDMapSurfaceView mGLView;

  class ClearRenderer implements GLSurfaceView.Renderer {

    @Override
    public void onSurfaceCreated(GL10 gl, javax.microedition.khronos.egl.EGLConfig config) {

    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
      gl.glViewport(0, 0, width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
      gl.glClear(GL10.GL_COLOR_BUFFER_BIT | GL10.GL_DEPTH_BUFFER_BIT);
    }
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    mGLView = new GDMapSurfaceView(this,null);
    mGLView.setRenderer(new ClearRenderer());
    setContentView(mGLView);


    /*setContentView(R.layout.view_map);
    setAmbientEnabled();

    mContainerView = (BoxInsetLayout) findViewById(R.id.container);
    mTextView = (TextView) findViewById(R.id.text);
    mClockView = (TextView) findViewById(R.id.clock);*/
  }

  @Override
  public void onEnterAmbient(Bundle ambientDetails) {
    super.onEnterAmbient(ambientDetails);
    updateDisplay();
  }

  @Override
  public void onUpdateAmbient() {
    super.onUpdateAmbient();
    updateDisplay();
  }

  @Override
  public void onExitAmbient() {
    updateDisplay();
    super.onExitAmbient();
  }

  private void updateDisplay() {
    /*if (isAmbient()) {
      mContainerView.setBackgroundColor(getResources().getColor(android.R.color.black));
      mTextView.setTextColor(getResources().getColor(android.R.color.white));
      mClockView.setVisibility(View.VISIBLE);

      mClockView.setText(AMBIENT_DATE_FORMAT.format(new Date()));
    } else {
      mContainerView.setBackground(null);
      mTextView.setTextColor(getResources().getColor(android.R.color.black));
      mClockView.setVisibility(View.GONE);
    }*/
  }
}
