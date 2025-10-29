package com.untouchableapps.android.geodiscoverer.ui

import android.content.Intent
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.Message
import com.untouchableapps.android.geodiscoverer.ui.Dialog
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.ui.ViewMap
import com.untouchableapps.android.geodiscoverer.core.GDCore
import java.lang.ref.WeakReference
import java.util.Vector

class CoreMessageHandler(viewMap: ViewMap) : Handler(Looper.getMainLooper()) {

  var weakViewMap: WeakReference<ViewMap> = WeakReference(viewMap)

  // State vars
  var dialogIntent: Intent? = null
  var lastSetWearDeviceSleepingCmd: String = ""

  /** Called when the core has a message  */
  override fun handleMessage(msg: Message) {

    // Abort if the object is not available anymore
    val viewMap = weakViewMap.get() ?: return

    // Handle the message
    val b: Bundle = msg.data
    when (msg.what) {
      0 -> {

        // Extract the command
        val command = b.getString("command")
        val args_start = command!!.indexOf("(")
        val args_end = command.lastIndexOf(")")
        val commandFunction = command.substring(0, args_start)
        val t = command.substring(args_start + 1, args_end)
        val commandArgs: Vector<String> = Vector<String>()
        var stringStarted = false
        var startPos = 0
        var i = 0
        while (i < t.length) {
          if (t.substring(i, i + 1) == "\"") {
            stringStarted = !stringStarted
          }
          if (!stringStarted) {
            if (t.substring(i, i + 1) == "," || i == t.length - 1) {
              var arg: String
              arg =
                if (i == t.length - 1) t.substring(startPos, i + 1) else t.substring(startPos, i)
              if (arg.startsWith("\"")) {
                arg = arg.substring(1)
              }
              if (arg.endsWith("\"")) {
                arg = arg.substring(0, arg.length - 1)
              }
              commandArgs.add(arg)
              startPos = i + 1
            }
          }
          i++
        }
        //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","command received: " + command);

        // Execute command
        var commandExecuted = false
        if (commandFunction == "fatalDialog") {
          viewMap.fatalDialog(commandArgs.get(0))
          commandExecuted = true
        }
        if (commandFunction == "errorDialog") {
          viewMap.errorDialog(commandArgs.get(0))
          commandExecuted = true
        }
        if (commandFunction == "warningDialog") {
          viewMap.warningDialog(commandArgs.get(0))
          commandExecuted = true
        }
        if (commandFunction == "infoDialog") {
          viewMap.infoDialog(commandArgs.get(0))
          commandExecuted = true
        }
        if (commandFunction == "createProgressDialog") {

          // Create a new dialog if it does not yet exist
          if (dialogIntent==null) {
            dialogIntent = Intent(viewMap, Dialog::class.java)
            dialogIntent?.flags = Intent.FLAG_ACTIVITY_SINGLE_TOP
            dialogIntent?.putExtra(Dialog.Extras.TEXT, commandArgs.get(0))
            val max: Int = commandArgs.get(1).toInt()
            dialogIntent?.putExtra(Dialog.Extras.MAX, max)
            viewMap.startActivity(dialogIntent)
          } else {
            GDApplication.addMessage(
              GDApplication.DEBUG_MSG,
              "GDApp",
              "skipping progress dialog request <" + commandArgs.get(0)
                .toString() + "> because progress dialog is already visible"
            )
          }
          commandExecuted = true
        }
        if (commandFunction == "updateProgressDialog") {
          if (dialogIntent!=null) {
            dialogIntent!!.putExtra(Dialog.Extras.PROGRESS, commandArgs.get(1).toInt())
            viewMap.startActivity(dialogIntent)
          }
          commandExecuted = true
        }
        if (commandFunction == "closeProgressDialog") {
          if (dialogIntent!=null) {
            dialogIntent!!.putExtra(Dialog.Extras.CLOSE, true)
            viewMap.startActivity(dialogIntent)
            dialogIntent=null
          }
          commandExecuted = true
        }
        if (commandFunction == "coreInitialized") {
          commandExecuted = true
        }
        if (commandFunction == "earlyInitComplete") {
          // Nothing to do as of now
          commandExecuted = true
        }
        if (commandFunction == "lateInitComplete") {
          // Nothing to do as of now
          commandExecuted = true
        }
        if (commandFunction == "setSplashVisibility") {
          // Nothing to do as of now
          commandExecuted = true
        }
        if (commandFunction == "updateWakeLock") {
          viewMap.updateWakeLock()
          commandExecuted = true
        }
        if (commandFunction == "updateScreen") {
          viewMap.forceRedraw()
          commandExecuted = true
        }
        if (commandFunction == "restartActivity") {
          if (GDApplication.coreObject != null) {
            val m: Message = Message.obtain(GDApplication.coreObject.messageHandler)
            m.what = GDCore.START_CORE
            GDApplication.coreObject.messageHandler.sendMessage(m)
          }
          commandExecuted = true
        }
        if (commandFunction == "exitActivity") {
          commandExecuted = true
        }
        if (commandFunction == "deactivateSwipes") {
          /*GDApplication.addMessage(
            GDApplication.ERROR_MSG, "GDApp",
            "deactivateSwipes command not implemented"
          )
          viewMap.vibrate()*/
          viewMap.errorDialog("Test 123")
          commandExecuted = true
        }
        if (commandFunction == "ambientTransitionFinished") {
          /*synchronized(activeRenderers) {
            activeRenderers.forEach() {
              it.isTransitioningToAmbient = false
            }
          }*/
          viewMap.ambientTransitionFinished()
          commandExecuted = true
        }
        if (commandFunction == "setWearDeviceSleeping") {
          lastSetWearDeviceSleepingCmd = command
          commandExecuted = true
        }
        if (commandFunction == "getWearDeviceAlive") {
          if (lastSetWearDeviceSleepingCmd!="")
            viewMap.coreObject?.executeAppCommand(lastSetWearDeviceSleepingCmd)
          commandExecuted = true
        }
        if (commandFunction == "exit") {
          GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "exit command received")
          viewMap.exit()
          commandExecuted = true
        }
        if (!commandExecuted) {
          GDApplication.addMessage(
            GDApplication.ERROR_MSG, "GDApp",
            "unknown command $command received"
          )
        }
      }
    }
  }
}