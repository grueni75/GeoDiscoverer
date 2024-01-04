//============================================================================
// Name        : ViewModel.kt
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2021 Matthias Gruenewald
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

package com.untouchableapps.android.geodiscoverer.ui.activity.viewmap

import androidx.compose.animation.ExperimentalAnimationApi
import androidx.compose.animation.core.MutableTransitionState
import androidx.compose.material.DismissState
import androidx.compose.material.DismissValue
import androidx.compose.material.ExperimentalMaterialApi
import androidx.compose.material3.*
import com.untouchableapps.android.geodiscoverer.R
import java.util.*
import androidx.compose.runtime.*
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.graphics.ExperimentalGraphicsApi
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.core.GDCore
import com.untouchableapps.android.geodiscoverer.logic.GDBackgroundTask
import com.untouchableapps.android.geodiscoverer.ui.activity.ViewMap
import java.io.File

@ExperimentalGraphicsApi
@ExperimentalMaterial3Api
@ExperimentalMaterialApi
@ExperimentalAnimationApi
@ExperimentalComposeUiApi
class ViewModel(viewMap: ViewMap) : androidx.lifecycle.ViewModel() {

  // Parameters
  val viewMap = viewMap

  // Called when the object is initialized
  fun onCoreEarlyInitComplete() {
    if (integratedListPOISearchRadius==0f)
      integratedListPOISearchRadius=(viewMap.coreObject!!.configStoreGetStringValue("Navigation", "poiSearchDistance")).toFloat()/1000f
    integratedListPOISearchRadiusMax=(viewMap.coreObject!!.configStoreGetStringValue("Navigation", "poiMaxSearchDistance")).toFloat()/1000f
  }

  // Check box item
  class CheckboxItem(
    var text: String,
    var checked: Boolean = false
  )

  // Address point item
  inner class IntegratedListItem(
    var left: String,
    var right: String="",
    var isPOI: Boolean,
    var index: Int,
    var longitude: Double=0.0,
    var latitude: Double=0.0
  ) {
    var visibilityState = MutableTransitionState<Boolean>(true)
    var dismissState=DismissState(
      initialValue = DismissValue.Default,
      confirmStateChange = {
        if (it == DismissValue.DismissedToStart) {
          if (isPOI) {
            /*GDApplication.addMessage(
              GDApplication.DEBUG_MSG,
              "GDApp",
              "POI: delete confirmed for ${index}"
            )*/
            integratedListDeleteItemHandler(index)
          } else {
            /*GDApplication.addMessage(
              GDApplication.DEBUG_MSG,
              "GDApp",
              "AP: starting hide animation for ${index}"
            )*/
            visibilityState.targetState = false
            return@DismissState true
          }
        }
        if (it == DismissValue.DismissedToEnd) {
          if (isPOI) {
            /*GDApplication.addMessage(
              GDApplication.DEBUG_MSG,
              "GDApp",
              "export confirmed for ${index}"
            )*/
            integratedListPOIImportHandler(index)
            return@DismissState false
          } else {
            /*GDApplication.addMessage(
              GDApplication.DEBUG_MSG,
              "GDApp",
              "edit confirmed for ${index}"
            )*/
            integratedListEditItemHandler(index)
            return@DismissState false
          }
        }
        false
      }
    )
    fun checkDismissState() {
      if (!visibilityState.targetState && !visibilityState.currentState) {
        if (dismissState.targetValue == DismissValue.DismissedToStart) {
          GDApplication.addMessage(
            GDApplication.DEBUG_MSG,
            "GDApp",
            "AP: delete confirmed for ${index}"
          )
          integratedListDeleteItemHandler(index)
        }
      }
    }
  }

  // Pending waypoint import questions
  class PendingImportWaypointsDecision(
    val gpxFilename: String,
    val waypointCount: String,
    val confirmHandler: () -> Unit = {},
    val dismissHandler: () -> Unit = {}
  )

  // State
  var fixSurfaceViewBug: Boolean by mutableStateOf(false)
  var drawerStatus: DrawerValue by mutableStateOf(DrawerValue.Closed)
  var messages: String by mutableStateOf("")
  var splashVisible: Boolean by mutableStateOf(false)
  var messagesVisible: Boolean by mutableStateOf(false)
  var busyText: String by mutableStateOf("")
  var snackbarText: String by mutableStateOf("")
    private set
  var snackbarVisible: Boolean by mutableStateOf(false)
    private set
  var snackbarActionText: String by mutableStateOf("")
    private set
  var snackbarActionHandler: () -> Unit = {}
    private set
  var progressMax: Int by mutableStateOf(-1)
    private set
  var progressCurrent: Int by mutableStateOf(0)
    private set
  var progressMessage: String by mutableStateOf("")
    private set
  var dialogMessage: String by mutableStateOf("")
    private set
  var dialogIsFatal: Boolean by mutableStateOf(false)
    private set
  var askTitle: String by mutableStateOf("")
    private set
  var askMessage: String by mutableStateOf("")
    private set
  var askEditTextEnabled: Boolean by mutableStateOf(false)
    private set
  var askEditTextValue: String by mutableStateOf("")
    private set
  var askEditTextTag: String by mutableStateOf("")
    private set
  var askEditTextTagLabel: String by mutableStateOf("")
    private set
  val askEditTextTagList = mutableStateListOf<String>()
  var askEditTextValueChangeHandler: (String) -> Unit = {}
    private set
  var askEditTextHint: String by mutableStateOf("")
    private set
  var askEditTextError: String by mutableStateOf("")
    private set
  val askMultipleChoiceList = mutableStateListOf<CheckboxItem>()
  val askSingleChoiceList = mutableStateListOf<String>()
  var askConfirmText: String by mutableStateOf("")
    private set
  var askDismissText: String by mutableStateOf("")
    private set
  var askEditTextConfirmHandler: (String, String) -> Unit = {_,_ ->}
    private set
  var integratedListPOICategoryHandler: (Int, String) -> Unit = { _, _ ->}
    private set
  var askEditTextAddressHandler: (Int, GDBackgroundTask.AddressPointItem) -> Unit = {_,_ ->}
    private set
  var integratedListPOISearchRadiusHandler: (Float) -> Unit = {}
    private set
  var askQuestionConfirmHandler: () -> Unit = {}
    private set
  var askQuestionDismissHandler: () -> Unit = {}
    private set
  var askMultipleChoiceConfirmHandler: (List<String>) -> Unit = {}
    private set
  var askSingleChoiceConfirmHandler: (String) -> Unit = {}
    private set
  var askMultipleChoiceCheckedHandler: () -> Unit = {}
    private set
  val pendingImportWaypointsDecisions = mutableListOf<PendingImportWaypointsDecision>()
  var integratedListVisible: Boolean by mutableStateOf(false)
    private set
  var integratedListTitle: String by mutableStateOf("")
    private set
  var integratedListPrevWidgetPage = "Default"
  val integratedListItems = mutableStateListOf<IntegratedListItem>()
  val integratedListTabs = mutableStateListOf<String>()
  var integratedListSelectedItem: Int by mutableStateOf(-1)
    private set
  var integratedListSelectedTab: Int by mutableStateOf(-1)
    private set
  var integratedListSelectItemHandler: (Int) -> Unit = {}
    private set
  var integratedListDeleteItemHandler: (Int) -> Unit = {}
    private set
  var integratedListEditItemHandler: (Int) -> Unit = {}
    private set
  var integratedListAddItemHandler: () -> Unit = {}
    private set
  var integratedListRefreshItemsHandler: () -> Unit = {}
  var integratedListSelectTabHandler: (Int) -> Unit = {}
    private set
  var integratedListPOIImportHandler: (Int) -> Unit = {}
  var integratedListPOIFilerEnabled: Boolean by mutableStateOf(false)
    private set
  val integratedListPOICategoryPath = mutableStateListOf<String>()
  var integratedListPOICategoryPathLeafReached: Boolean by mutableStateOf(false)
  val integratedListPOICategoryList = mutableStateListOf<String>()
  var integratedListPOISelectedCategory: Int by mutableStateOf(-1)
    private set
  var integratedListBusy: Boolean by mutableStateOf(false)
    private set
  var integratedListOutdated = false
  var integratedListLimitReached: Boolean by mutableStateOf(false)
    private set
  var integratedListPOISearchRadius: Float by mutableStateOf(0f)
    private set
  var integratedListPOISearchRadiusMax: Float by mutableStateOf(1f)
    private set
  val integratedListPOIItems = mutableStateListOf<IntegratedListItem>()
  var integratedListCenterItem: Boolean by mutableStateOf(false)
    private set
  var restartCoreAfterNoPendingWaypointsDecision = false

  @Synchronized
  fun toggleMessagesVisibility() {
    messagesVisible = !messagesVisible
  }

  @Synchronized
  fun openProgress(message: String, maxProgress: Int) {
    progressMessage = message
    progressCurrent = 0
    progressMax = maxProgress
  }

  @Synchronized
  fun setProgress(value: Int, message: String="") {
    if (message!="") progressMessage = message
    progressCurrent = value
  }

  @Synchronized
  fun closeProgress() {
    progressMax=-1
  }

  @Synchronized
  fun setDialog(message: String, isFatal: Boolean = false) {
    dialogIsFatal = isFatal
    dialogMessage = message
  }

  @Synchronized
  fun showSnackbar(text: String, actionText: String = "", actionHandler: () -> Unit = {}) {
    snackbarActionHandler = actionHandler
    snackbarActionText = actionText
    snackbarText = text
    snackbarVisible = true
  }

  @Synchronized
  fun hideSnackbar() {
    snackbarVisible = false
  }

  @Synchronized
  private fun configureAddressPointTag() {
    askEditTextTag = collectAddressPointGroups(askEditTextTagList)
    askEditTextTagLabel = viewMap.getString(R.string.dialog_tag_label)
  }

  @Synchronized
  fun setPOISearchRadius(radius: Float) {
    integratedListPOISearchRadius = radius
  }

  @Synchronized
  fun setEditTextValue(value: String) {
    askEditTextValue=value
  }

  @Synchronized
  fun setEditTextTag(tag: String) {
    askEditTextTag=tag
  }

  @Synchronized
  fun askForAddressPointAdd(address: String, confirmHandler: (String, String) -> Unit) {
    closeQuestion()
    askEditTextEnabled=true
    if (address!="")
      askEditTextValue = address
    askEditTextHint = viewMap.getString(R.string.dialog_address_input_hint)
    configureAddressPointTag()
    askConfirmText = viewMap.getString(R.string.dialog_lookup)
    askDismissText = viewMap.getString(R.string.dialog_dismiss)
    askEditTextConfirmHandler = confirmHandler
    askMessage = viewMap.getString(R.string.dialog_address)
    askTitle = viewMap.getString(R.string.dialog_add_address_point_title)
  }

  @Synchronized
  fun askForAddressPointEdit(pointName: String, confirmHandler: (String, String) -> Unit) {
    closeQuestion()
    askEditTextEnabled=true
    askEditTextValue = pointName
    askMessage = viewMap.getString(R.string.dialog_name_label)
    configureAddressPointTag()
    askConfirmText = viewMap.getString(R.string.dialog_update)
    askDismissText = viewMap.getString(R.string.dialog_dismiss)
    askEditTextConfirmHandler = confirmHandler
    askTitle = viewMap.getString(R.string.dialog_rename_address_point_title)
  }

  @Synchronized
  fun askForRouteDownload(name: String, dstFile: File, confirmHandler: () -> Unit) {
    closeQuestion()
    var message: String = if (dstFile.exists())
      viewMap.getString(R.string.dialog_overwrite_route_question)
    else viewMap.getString(R.string.dialog_copy_route_question)
    message = String.format(message, name)
    askMessage = message
    askConfirmText = viewMap.getString(R.string.dialog_yes)
    askDismissText = viewMap.getString(R.string.dialog_no)
    askQuestionConfirmHandler = confirmHandler
    askTitle = viewMap.getString(R.string.dialog_route_name_title)
  }

  @Synchronized
  fun askForRouteName(gpxName: String, confirmHandler: (String, String) -> Unit) {
    closeQuestion()
    askEditTextEnabled=true
    askEditTextValue = gpxName
    askEditTextValueChangeHandler = { value ->
      val dstFilename = GDCore.getHomeDirPath() + "/Route/" + value
      val dstFile = File(dstFilename)
      askEditTextError = if (value == "" || !dstFile.exists())
        ""
      else
        viewMap.getString(R.string.route_exists)
    }
    askConfirmText = viewMap.getString(R.string.dialog_import)
    askDismissText = viewMap.getString(R.string.dialog_dismiss)
    askEditTextConfirmHandler = confirmHandler
    askMessage = viewMap.getString(R.string.dialog_route_name_message)
    askTitle = viewMap.getString(R.string.dialog_route_name_title)
  }

  @Synchronized
  fun askForTrackTreatment(confirmHandler: () -> Unit, dismissHandler: () -> Unit) {
    closeQuestion()
    askMessage = viewMap.getString(R.string.dialog_continue_or_new_track_question)
    askConfirmText = viewMap.getString(R.string.dialog_new_track)
    askDismissText = viewMap.getString(R.string.dialog_contine_track)
    askQuestionConfirmHandler = confirmHandler
    askQuestionDismissHandler = dismissHandler
    askTitle = viewMap.getString(R.string.dialog_track_treatment_title)
  }

  @Synchronized
  fun askForMapDownloadType(confirmHandler: () -> Unit, dismissHandler: () -> Unit) {
    closeQuestion()
    askMessage = viewMap.getString(R.string.dialog_map_download_type_question)
    askConfirmText = viewMap.getString(R.string.dialog_map_download_type_option2)
    askDismissText = viewMap.getString(R.string.dialog_map_download_type_option1)
    askQuestionConfirmHandler = confirmHandler
    askQuestionDismissHandler = dismissHandler
    askTitle = viewMap.getString(R.string.dialog_map_download_type_title)
  }

  @Synchronized
  fun askForMapDownloadDetails(routeName: String, confirmHandler: (List<String>) -> Unit) {
    closeQuestion()
    val result = viewMap.coreObject!!.executeCoreCommand("getMapLayers()")
    val mapLayers = result.split(",".toRegex()).toList()
    askConfirmText = viewMap.getString(R.string.dialog_download)
    askDismissText = viewMap.getString(R.string.dialog_dismiss)
    askMultipleChoiceList.clear()
    mapLayers.forEach {
      askMultipleChoiceList.add(CheckboxItem(it))
    }
    askMultipleChoiceCheckedHandler = {
      askMessage = viewMap.getString(R.string.dialog_download_job_estimating_size_message)
      viewMap.addMapDownloadJob(true, routeName, getSelectedChoices())
    }
    askMultipleChoiceConfirmHandler = confirmHandler
    askMessage = viewMap.getString(R.string.dialog_download_job_no_level_selected_message)
    askTitle = viewMap.getString(R.string.dialog_download_job_level_selection_question)
  }

  @Synchronized
  fun setMessage(message: String) {
    askMessage = message
  }

  @Synchronized
  private fun askForImportWaypointsDecision(question: PendingImportWaypointsDecision) {
    closeQuestion()
    askMessage = viewMap.getString(
      R.string.dialog_waypoint_import_message,
      question.waypointCount,
      question.gpxFilename
    )
    askConfirmText = viewMap.getString(R.string.dialog_yes)
    askDismissText = viewMap.getString(R.string.dialog_no)
    askQuestionConfirmHandler = question.confirmHandler
    askQuestionDismissHandler = question.dismissHandler
    askTitle = viewMap.getString(R.string.dialog_waypoint_import_title)
  }

  @Synchronized
  fun askForImportWaypointsDecision(
    gpxFilename: String,
    waypointCount: String,
    confirmHandler: () -> Unit,
    dismissHandler: () -> Unit
  ) {
    val decision = PendingImportWaypointsDecision(
      gpxFilename, waypointCount, confirmHandler, dismissHandler
    )
    if (askTitle != "") {
      if (pendingImportWaypointsDecisions.isEmpty())
        restartCoreAfterNoPendingWaypointsDecision=false
      pendingImportWaypointsDecisions.add(decision)
    } else {
      askForImportWaypointsDecision(decision)
    }
  }

  @Synchronized
  fun askForRouteRemovalKind(
    confirmHandler: () -> Unit,
    dismissHandler: () -> Unit
  ) {
    closeQuestion()
    askMessage = viewMap.getString(R.string.dialog_route_remove_question)
    askConfirmText = viewMap.getString(R.string.dialog_hide)
    askDismissText = viewMap.getString(R.string.dialog_delete)
    askQuestionConfirmHandler = confirmHandler
    askQuestionDismissHandler = dismissHandler
    askTitle = viewMap.getString(R.string.dialog_route_remove_title)
  }

  @Synchronized
  fun isImportWaypointsDecisionPending(): Boolean {
    return pendingImportWaypointsDecisions.isNotEmpty()
  }

  @Synchronized
  fun askForMapCleanup(confirmHandler: () -> Unit, dismissHandler: () -> Unit) {
    closeQuestion()
    askMessage = viewMap.getString(R.string.dialog_map_cleanup_question)
    askConfirmText = viewMap.getString(R.string.dialog_map_cleanup_all_zoom_levels)
    askDismissText = viewMap.getString(R.string.dialog_map_cleanup_current_zoom_level)
    askQuestionConfirmHandler = confirmHandler
    askQuestionDismissHandler = dismissHandler
    askTitle = viewMap.getString(R.string.dialog_map_cleanup_title)
  }

  @Synchronized
  fun askForTracksAsRoutes(confirmHandler: (List<String>) -> Unit) {
    closeQuestion()

    // Obtain the list of tracks in the folder
    val folderFile = File(viewMap.coreObject!!.homePath + "/Track")
    val routes = mutableListOf<String>()
    val files = folderFile.listFiles()
    if (files!=null) {
      for (file in files) {
        if (!file.isDirectory
          && file.name.substring(file.name.length - 1) != "~"
          && file.name.substring(file.name.length - 4) != ".bin"
        ) {
          routes.add(file.name)
        }
      }
    }
    routes.sort()
    routes.reverse()

    // Create the dialog
    askConfirmText = viewMap.getString(R.string.dialog_copy)
    askDismissText = viewMap.getString(R.string.dialog_dismiss)
    askMultipleChoiceList.clear()
    routes.forEach {
      askMultipleChoiceList.add(CheckboxItem(it))
    }
    askMultipleChoiceConfirmHandler = confirmHandler
    askMessage = viewMap.getString(R.string.dialog_track_as_route_selection_question)
    askTitle = viewMap.getString(R.string.dialog_track_as_route_title)
  }

  @Synchronized
  fun askForRemoveRoutes(confirmHandler: (List<String>) -> Unit) {
    closeQuestion()

    // Obtain the list of file in the folder
    val folderFile = File(viewMap.coreObject!!.homePath + "/Route")
    val routes = mutableListOf<String>()
    val files = folderFile.listFiles()
    if (files!=null) {
      for (file in files) {
        if (!file.isDirectory
          && file.name.substring(file.name.length - 1) != "~"
          && file.name.substring(file.name.length - 4) != ".bin"
        ) {
          routes.add(file.name)
        }
      }
    }
    routes.sort()

    // Create the dialog
    askConfirmText = viewMap.getString(R.string.dialog_delete)
    askDismissText = viewMap.getString(R.string.dialog_dismiss)
    askMultipleChoiceList.clear()
    routes.forEach {
      askMultipleChoiceList.add(CheckboxItem(it))
    }
    askMultipleChoiceConfirmHandler = confirmHandler
    askMessage = viewMap.getString(R.string.dialog_route_remove_selection_question)
    askTitle = viewMap.getString(R.string.dialog_remove_routes_title)
  }

  @Synchronized
  fun askForSendLogs(confirmHandler: (List<String>) -> Unit) {
    closeQuestion()

    // Obtain the list of file in the folder
    val folderFile: File = File(viewMap.coreObject!!.homePath + "/Log")
    val logs = mutableListOf<String>()
    val files = folderFile.listFiles()
    if (files!=null) {
      for (file in files) {
        if (!file.isDirectory
          && file.name.substring(file.name.length - 1) != "~"
          && file.name != "send.log"
          && !file.name.endsWith(".dmp")
        ) {
          logs.add(file.name)
        }
      }
    }
    logs.sortWith(
      object : Comparator<String> {
        var stringComparator = Collections.reverseOrder<String>()
        override fun compare(lhs: String, rhs: String): Int {
          var lhs2 = lhs
          var rhs2 = rhs
          val pattern = Regex("^(.*)-(\\d\\d\\d\\d\\d\\d\\d\\d-\\d\\d\\d\\d\\d\\d)\\.log$")
          if (lhs.matches(pattern)) {
            lhs2 = lhs.replace(pattern, "$2-$1.log")
          }
          if (rhs.matches(pattern)) {
            rhs2 = rhs.replace(pattern, "$2-$1.log")
          }
          return stringComparator.compare(lhs2, rhs2)
        }
      }
    )

    // Create the dialog
    askConfirmText = viewMap.getString(R.string.dialog_send)
    askDismissText = viewMap.getString(R.string.dialog_dismiss)
    askMultipleChoiceList.clear()
    logs.forEach {
      askMultipleChoiceList.add(CheckboxItem(it))
    }
    askMultipleChoiceConfirmHandler = confirmHandler
    askMessage = viewMap.getString(R.string.dialog_log_selection_question)
    askTitle = viewMap.getString(R.string.dialog_send_logs_title)
  }

  @Synchronized
  fun askForConfigReset(confirmHandler: () -> Unit) {
    closeQuestion()
    askMessage = viewMap.getString(R.string.dialog_reset_question)
    askConfirmText = viewMap.getString(R.string.dialog_yes)
    askDismissText = viewMap.getString(R.string.dialog_no)
    askQuestionConfirmHandler = confirmHandler
    askTitle = viewMap.getString(R.string.dialog_config_reset_title)
  }

  @Synchronized
  fun askForMapLegend(names: List<String>, confirmHandler: (String) -> Unit) {
    closeQuestion()
    askConfirmText = ""
    askDismissText = ""
    askSingleChoiceList.clear()
    names.forEach {
      askSingleChoiceList.add(it)
    }
    askSingleChoiceConfirmHandler = confirmHandler
    askMessage = viewMap.getString(R.string.dialog_map_legend_selection_question)
    askTitle = viewMap.getString(R.string.dialog_map_legend_selection_title)
  }

  @Synchronized
  fun askForMapLayer(selectHandler: (Int)->Unit) {
    closeQuestion()
    val layers: String = viewMap.coreObject!!.executeCoreCommand("getMapLayers")
    val mapLayers = layers.split(",".toRegex()).toList()
    val selectedLayer: String = viewMap.coreObject!!.executeCoreCommand("getSelectedMapLayer")
    integratedListItems.clear()
    for (i in mapLayers.indices) {
      integratedListItems.add(IntegratedListItem(
        left=mapLayers.get(i),
        isPOI=false,
        index=i
      ))
      if (mapLayers.get(i)==selectedLayer)
        integratedListSelectedItem=i
    }
    integratedListTitle=viewMap.getString(R.string.dialog_map_layer_selection_question)
    integratedListSelectItemHandler=selectHandler
    integratedListCenterItem=true
    openIntegratedList()
  }

  @Synchronized
  fun addAddressPointGroup(name: String) {
    var inserted=false
    for (i in integratedListTabs.indices) {
      if (name<integratedListTabs[i]) {
        integratedListTabs.add(i,name)
        inserted=true
        break
      }
    }
    if (!inserted) {
      integratedListTabs.add(name)
    }
  }

  @Synchronized
  private fun collectAddressPointGroups(groups: MutableList<String>): String {
    val selectedGroup = viewMap.coreObject!!.configStoreGetStringValue("Navigation", "selectedAddressPointGroup")
    val names = viewMap.coreObject!!.configStoreGetAttributeValues("Navigation/AddressPoint", "name").toMutableList()
    groups.clear()
    names.forEach() {
      val groupName: String = viewMap.coreObject!!.configStoreGetStringValue(
        "Navigation/AddressPoint[@name='$it']",
        "group"
      )
      if (!groups.contains(groupName)) {
        groups.add(groupName)
      }
    }
    if (!groups.contains(selectedGroup)) {
      groups.add(selectedGroup)
    }
    if (!groups.contains("Default")) {
      groups.add("Default")
    }
    groups.sort()
    //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","groups size in collectAddressPointGroups: ${groups.size}")
    return selectedGroup
  }

  @Synchronized
  fun fillAddressPoints(fillTabs: Boolean = false) {
    integratedListTitle=viewMap.getString(R.string.dialog_manage_address_points_title)
    integratedListPOIItems.clear()
    setPOIFilterEnabled(false)
    val selectedGroupName = viewMap.coreObject!!.configStoreGetStringValue("Navigation", "selectedAddressPointGroup")
    var selectedItem=""
    if ((integratedListSelectedItem!=-1)&&(integratedListItems.size>integratedListSelectedItem)) {
      selectedItem=integratedListItems[integratedListSelectedItem].left
    }
    if (fillTabs) {
      integratedListTabs.clear()
      integratedListSelectedTab=0
      val groups = mutableListOf<String>()
      collectAddressPointGroups(groups)
      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","groups size in fillAddressPoints: ${groups.size}")
      for (i in groups.indices) {
        addAddressPointGroup(groups[i])
      }
      integratedListTabs.add("POIs")
      for (i in integratedListTabs.indices) {
        if (integratedListTabs[i] == selectedGroupName) {
          integratedListSelectedTab = i
        }
      }
      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","tab size in fillAddressPoints: ${integratedListTabs.size}")
    }
    integratedListSelectedItem=-1
    integratedListItems.clear()
    val addressPoints = GDApplication.backgroundTask.fillAddressPoints()
    for (i in addressPoints.indices) {
      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","adding ${addressPoints[i].nameUniquified}")
      integratedListItems.add(IntegratedListItem(
        left=addressPoints[i].nameUniquified,
        right=addressPoints[i].distanceFormatted,
        isPOI=false,
        index=i,
        longitude=addressPoints[i].longitude,
        latitude=addressPoints[i].latitude
      ))
      if ((selectedItem!="")&&(integratedListItems[i].left==selectedItem)) {
        integratedListSelectedItem=i
      }
    }
    //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","list updated")
  }

  @Synchronized
  private fun fillPOIs() {

    // Set the title
    if (integratedListPOICategoryPath.isEmpty())
      integratedListTitle=viewMap.getString(R.string.dialog_manage_address_points_title)
    else {
      integratedListTitle=integratedListPOICategoryPath.last()
    }


    // Do not continue if search already running
    integratedListItems.clear()
    if (integratedListBusy) {
      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","poi search ongoing, deferring")
      integratedListOutdated = true
      return
    }

    // Fill the address list
    integratedListPOIItems.clear()
    integratedListBusy=true
    integratedListSelectedItem=-1
    GDApplication.backgroundTask.findPOIs(
      categoryPath=integratedListPOICategoryPath.toList(),
      searchRadius=integratedListPOISearchRadius.toInt()*1000)
    { result, limitReached ->
      integratedListBusy = false
      if (integratedListOutdated) {
        //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","new poi search requested, restarting")
        integratedListOutdated = false
        fillPOIs()
      } else {
        for (i in result.indices) {
          integratedListPOIItems.add(IntegratedListItem(
            left=result[i].nameUniquified,
            right=result[i].distanceFormatted,
            isPOI=true,
            index=i,
            longitude=result[i].longitude,
            latitude=result[i].latitude
          ))
        }
        integratedListLimitReached=limitReached
      }
    }
  }

  @Synchronized
  fun manageAddressPoints() {

    // Fill the items and tabs
    fillAddressPoints(true)

    // Fill the remaining fields
    integratedListSelectItemHandler={
      if (integratedListSelectedTab==integratedListTabs.size-1) {
        viewMap.coreObject?.executeCoreCommand("setTargetAtGeographicCoordinate",
          integratedListPOIItems[it].longitude.toString(),
          integratedListPOIItems[it].latitude.toString()
        )
      } else {
        viewMap.coreObject?.executeCoreCommand("setTargetAtAddressPoint",
          integratedListItems[it].left
        )
      }
    }
    integratedListSelectTabHandler={

      // POI tab selected?
      if (it==integratedListTabs.size-1) {

        // Prepare the POI list from the database
        if (integratedListPOICategoryList.isEmpty()) {
          integratedListPOICategoryHandler(-1, "")
        }
        fillPOIs()

      } else {

        // Prepare the stored address points
        viewMap.coreObject!!.configStoreSetStringValue(
          "Navigation",
          "selectedAddressPointGroup",
          integratedListTabs[it]
        )
        viewMap.coreObject?.executeCoreCommand("addressPointGroupChanged")
        fillAddressPoints()
      }
    }
    integratedListDeleteItemHandler={
      var name=if (integratedListSelectedTab==integratedListTabs.size-1)
        integratedListPOIItems[it].left
      else
        integratedListItems[it].left
      viewMap.coreObject?.executeCoreCommand("removeAddressPoint", name)
      // Item is not removed from the list
    }
    integratedListEditItemHandler={
      askForAddressPointEdit(integratedListItems[it].left) { name, group ->

        // Rename the address point if name has changed
        var changed = false
        var newName = name
        if (newName != "" && newName != integratedListItems[it].left) {
          newName = viewMap.coreObject!!.executeCoreCommand(
            "renameAddressPoint",
            integratedListItems[it].left,
            name
          )
          changed = true
        }

        // Change the group if another one is selected
        var fillTabs = false
        if (group != integratedListTabs[integratedListSelectedTab]) {
          val path = "Navigation/AddressPoint[@name='$newName']"
          viewMap.coreObject?.configStoreSetStringValue(path, "group", group)
          viewMap.coreObject?.executeCoreCommand("addressPointGroupChanged")
          changed = true
          fillTabs = true
        }

        // Recreate the list (as there can be deleted one)
        if (changed)
          fillAddressPoints(fillTabs)
      }
    }
    integratedListAddItemHandler={
      askForAddressPointAdd(
        ""
      ) { address, group ->

        // Look up the coordinates
        GDApplication.backgroundTask.getLocationFromAddress(
          context=viewMap,
          name=address,
          address=address,
          group=group
        ) { locationFound ->
          informLocationLookupResult(address,locationFound)
        }
      }
    }
    integratedListPOIImportHandler={
      val addressPoint=integratedListPOIItems[it]
      val group=viewMap.coreObject!!.configStoreGetStringValue("Navigation", "selectedAddressPointGroup")
      viewMap.coreObject!!.executeCoreCommand(
        "addAddressPoint",
        addressPoint.left, "(${addressPoint.longitude},${addressPoint.latitude})",
        addressPoint.longitude.toString(), addressPoint.latitude.toString(),
        group
      )
      //refreshAddressPoints()
    }
    integratedListPOICategoryHandler={ index, category ->

      // Remove the last path if it is already in this list
      if (integratedListPOICategoryPathLeafReached) {
        integratedListPOICategoryPath.removeLast()
      }

      // Decide on the path
      if (category == "..") {
        integratedListPOICategoryPath.removeLast()
      } else if (category != "") {
        integratedListPOICategoryPath.add(category)
      } else {
        integratedListPOICategoryPath.clear()
      }
      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","before: askEditTextCategoryPath.size=${askEditTextCategoryPath.size}")

      // In case this category was seleted already, deselect it
      if ((index == integratedListPOISelectedCategory) && (category != "") && (category != "..")) {
        integratedListPOISelectedCategory = -1
        integratedListPOICategoryPath.removeLast()
      }

      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","after: askEditTextCategoryPath.size=${askEditTextCategoryPath.size}")

      // Check if new categories exists at this path
      val nextCategories = GDApplication.backgroundTask.fillPOICategories(integratedListPOICategoryPath)
      //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","categories=${nextCategories.size}")

      // Find the nearby point of interests with the selected category
      fillPOIs()

      // Fill the next categories if more exists
      integratedListPOISelectedCategory = -1
      if (nextCategories.isNotEmpty()) {
        integratedListPOICategoryPathLeafReached=false
        integratedListPOICategoryList.clear()
        if (integratedListPOICategoryPath.isNotEmpty()) {
          integratedListPOICategoryList.add("..")
        }
        integratedListPOICategoryList.addAll(nextCategories)
        integratedListPOISelectedCategory = -1
      } else {

        // Select the category
        integratedListPOICategoryPathLeafReached=true
        integratedListPOISelectedCategory = index
      }
    }
    integratedListPOISearchRadiusHandler = { radius ->
      setPOISearchRadius(radius)
      fillPOIs()
    }
    integratedListRefreshItemsHandler = {
      refreshAddressPoints()
    }

    // Open the list
    openIntegratedList()
  }

  @Synchronized
  fun refreshAddressPoints() {
    if (integratedListVisible) {
      val poiTabSelected=(integratedListSelectedTab==integratedListTabs.size-1)
      fillAddressPoints(true)
      if (poiTabSelected) {
        integratedListItems.clear()
        integratedListSelectedTab=integratedListTabs.size-1
        fillPOIs()
      }
    }
  }
  @Synchronized
  fun informLocationLookupResult(address: String, found: Boolean) {
    if (!found) {
      viewMap.viewModel.showSnackbar(viewMap.getString(R.string.address_point_not_found, address))
    } else {
      viewMap.viewModel.showSnackbar(viewMap.getString(R.string.address_point_added, address))
      refreshAddressPoints()
    }
  }

  @Synchronized
  fun selectIntegratedListItem(selected: Int) {
    integratedListSelectedItem=selected
  }

  @Synchronized
  fun selectIntegratedListTab(selected: Int) {
    integratedListSelectedTab=selected
  }

  @Synchronized
  fun setPOIFilterEnabled(enabled: Boolean) {
    integratedListPOIFilerEnabled=enabled
  }

  @Synchronized
  fun closeQuestion() {
    askEditTextEnabled=false
    askEditTextValueChangeHandler = {}
    askEditTextAddressHandler = {_,_->}
    askEditTextConfirmHandler = {_,_->}
    askEditTextHint = ""
    askEditTextError = ""
    askEditTextTagList.clear()
    askQuestionConfirmHandler = {}
    askQuestionDismissHandler = {}
    askMultipleChoiceList.clear()
    askSingleChoiceList.clear()
    askMultipleChoiceConfirmHandler = {}
    askMultipleChoiceCheckedHandler = {}
    askSingleChoiceConfirmHandler = {}
    askMessage = ""
    askTitle = ""
  }

  @Synchronized
  fun openIntegratedList() {
    integratedListPOIFilerEnabled=false
    integratedListVisible=true
    integratedListPrevWidgetPage=viewMap.coreObject!!.executeCoreCommand("getPage")
    viewMap.coreObject!!.executeCoreCommand("setPage","Empty","2")
    setEditTextValue("")
  }

  @Synchronized
  fun closeIntegratedList() {
    integratedListPOIFilerEnabled = false
    integratedListVisible = false
    integratedListItems.clear()
    integratedListPOIItems.clear()
    integratedListSelectItemHandler = {}
    integratedListDeleteItemHandler = {}
    integratedListEditItemHandler = {}
    integratedListAddItemHandler = {}
    integratedListRefreshItemsHandler = {}
    integratedListTabs.clear()
    integratedListSelectTabHandler = {}
    integratedListPOIImportHandler = {}
    integratedListPOICategoryHandler = { _, _ -> }
    integratedListPOISearchRadiusHandler = {}
    integratedListTitle = ""
    integratedListBusy=false
    integratedListCenterItem=false
    viewMap.coreObject!!.executeCoreCommand("setPage",integratedListPrevWidgetPage,"-2")
    viewMap.coreObject!!.executeCoreCommand("openFingerMenu")
  }

  @Synchronized
  fun getSelectedChoices(): List<String> {
    val result = mutableListOf<String>()
    askMultipleChoiceList.forEach() {
      if (it.checked) {
        result.add(it.text)
        //GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","${it.text} selected")
      }
    }
    return result
  }

  @Synchronized
  fun allAskAlertDialogsClosed() {
    if (pendingImportWaypointsDecisions.isNotEmpty()) {
      var decision = pendingImportWaypointsDecisions.removeAt(0)
      askForImportWaypointsDecision(decision)
    }
  }

}

