package com.untouchableapps.android.geodiscoverer.ui

import android.Manifest
import android.annotation.SuppressLint
import android.app.ActivityManager
import android.content.Intent
import android.content.pm.PackageManager
import android.content.res.Configuration
import android.hardware.Sensor
import android.hardware.SensorManager
import android.os.Build
import android.os.Bundle
import android.os.PowerManager
import android.os.SystemClock
import android.os.VibrationEffect
import android.os.Vibrator
import android.os.VibratorManager
import android.provider.Settings
import android.view.MotionEvent
import android.view.ViewGroup
import android.view.WindowManager
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.mutableStateOf
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.pointer.pointerInteropFilter
import androidx.compose.ui.viewinterop.AndroidView
import androidx.wear.ambient.AmbientLifecycleObserver
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.core.GDAppInterface
import com.untouchableapps.android.geodiscoverer.core.GDCore
import com.untouchableapps.android.geodiscoverer.core.GDMapSurfaceView
import com.untouchableapps.android.geodiscoverer.ui.theme.WearAppTheme
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.MainScope
import kotlinx.coroutines.cancel
import java.util.Timer
import java.util.TimerTask

class ViewMap : ComponentActivity(), CoroutineScope by MainScope() {

  companion object {
    // Minimum distance between two toasts in milliseconds
    const val TOAST_DISTANCE = 5000
    const val AMBIENT_MODE_TIMEOUT_OFFSET = 1000L
  }

  // Reference to the core object and it's view
  var coreObject: GDCore? = null

  // Flags
  var compassWatchStarted = false

  // Managers
  var sensorManager: SensorManager? = null
  var powerManager: PowerManager? = null
  var vibrator: Vibrator? = null

  // Wake lock for the core
  var wakeLockCore: PowerManager.WakeLock? = null

  // Time the last toast was shown
  var lastToastTimestamp: Long = 0

  /* Variables to control fast rendering in ambient mode
  var isAmbient=false
  var isTransitioningToAmbient=false*/

  // For forcing redraws
  val invalidateState = mutableStateOf(0)

  // Variables to keep the display on
  var keepDisplayOnActive = false
  var displayTimer: Timer? = null
  var displayTimeout = 0L
  var keepSreenOnCount = 0

  // Ambient mode callback
  val ambientCallback = object : AmbientLifecycleObserver.AmbientLifecycleCallback {
    override fun onEnterAmbient(ambientDetails: AmbientLifecycleObserver.AmbientDetails) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "entering ambient mode")
      //coreObject?.executeCoreCommand("setAmbientMode", "1");
      //wakeLockCore?.acquire()
      /*runOnUiThread {
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
      }*/
    }

    override fun onExitAmbient() {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "exiting ambient mode")
    }

    override fun onUpdateAmbient() {
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "ambient mode update")
    }
  }
  private val ambientObserver = AmbientLifecycleObserver(this, ambientCallback)

  // Called when transition to ambient mode finishes
  fun ambientTransitionFinished() {
    //wakeLockCore?.release()
    /*runOnUiThread {
      window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
    }*/
    releaseDisplay()
  }

  // Releases the display
  @Synchronized
  fun releaseDisplay() {
    if (keepDisplayOnActive) return
    displayTimer?.cancel()
    displayTimer=null
    //wakeLockCore?.release()
    setKeepScreenOn(false)
    keepDisplayOnActive = false
  }

  // Keeps the screen on for more than the default time
  @Synchronized
  fun updateDisplayTimeout() {
    //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp", "display timeout update");
    if (displayTimeout == 0L) return
    coreObject?.executeCoreCommand("setAmbientMode", "0");
    if (displayTimer!=null)
      displayTimer?.cancel()
    displayTimer=Timer()
    if (!keepDisplayOnActive)
      setKeepScreenOn(true)
    keepDisplayOnActive=true
    displayTimer?.schedule(object : TimerTask() {
      override fun run() {
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "display timeout expired")
        coreObject?.executeCoreCommand("setAmbientMode", "1");
        keepDisplayOnActive=false
      }
    }, displayTimeout)
    GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "ambient mode start time pushed out")
    //coreObject?.executeCoreCommand("setAmbientModeStartTime", (displayTimeout * 1000).toString())
  }

  // Forces a redraw of the whole view hierarchy
  fun forceRedraw() {
    invalidateState.value++
  }

  // Short vibration to give feedback to user
  fun vibrate() {
    vibrator?.vibrate(VibrationEffect.createOneShot(100, VibrationEffect.DEFAULT_AMPLITUDE))
  }

  // Shows a dialog
  @Synchronized
  fun dialog(kind: Int, message: String) {
    if ((kind == Dialog.Types.WARNING)||(kind == Dialog.Types.INFO)) {
      val diff: Long = SystemClock.uptimeMillis() - lastToastTimestamp
      if (diff <= TOAST_DISTANCE) {
        GDApplication.addMessage(
          GDApplication.DEBUG_MSG, "GDApp",
          "skipping dialog request <$message> because toast is still visible"
        )
        return
      }
      GDApplication.showMessageBar(
        applicationContext,
        message,
        GDApplication.MESSAGE_BAR_DURATION_LONG
      )
      lastToastTimestamp = SystemClock.uptimeMillis()
    } else {
      val intent = Intent(this, Dialog::class.java)
      GDApplication.addMessage(GDAppInterface.DEBUG_MSG, "GDApp", "requesting dialog of kind $kind with message <$message>")
      intent.flags = Intent.FLAG_ACTIVITY_SINGLE_TOP
      intent.putExtra(Dialog.Extras.TEXT, message)
      intent.putExtra(Dialog.Extras.KIND, kind)
      startActivity(intent)
    }
  }

  /** Shows a fatal dialog and quits the applications  */
  fun fatalDialog(message: String) {
    dialog(Dialog.Types.FATAL, message)
  }

  /** Shows an error dialog without quitting the application  */
  fun errorDialog(message: String) {
    dialog(Dialog.Types.ERROR, message)
  }

  /** Shows a warning dialog without quitting the application  */
  fun warningDialog(message: String) {
    dialog(Dialog.Types.WARNING, message)
  }

  /** Shows an info dialog without quitting the application  */
  fun infoDialog(message: String) {
    dialog(Dialog.Types.INFO, message)
  }

  // Handles the setting of the keep screen on flag
  fun setKeepScreenOn(keepOn: Boolean) {
    if (keepOn) {
      if (keepSreenOnCount==0) {
        runOnUiThread {
          window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        }
      }
      keepSreenOnCount++
    } else {
      keepSreenOnCount--
      if (keepSreenOnCount<=0) {
        runOnUiThread {
          window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        }
        keepSreenOnCount=0
      }
    }
    GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "keep screen on count is $keepSreenOnCount")
  }

  // Sets the screen time out
  @SuppressLint("Wakelock")
  fun updateWakeLock() {
    if (coreObject != null) {
      val state = coreObject!!.executeCoreCommand("getWakeLock")
      if (state == "true") {
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "wake lock enabled")
        setKeepScreenOn(true)
      } else {
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "wake lock disabled")
        setKeepScreenOn(false)
      }
    }
  }

  // Start listening for compass bearing
  @Synchronized
  fun startWatchingCompass() {
    if (coreObject != null && !compassWatchStarted) {
      sensorManager?.registerListener(
        coreObject,
        sensorManager?.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
        SensorManager.SENSOR_DELAY_NORMAL
      )
      sensorManager?.registerListener(
        coreObject,
        sensorManager?.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD),
        SensorManager.SENSOR_DELAY_NORMAL
      )
      compassWatchStarted = true
    }
  }

  // Stop listening for compass bearing
  @Synchronized
  fun stopWatchingCompass() {
    if (compassWatchStarted) {
      sensorManager?.unregisterListener(coreObject)
      compassWatchStarted = false
    }
  }

  // Called when a configuration change (e.g., caused by a screen rotation) has occured
  override fun onConfigurationChanged(newConfig: Configuration) {
    super.onConfigurationChanged(newConfig)
  }

  // Communication with the native core
  var coreMessageHandler = CoreMessageHandler(this)

  /** Gets the vibrator in legacy way  */
  @Suppress("deprecation")
  private fun getLegacyVibrator(): Vibrator? {
    return getSystemService(VIBRATOR_SERVICE) as Vibrator;
  }

  // Checks if all required permissions are granted
  private fun permissionsGranted(): Boolean {
    var allPermissionsGranted = true
    if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) allPermissionsGranted =
      false
    if (checkSelfPermission(Manifest.permission.VIBRATE) != PackageManager.PERMISSION_GRANTED) allPermissionsGranted =
      false
   return allPermissionsGranted
  }

  // Creates the activity
  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","ViewMap activity started")

    // Launch the permission dialog if needed
    if (!permissionsGranted()) {
      GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","permissions not granted, launching permission dialog")
      val intent = Intent(this, Dialog::class.java)
      intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK
      intent.putExtra(Dialog.Extras.GET_PERMISSIONS, true)
      startActivity(intent)
      finish()
      return
    }

    // Get the core object
    coreObject = GDApplication.coreObject
    if (coreObject == null) {
      finish()
      return
    }
    (application as GDApplication).setMessageHandler(coreMessageHandler)

    // Get display timeout
    // use watchDisplayTimeout from config instead of system timeout
    try {
      displayTimeout =
        Settings.System.getInt(contentResolver, Settings.System.SCREEN_OFF_TIMEOUT).toLong()
        GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", "display timeout is $displayTimeout ms")
    } catch (e: Settings.SettingNotFoundException) {
      displayTimeout = 0
      GDApplication.addMessage(GDApplication.DEBUG_MSG, "GDApp", e.message)
    }
    displayTimeout -= AMBIENT_MODE_TIMEOUT_OFFSET

    // Get the managers
    powerManager = getSystemService(POWER_SERVICE) as PowerManager
    vibrator = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
      (getSystemService(VIBRATOR_MANAGER_SERVICE) as VibratorManager).defaultVibrator
    } else {
      getLegacyVibrator()
    }

    // Get a wake lock
    wakeLockCore = powerManager?.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "GDApp: Active core");
    if (wakeLockCore==null) {
      fatalDialog(getString(R.string.no_wake_lock))
      return
    }

    // Check for OpenGL ES 2.00
    val activityManager = getSystemService(ACTIVITY_SERVICE) as ActivityManager
    val configurationInfo = activityManager.deviceConfigurationInfo
    val supportsEs2 =
      configurationInfo.reqGlEsVersion >= 0x20000 || Build.FINGERPRINT.startsWith("generic")
    if (!supportsEs2) {
      fatalDialog(getString(R.string.opengles20_required))
      return
    }

    // Get important handles
    sensorManager = this.getSystemService(SENSOR_SERVICE) as SensorManager
    if (sensorManager == null) {
      Toast.makeText(this, getString(R.string.missing_system_service), Toast.LENGTH_LONG)
      finish();
      return
    }

    // Get the core object
    coreObject = GDApplication.coreObject
    if (coreObject == null) {
      finish()
      return
    }

    // Init the child objects
    if (coreObject!!.coreEarlyInitComplete) {
      //viewModel.onCoreEarlyInitComplete()
    }

    // Get ambient mode handling
    lifecycle.addObserver(ambientObserver)

    // Create the activity content
    setContent {
      WearAppTheme {
        screenContent()
      }
    }
  }

  // Destroys the activity
  @SuppressLint("Wakelock")
  override fun onDestroy() {
    super.onDestroy()
    GDApplication.addMessage(
      GDApplication.DEBUG_MSG,
      "GDApp",
      "onDestroy called by " + Thread.currentThread().name
    )
    lifecycle.removeObserver(ambientObserver)
    cancel() // Stop all coroutines
  }

  // Called when a new intent is available
  override fun onNewIntent(intent: Intent) {
    setIntent(intent)
    GDApplication.addMessage(
      GDApplication.DEBUG_MSG, "GDApp",
      "onNewIntent: $intent"
    )
  }

  override fun onPause() {
    super.onPause()
    GDApplication.addMessage(
      GDApplication.DEBUG_MSG,
      "GDApp",
      "onPause called by " + Thread.currentThread().name
    )
    stopWatchingCompass()
    coreObject?.executeAppCommand("setWearDeviceSleeping(1)");
    val intent = Intent(this, com.untouchableapps.android.geodiscoverer.GDService::class.java)
    intent.action = "activityPaused"
    startService(intent)
  }

  override fun onResume() {
    super.onResume()
    GDApplication.addMessage(
      GDApplication.DEBUG_MSG,
      "GDApp",
      "onResume called by " + Thread.currentThread().name
    )

    // Resume all components only if a exit or restart is not requested
    startWatchingCompass()
    coreObject?.executeAppCommand("setWearDeviceSleeping(0)");
    val intent = Intent(this, com.untouchableapps.android.geodiscoverer.GDService::class.java)
    intent.action = "activityResumed"
    startService(intent)
    updateDisplayTimeout()
  }

  fun exit() {
    finishAffinity();
    val intent = Intent(this, com.untouchableapps.android.geodiscoverer.GDService::class.java)
    intent.action = "exit"
    startService(intent)
  }

  // Main content on the screen
  @OptIn(ExperimentalComposeUiApi::class)
  @Composable
  private fun screenContent() {
    //val scope = rememberCoroutineScope()
    val tick = invalidateState.value
    Box(
      modifier = Modifier.Companion
        .fillMaxSize()
        .pointerInteropFilter { motionEvent: MotionEvent ->
          updateDisplayTimeout()
          false
        }
    ) {
      mapSurface()
    }
  }

  // Map surface view
  @Composable
  private fun mapSurface() {
    AndroidView(
      factory = { context ->
        GDMapSurfaceView(context, null).apply {
          layoutParams = ViewGroup.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT,
          )
          setCoreObject(coreObject)
        }
      },
    )
  }

}