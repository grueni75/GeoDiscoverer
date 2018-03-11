package com.untouchableapps.android.geodiscoverer;

import android.Manifest;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.support.wearable.activity.WearableActivity;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

public class Dialog extends WearableActivity {

  // Identifiers for extras
  public static final String EXTRA_TEXT = "com.untouchableapps.android.geodiscoverer.dialog.TEXT";
  public static final String EXTRA_MAX = "com.untouchableapps.android.geodiscoverer.dialog.MAX";
  public static final String EXTRA_PROGRESS = "com.untouchableapps.android.geodiscoverer.dialog.PROGRESS";
  public static final String EXTRA_KIND = "com.untouchableapps.android.geodiscoverer.dialog.KIND";
  public static final String EXTRA_CLOSE = "com.untouchableapps.android.geodiscoverer.dialog.CLOSE";
  public static final String EXTRA_GET_PERMISSIONS = "com.untouchableapps.android.geodiscoverer.dialog.GET_PERMISSIONS";

  // References to objects in the dialog
  TextView progressDialogText;
  ProgressBar progressDialogBar;
  LinearLayout progressDialogLayout;
  TextView messageDialogText;
  ImageButton messageDialogImageButton;
  LinearLayout messageDialogLayout;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_dialog);

    // Get references
    progressDialogText = (TextView) findViewById(R.id.progress_dialog_text);
    progressDialogBar = (ProgressBar) findViewById(R.id.progress_dialog_bar);
    progressDialogLayout = (LinearLayout) findViewById(R.id.progress_dialog);
    messageDialogImageButton = (ImageButton) findViewById(R.id.message_dialog_button);
    messageDialogText = (TextView) findViewById(R.id.message_dialog_text);
    messageDialogLayout = (LinearLayout) findViewById(R.id.message_dialog);

    // Check which type is requested
    Intent intent = getIntent();
    onNewIntent(intent);

    // Enables Always-on
    setAmbientEnabled();
  }

  @Override
  protected void onNewIntent(Intent intent) {
    super.onNewIntent(intent);

    // Message update?
    if (intent.hasExtra(EXTRA_TEXT)) {
      progressDialogText.setText(intent.getStringExtra(EXTRA_TEXT));
      progressDialogText.requestLayout();
      messageDialogText.setText(intent.getStringExtra(EXTRA_TEXT));
      messageDialogText.requestLayout();
    }

    // Shall permissions be granted?
    if (intent.hasExtra(EXTRA_GET_PERMISSIONS)) {
      requestPermissions(new String[]{
          Manifest.permission.WRITE_EXTERNAL_STORAGE,
          Manifest.permission.VIBRATE}, 0);
    }

    // Max value for progress dialog?
    if (intent.hasExtra(EXTRA_MAX)) {
      int max = intent.getIntExtra(EXTRA_MAX,0);
      progressDialogBar.setIndeterminate(max == 0 ? true : false);
      progressDialogBar.setMax(max);
      progressDialogBar.setProgress(0);
      progressDialogBar.requestLayout();
      progressDialogLayout.setVisibility(progressDialogLayout.VISIBLE);
      progressDialogLayout.requestLayout();
      messageDialogLayout.setVisibility(messageDialogLayout.INVISIBLE);
      messageDialogLayout.requestLayout();
    }

    // Progress update?
    if (intent.hasExtra(EXTRA_PROGRESS)) {
      progressDialogBar.setProgress(intent.getIntExtra(EXTRA_PROGRESS, 0));
      progressDialogBar.requestLayout();
    }

    // Dialog with button?
    if (intent.hasExtra(EXTRA_KIND)) {
      int kind=intent.getIntExtra(EXTRA_KIND,-1);
      if (kind == WatchFace.FATAL_DIALOG) {
        messageDialogImageButton.setOnClickListener(new View.OnClickListener() {
          @Override
          public void onClick(View v) {
            finish();
            System.exit(1);
          }
        });
      } else {
        messageDialogImageButton.setOnClickListener(new View.OnClickListener() {
          @Override
          public void onClick(View v) {
            finish();
          }
        });
      }
      progressDialogLayout.setVerticalGravity(progressDialogLayout.INVISIBLE);
      progressDialogLayout.requestLayout();
      messageDialogLayout.setVisibility(messageDialogLayout.VISIBLE);
      messageDialogLayout.requestLayout();
    }

    // Shall we close?
    if (intent.hasExtra(EXTRA_CLOSE)) {
      finish();
    }

  }
}
