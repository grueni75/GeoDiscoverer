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

package com.untouchableapps.android.geodiscoverer.cockpit;

import java.util.Calendar;
import java.util.Locale;

import android.content.Context;
import android.speech.tts.TextToSpeech;

import com.untouchableapps.android.geodiscoverer.GDApplication;
import com.untouchableapps.android.geodiscoverer.R;

public class CockpitAppVoice implements CockpitAppInterface, TextToSpeech.OnInitListener {
  
  // Context
  Context context;

  // Cockpit engine
  CockpitEngine cockpitEngine;

  // Average delay the tts system requires to speak
  long speakDelay = 0;

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
  public CockpitAppVoice(Context context, CockpitEngine cockpitEngine) {
    super();
        
    // Init parameters
    minDurationBetweenOffRouteAlerts = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit/App/Voice", "minDurationBetweenOffRouteAlerts")) *  1000;
    speakDelay = Integer.parseInt(GDApplication.coreObject.configStoreGetStringValue("Cockpit/App/Voice", "speakDelay")) *  1000;

    // Remember variables
    this.context = context;
    this.cockpitEngine = cockpitEngine;

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
  public void alert(AlertType type, boolean repeated) {
    if (!textToSpeechReady)
      return;
    long t = Calendar.getInstance().getTimeInMillis();
    String navigationInstructions=this.navigationInstructions;
    if (type==AlertType.offRoute) {
      
      // Only speak off route alert with defined distance
      //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "voiceApp: got off route indication (repeated=" + String.valueOf(repeated) + ")");
      long diffToLastUpdate = t - lastAlert;
      if ((!repeated)||(diffToLastUpdate>minDurationBetweenOffRouteAlerts)) {
        cockpitEngine.audioWakeup();
        textToSpeech.playEarcon("[alert]", TextToSpeech.QUEUE_FLUSH, null);
        textToSpeech.speak(navigationInstructions, TextToSpeech.QUEUE_ADD, null);
        lastAlert=t;
      }
    }
    if (type==AlertType.newTurn) {
      cockpitEngine.audioWakeup();
      textToSpeech.playEarcon("[alert]", TextToSpeech.QUEUE_FLUSH, null);
      textToSpeech.speak(navigationInstructions, TextToSpeech.QUEUE_ADD, null);      
    }
    //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", String.format("t(alert)=%d", System.currentTimeMillis() / 1000));
  }

  /** Converts a distance string into meters */
  protected float textDistanceToMeters(String text) {
    if (text.equals(""))
      return 0;
    float number = Float.valueOf(text.substring(0, text.indexOf(" ")));
    String unit = text.substring(text.indexOf(" ") + 1);
    if (unit.equals("m")) {
      return (float) number;
    }
    if (unit.equals("km")) {
      return (float) number*(float)1e3;
    }
    if (unit.equals("Mm")) {
      return (float) number*(float)1e6;
    }
    if (unit.equals("mi")) {
      return (float) number*(float)1609.34;
    }
    if (unit.equals("yd")) {
      return (float) number*(float)0.9144;
    }
    GDApplication.coreObject.executeAppCommand("fatalDialog(\"Distance unit not supported\")");
    return 0;
  }

  /** Converts a speed string into meters per second */
  protected float textSpeedToMetersPerSecond(String text) {
    if (text.equals(""))
      return 0;
    float number = Float.valueOf(text.substring(0, text.indexOf(" ")));
    String unit = text.substring(text.indexOf(" ")+1);
    if (unit.equals("km/h")) {
      return (float) (number/3.6);
    }
    if (unit.equals("mph")) {
      return (float) (number*0.44704);
    }
    GDApplication.coreObject.executeAppCommand("fatalDialog(\"Speed unit not supported\")");
    return 0;
  }

  /** Converts a distance string into a speakable string */
  protected String textDistanceToVoiceDistance(String distance) {
    if (distance.equals(""))
      return "";
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

    // Is a turn coming?
    String distance="";
    if (!infos.turnAngle.equals("-")) {
      distance = infos.turnDistance;
    } else {
      if (infos.offRoute) {
        distance = infos.routeDistance;
      }
    }
    if (distance.equals(""))
      return;

    // Delay the audio requires for speaking
    long audioDelay=speakDelay;
    if (cockpitEngine.audioIsAsleep())
      audioDelay+=cockpitEngine.audioWakeupDelay;

    // Adapt the distance to the audio delay
    if (!distance.equals("")) {
      //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", String.format("distance=%s",distance));
      if ((!infos.targetBearing.equals("-")) && (!infos.locationSpeed.equals("-"))) {
        float distanceOriginal = textDistanceToMeters(distance);
        float distanceNew;
        float speed = textSpeedToMetersPerSecond(infos.locationSpeed);
        float distanceTravelled = speed * audioDelay / 1000;
        float angleDegree = Float.parseFloat(infos.targetBearing) - Float.parseFloat(infos.locationBearing);
        float angle = angleDegree * (float) Math.PI / (float) 180.0;
        distanceNew = (float) Math.sqrt(distanceTravelled * distanceTravelled +
            distanceOriginal * distanceOriginal - 2 * distanceTravelled * distanceOriginal *
            Math.cos(angle));
        /*GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",String.format("distanceOriginal=%f "+
            "speed=%f distanceTravelled=%f angle(degree)=%f angle(rad)=%f distanceNew=%f",
            distanceOriginal,speed,distanceTravelled,angleDegree,angle,distanceNew));*/
        distance = cockpitEngine.app.coreObject.executeCoreCommand(String.format("formatMeters(%f)", distanceNew));
        //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp",distance);
      }
      //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", String.format("distance=%s",distance));
      distance = textDistanceToVoiceDistance(distance);
    }

    // Is a turn coming?
    if (!infos.turnAngle.equals("-")) {
      Float turnAngle = Float.valueOf(infos.turnAngle);
      distance = textDistanceToVoiceDistance(distance);
      if (turnAngle>0)
        navigationInstructions=context.getString(R.string.tts_turn_left, distance);
      else
        navigationInstructions=context.getString(R.string.tts_turn_right, distance);
    } else {

      // Are we off route?
      if (infos.offRoute) {
        
        distance = textDistanceToVoiceDistance(distance);
        navigationInstructions=context.getString(R.string.tts_off_route, distance);
        
      }
    }
    //GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", String.format("t(update)=%d",System.currentTimeMillis()/1000));
  }
  
  /** Clean up everything */
  public void stop() {
    textToSpeech.stop();
    navigationInstructions=null;
  }
  
  /** Called by the text to speech engine if init is complete */
  public void onInit(int status) {
    if ((status == TextToSpeech.SUCCESS)&&(textToSpeech!=null)) {
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