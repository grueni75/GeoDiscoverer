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

import androidx.compose.material3.*
import com.untouchableapps.android.geodiscoverer.R
import java.util.*
import androidx.compose.runtime.*
import com.untouchableapps.android.geodiscoverer.GDApplication
import com.untouchableapps.android.geodiscoverer.core.GDCore
import com.untouchableapps.android.geodiscoverer.ui.activity.ViewMap
import java.io.File

@ExperimentalMaterial3Api
class ViewModel(viewMap: ViewMap) : androidx.lifecycle.ViewModel() {

  // Parameters
  val viewMap = viewMap

  // Check box item
  inner class CheckboxItem(text: String) {
    var text: String = text
    var checked: Boolean = false
  }

  // Pending waypoint import questions
  inner class PendingImportWaypointsDecision(
    gpxFilename: String,
    waypointCount: String,
    confirmHandler: () -> Unit = {},
    dismissHandler: () -> Unit = {}
  ) {
    val gpxFilename = gpxFilename
    val waypointCount = waypointCount
    val confirmHandler = confirmHandler
    val dismissHandler = dismissHandler
  }

  // State
  var fixSurfaceViewBug: Boolean by mutableStateOf(false)
  var mapChanged: Boolean by mutableStateOf(false)
    private set
  var drawerStatus: DrawerValue by mutableStateOf(DrawerValue.Closed)
  var messages: String by mutableStateOf("")
  var splashVisible: Boolean by mutableStateOf(false)
  var messagesVisible: Boolean by mutableStateOf(false)
  var busyText: String by mutableStateOf("")
  var snackbarText: String by mutableStateOf("")
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
  var askEditTextValue: String by mutableStateOf("")
    private set
  var askEditTextTag: String by mutableStateOf("")
    private set
  var askEditTextTagLabel: String by mutableStateOf("")
    private set
  var askEditTextTagList = mutableStateListOf<String>()
    private set
  var askEditTextValueChangeHandler: (String) -> Unit = {}
    private set
  var askEditTextHint: String by mutableStateOf("")
    private set
  var askEditTextError: String by mutableStateOf("")
    private set
  var askMultipleChoiceList = mutableStateListOf<CheckboxItem>()
    private set
  var askSingleChoiceList = mutableStateListOf<String>()
    private set
  var askConfirmText: String by mutableStateOf("")
    private set
  var askDismissText: String by mutableStateOf("")
    private set
  var askEditTextConfirmHandler: (String, String) -> Unit = {_,_ ->}
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
  var pendingImportWaypointsDecisions = mutableListOf<PendingImportWaypointsDecision>()
  var integratedListVisible: Boolean by mutableStateOf(false)
    private set
  var integratedListTitle: String by mutableStateOf("")
    private set
  var integratedListItems = mutableStateListOf<String>()
    private set
  var integratedListTabs = mutableStateListOf<String>()
    private set
  var integratedListSelectedItem: Int by mutableStateOf(-1)
    private set
  var integratedListSelectedTab: Int by mutableStateOf(-1)
    private set
  var integratedListSelectItemHandler: (String) -> Unit = {}
    private set
  var integratedListDeleteItemHandler: (String) -> Unit = {}
    private set
  var integratedListEditItemHandler: (Int) -> Unit = {}
    private set
  var integratedListAddItemHandler: () -> Unit = {}
    private set
  var integratedListSelectTabHandler: (String) -> Unit = {}
    private set

  // Methods to modify the state
  @Synchronized
  fun mapChanged(changed: Boolean) {
    mapChanged = changed
  }

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
  }

  @Synchronized
  private fun configureAddressPointTag() {
    askEditTextTag = collectAddressPointGroups(askEditTextTagList)
    askEditTextTagLabel = viewMap.getString(R.string.dialog_tag_label)
  }

  @Synchronized
  fun askForAddressPointAdd(address: String, confirmHandler: (String, String) -> Unit) {
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
  fun askForAddressPointEdit(pointName: String, selectedGroupName: String, groupNames: List<String>, confirmHandler: (String, String) -> Unit) {
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
    askMessage = viewMap.getString(R.string.dialog_continue_or_new_track_question)
    askConfirmText = viewMap.getString(R.string.dialog_new_track)
    askDismissText = viewMap.getString(R.string.dialog_contine_track)
    askQuestionConfirmHandler = confirmHandler
    askQuestionDismissHandler = dismissHandler
    askTitle = viewMap.getString(R.string.dialog_track_treatment_title)
  }

  @Synchronized
  fun askForMapDownloadType(confirmHandler: () -> Unit, dismissHandler: () -> Unit) {
    askMessage = viewMap.getString(R.string.dialog_map_download_type_question)
    askConfirmText = viewMap.getString(R.string.dialog_map_download_type_option2)
    askDismissText = viewMap.getString(R.string.dialog_map_download_type_option1)
    askQuestionConfirmHandler = confirmHandler
    askQuestionDismissHandler = dismissHandler
    askTitle = viewMap.getString(R.string.dialog_map_download_type_title)
  }

  @Synchronized
  fun askForMapDownloadDetails(routeName: String, confirmHandler: (List<String>) -> Unit) {
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
  fun askForUpdateMessage(message: String) {
    askMessage = message
  }

  @Synchronized
  private fun askForImportWaypointsDecision(question: PendingImportWaypointsDecision) {
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
    askMessage = viewMap.getString(R.string.dialog_route_remove_question)
    askConfirmText = viewMap.getString(R.string.dialog_hide)
    askDismissText = viewMap.getString(R.string.dialog_delete)
    askQuestionConfirmHandler = confirmHandler
    askQuestionDismissHandler = dismissHandler
    askTitle = viewMap.getString(R.string.dialog_route_remove_title)
  }

  @Synchronized
  fun askForImportWaypointsDecisionPending(): Boolean {
    return pendingImportWaypointsDecisions.isNotEmpty()
  }

  @Synchronized
  fun askForMapCleanup(confirmHandler: () -> Unit, dismissHandler: () -> Unit) {
    askMessage = viewMap.getString(R.string.dialog_map_cleanup_question)
    askConfirmText = viewMap.getString(R.string.dialog_map_cleanup_all_zoom_levels)
    askDismissText = viewMap.getString(R.string.dialog_map_cleanup_current_zoom_level)
    askQuestionConfirmHandler = confirmHandler
    askQuestionDismissHandler = dismissHandler
    askTitle = viewMap.getString(R.string.dialog_map_cleanup_title)
  }

  @Synchronized
  fun askForTracksAsRoutes(confirmHandler: (List<String>) -> Unit) {

    // Obtain the list of tracks in the folder
    val folderFile = File(viewMap.coreObject!!.homePath + "/Track")
    val routes = mutableListOf<String>()
    for (file in folderFile.listFiles()) {
      if (!file.isDirectory
        && file.name.substring(file.name.length - 1) != "~"
        && file.name.substring(file.name.length - 4) != ".bin"
      ) {
        routes.add(file.name)
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

    // Obtain the list of file in the folder
    val folderFile = File(viewMap.coreObject!!.homePath + "/Route")
    val routes = mutableListOf<String>()
    for (file in folderFile.listFiles()) {
      if (!file.isDirectory
        && file.name.substring(file.name.length - 1) != "~"
        && file.name.substring(file.name.length - 4) != ".bin"
      ) {
        routes.add(file.name)
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

    // Obtain the list of file in the folder
    val folderFile: File = File(viewMap.coreObject!!.homePath + "/Log")
    val logs = mutableListOf<String>()
    for (file in folderFile.listFiles()) {
      if (!file.isDirectory
        && file.name.substring(file.name.length - 1) != "~"
        && file.name != "send.log"
        && !file.name.endsWith(".dmp")
      ) {
        logs.add(file.name)
      }
    }
    logs.sortWith(
      object : Comparator<String> {
        var stringComparator = Collections.reverseOrder<String>()
        override fun compare(lhs: String, rhs: String): Int {
          var lhs = lhs
          var rhs = rhs
          val pattern = Regex("^(.*)-(\\d\\d\\d\\d\\d\\d\\d\\d-\\d\\d\\d\\d\\d\\d)\\.log$")
          if (lhs.matches(pattern)) {
            lhs = lhs.replace(pattern, "$2-$1.log")
          }
          if (rhs.matches(pattern)) {
            rhs = rhs.replace(pattern, "$2-$1.log")
          }
          return stringComparator.compare(lhs, rhs)
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
    askMessage = viewMap.getString(R.string.dialog_reset_question)
    askConfirmText = viewMap.getString(R.string.dialog_yes)
    askDismissText = viewMap.getString(R.string.dialog_no)
    askQuestionConfirmHandler = confirmHandler
    askTitle = viewMap.getString(R.string.dialog_config_reset_title)
  }

  @Synchronized
  fun askForMapLegend(names: List<String>, confirmHandler: (String) -> Unit) {
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
  fun askForMapLayer(selectHandler: (String)->Unit) {
    val layers: String = viewMap.coreObject!!.executeCoreCommand("getMapLayers")
    val mapLayers = layers.split(",".toRegex()).toList()
    val selectedLayer: String = viewMap.coreObject!!.executeCoreCommand("getSelectedMapLayer")
    integratedListItems.clear()
    for (i in mapLayers.indices) {
      integratedListItems.add(mapLayers.get(i))
      if (mapLayers.get(i)==selectedLayer)
        integratedListSelectedItem=i
    }
    integratedListTitle=viewMap.getString(R.string.dialog_map_layer_selection_question)
    integratedListSelectItemHandler=selectHandler
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
  fun removeAddressPointGroup(name: String) {

  }

  @Synchronized
  fun addAddressPoint(name: String) {
    var inserted=false
    for (i in integratedListItems.indices) {
      if (name<integratedListItems[i]) {
        integratedListItems.add(i,name)
        inserted=true
        break
      }
    }
    if (!inserted) {
      integratedListItems.add(name)
    }
  }

  @Synchronized
  fun removeAddressPoint(name: String) {
    integratedListItems.remove(name)
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
    GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","groups size in collectAddressPointGroups: ${groups.size}")
    return selectedGroup
  }

  @Synchronized
  fun fillAddressPoints(fillTabs: Boolean = false) {
    val selectedGroupName = viewMap.coreObject!!.configStoreGetStringValue("Navigation", "selectedAddressPointGroup")
    var selectedItem=""
    if ((integratedListSelectedItem!=-1)&&(integratedListItems.size>integratedListSelectedItem)) {
      selectedItem=integratedListItems[integratedListSelectedItem]
    }
    if (fillTabs) {
      integratedListTabs.clear()
      integratedListSelectedTab=0
      val groups = mutableListOf<String>()
      collectAddressPointGroups(groups)
      GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","groups size in fillAddressPoints: ${groups.size}")
      for (i in groups.indices) {
        addAddressPointGroup(groups[i])
        if (integratedListTabs[i] == selectedGroupName) {
          integratedListSelectedTab = i
        }
      }
      GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","tab size in fillAddressPoints: ${integratedListTabs.size}")
    }
    val unsortedNames = viewMap.coreObject!!.configStoreGetAttributeValues("Navigation/AddressPoint", "name").toMutableList()
    integratedListSelectedItem=-1
    integratedListItems.clear()
    while (unsortedNames.size > 0) {
      var newestName = unsortedNames.removeFirst()
      val foreignRemovalRequest: String = viewMap.coreObject!!.configStoreGetStringValue(
        "Navigation/AddressPoint[@name='$newestName']",
        "foreignRemovalRequest"
      )
      val groupName: String = viewMap.coreObject!!.configStoreGetStringValue(
        "Navigation/AddressPoint[@name='$newestName']",
        "group"
      )
      if (groupName == selectedGroupName && foreignRemovalRequest != "1") {
        GDApplication.addMessage(GDApplication.DEBUG_MSG,"GDApp","point: ${newestName}")
        addAddressPoint(newestName)
      }
    }
    if (selectedItem!="") {
      for (i in integratedListItems.indices) {
        if (integratedListItems[i]==selectedItem) {
          integratedListSelectedItem=i
          break
        }
      }
    }
  }

  @Synchronized
  fun manageAddressPoints() {

    // Fill the items and tabs
    fillAddressPoints(true)

    // Fill the remaining fields
    integratedListTitle=viewMap.getString(R.string.dialog_manage_address_points_title)
    integratedListSelectItemHandler= {
      viewMap.coreObject!!.executeCoreCommand("setTargetAtAddressPoint", it)
    }
    integratedListSelectTabHandler= {
      viewMap.coreObject!!.configStoreSetStringValue("Navigation", "selectedAddressPointGroup", it)
      viewMap.coreObject!!.executeCoreCommand("addressPointGroupChanged")
      fillAddressPoints()
    }
    integratedListDeleteItemHandler= {
      viewMap.coreObject!!.executeCoreCommand("removeAddressPoint", it)
      // Item is not removed from the list
    }
    integratedListEditItemHandler= {
      askForAddressPointEdit(
        integratedListItems[it],
        integratedListTabs[integratedListSelectedTab],
        integratedListTabs
      ) { name, group ->

        // Rename the address point if name has changed
        var changed = false
        var newName = name
        if (newName != "" && newName != integratedListItems[it]) {
          newName = viewMap.coreObject!!.executeCoreCommand(
            "renameAddressPoint",
            integratedListItems[it],
            name
          )
          changed = true
        }

        // Change the group if another one is selected
        var fillTabs = false
        if (group != integratedListTabs[integratedListSelectedTab]) {
          val path = "Navigation/AddressPoint[@name='$newName']"
          viewMap.coreObject!!.configStoreSetStringValue(path, "group", group)
          viewMap.coreObject!!.executeCoreCommand("addressPointGroupChanged")
          changed = true
          fillTabs = true
        }

        // Recreate the list (as there can be deleted one)
        if (changed)
          fillAddressPoints(fillTabs)
      }
    }
    integratedListAddItemHandler = {
      askForAddressPointAdd(
        "",
      ) { address, group ->

        // Look up the coordinates
        viewMap.backgroundTask!!.getLocationFromAddress(
          name=address,
          address=address,
          group=group
        ) { locationFound ->
          informLocationLookupResult(address,locationFound)
        }


        /* Recreate the list (as there can be deleted one)
        if (added)
          fillAddressPoints(integratedListTabs[integratedListSelectedTab],fillTabs)*/
      }
    }

    // Open the list
    openIntegratedList()
  }

  @Synchronized
  fun refreshAddressPoints() {
    if (integratedListVisible) {
      fillAddressPoints(true)
    }
  }
  @Synchronized
  fun informLocationLookupResult(address: String, found: Boolean) {
    if (!found) {
      viewMap.viewModel.showSnackbar(viewMap.getString(R.string.address_point_not_found, address))
      refreshAddressPoints()
    } else {
      viewMap.viewModel.showSnackbar(viewMap.getString(R.string.address_point_added, address))
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
  fun closeQuestion() {
    askEditTextValue = ""
    askEditTextValueChangeHandler = {}
    askEditTextHint = ""
    askEditTextError = ""
    askEditTextTag = ""
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
    integratedListVisible=true
    viewMap.coreObject!!.executeCoreCommand("setWidgetlessMode","1")
  }

  @Synchronized
  fun closeIntegratedList() {
    integratedListVisible = false
    integratedListItems.clear()
    integratedListSelectItemHandler = {}
    integratedListDeleteItemHandler = {}
    integratedListEditItemHandler = {}
    integratedListAddItemHandler = {}
    integratedListTabs.clear()
    integratedListSelectTabHandler = {}
    integratedListTitle = ""
    viewMap.coreObject!!.executeCoreCommand("setWidgetlessMode","0")
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

