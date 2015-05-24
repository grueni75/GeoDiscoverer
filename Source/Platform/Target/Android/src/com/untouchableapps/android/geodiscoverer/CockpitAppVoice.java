//============================================================================
// Name        : CockpitAppVoice.java
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

import java.util.Calendar;
import java.util.Locale;

import android.content.Context;
import android.media.MediaPlayer;
import android.speech.tts.TextToSpeech;

public class CockpitAppVoice implements CockpitAppInterface, TextToSpeech.OnInitListener {
  
  // Context
  Context context;
  
  // Last time a navigation alert was spoken
  long lastAlert;
  long minDurationBetweenOffRouteAlerts;
  
  // Text to speech engine for saying something
  TextToSpeech textToSpeech;
  Locale textToSpeechLocale;
  boolean textToSpeechReady = false;
  
  // Current text to say
  String navigationInstructions = null; 
    
  /** Constructor */
  public CockpitAppVoice(Context context) {
    super();
        
    // Init parameters
    minDurationBetweenOffRouteAlerts = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit/App/Voice", "minDurationBetweenOffRouteAlerts")) *  1000;

    // Remember context
    this.context = context;

    // Prepare the text to speech engine
    textToSpeech = new TextToSpeech(context, this);
  }

  /** Prepare voice output */
  public void start() {
    
  }

  /** Not necessary */
  public void focus() {
  }
  
  /** Tell what is happening */
  public void inform() {
  }  

  /** Not necessary */
  public void alert(AlertType type) {
    if (!textToSpeechReady)
      return;
    long t = Calendar.getInstance().getTimeInMillis();
    if (type==AlertType.offRoute) {
      
      // Only speak off route alert with defined distance
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "voiceApp: got off route indication");
      long diffToLastUpdate = t - lastAlert;
      if (diffToLastUpdate>minDurationBetweenOffRouteAlerts) {
        textToSpeech.playEarcon("[alert]", TextToSpeech.QUEUE_FLUSH, null);
        textToSpeech.speak(navigationInstructions, TextToSpeech.QUEUE_ADD, null);
        lastAlert=t;
      }
    }
    if (type==AlertType.newTurn) {
      textToSpeech.playEarcon("[alert]", TextToSpeech.QUEUE_FLUSH, null);
      textToSpeech.speak(navigationInstructions, TextToSpeech.QUEUE_ADD, null);      
    }
  }  

  /** Converts a distance string into a speakable string */
  protected String textDistanceToVoiceDistance(String distance) {
    float distanceNumber = Float.valueOf(distance.substring(0,distance.indexOf(" ")));
    distanceNumber=Math.round(distanceNumber);
    String distanceUnit = distance.substring(distance.indexOf(" ")+1);
    if (distanceUnit.equals("m")) {
      distanceUnit=context.getString(R.string.tts_meters);
    }
    if (distanceUnit.equals("km")) {
      distanceUnit=context.getString(R.string.tts_kilometers);
    }
    if (distanceUnit.equals("Mm")) {
      distanceUnit=context.getString(R.string.tts_megameters);
    }
    if (distanceUnit.equals("mi")) {
      distanceUnit=context.getString(R.string.tts_miles);
    }
    if (distanceUnit.equals("yd")) {
      distanceUnit=context.getString(R.string.tts_yards);
    }
    return String.format(textToSpeechLocale, "%.0f %s", distanceNumber,distanceUnit);
  }
  
  /** Prepare the text to say */
  public void update(CockpitInfos infos) {
    
    navigationInstructions="";
    
    // Are we off route?
    if (infos.offRoute) {
      
      String distance = textDistanceToVoiceDistance(infos.routeDistance);
      navigationInstructions=context.getString(R.string.tts_off_route, distance);
      
    } else {

      // Is a turn coming?
      if (!infos.turnAngle.equals("-")) {
        Float turnAngle = Float.valueOf(infos.turnAngle);
        String distance = textDistanceToVoiceDistance(infos.turnDistance);
        if (turnAngle>0)
          navigationInstructions=context.getString(R.string.tts_turn_left, distance);
        else
          navigationInstructions=context.getString(R.string.tts_turn_right, distance);
        
      }
    }
  }
  
  /** Clean up everything */
  public void stop() {
    textToSpeech.stop();
    navigationInstructions=null;
  }
  
  /** Called by the text to speech engine if init is complete */
  public void onInit(int status) {
    if (status == TextToSpeech.SUCCESS) {
      textToSpeechLocale = new Locale(context.getString(R.string.tts_locale));
      int result = textToSpeech.setLanguage(textToSpeechLocale);
      if ((result == TextToSpeech.LANG_MISSING_DATA) || (result == TextToSpeech.LANG_NOT_SUPPORTED)) {
        GDApplication.coreObject.executeAppCommand("errorDialog(\"" + context.getString(R.string.tts_cannot_set_language) + "\")");
        return;
      }
      if (textToSpeech.addEarcon("[alert]", context.getPackageName(), R.raw.alert)!=TextToSpeech.SUCCESS) {
        GDApplication.coreObject.executeAppCommand("errorDialog(\"" + context.getString(R.string.tts_cannot_add_alert_earcon) + "\")");
        return;
      }
      textToSpeechReady=true;
    } else {
      GDApplication.coreObject.executeAppCommand("errorDialog(\"" + context.getString(R.string.tts_cannot_init) + "\")");
    }    
  }

}
