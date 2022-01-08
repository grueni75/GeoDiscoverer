//============================================================================
// Name        : ViewContentDialog.kt
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

import androidx.compose.animation.*
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.material.ExperimentalMaterialApi
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.rotate
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.*
import com.untouchableapps.android.geodiscoverer.R
import com.untouchableapps.android.geodiscoverer.ui.component.*
import com.untouchableapps.android.geodiscoverer.ui.theme.SurfaceColorAtElevation

@ExperimentalMaterial3Api
class ViewContentDialog(viewContent: ViewContent) {

  // Parameters
  val viewContent=viewContent
  val viewMap=viewContent.viewMap
  val layoutParams=viewContent.layoutParams

  // Main content of the activity
  @ExperimentalAnimationApi
  @ExperimentalMaterial3Api
  @ExperimentalMaterialApi
  @Composable
  fun content(
    viewModel: ViewModel,
    height: Dp
  ) {
    if (viewModel.progressMax != -1) {
      AlertDialog(
        modifier = Modifier
          .wrapContentHeight(),
        onDismissRequest = {
        },
        confirmButton = {
        },
        title = {
          Text(text = viewModel.progressMessage)
        },
        text = {
          if (viewModel.progressMax==0) {
            GDLinearProgressIndicator(
              modifier = Modifier.fillMaxWidth(),
            )
          } else {
            GDLinearProgressIndicator(
              modifier = Modifier.fillMaxWidth(),
              progress = viewModel.progressCurrent.toFloat() / viewModel.progressMax.toFloat()
            )
          }
        }
      )
    }
    if (viewModel.dialogMessage != "") {
      AlertDialog(
        modifier = Modifier
          .wrapContentHeight(),
        onDismissRequest = {
          viewModel.setDialog("")
        },
        confirmButton = {
          TextButton(
            onClick = {
              if (viewModel.dialogIsFatal)
                viewMap.finish()
              viewModel.setDialog("")
            }
          ) {
            if (viewModel.dialogIsFatal)
              Text(stringResource(id = R.string.button_label_exit))
            else
              Text(stringResource(id = R.string.button_label_ok))
          }
        },
        icon = {
          if (viewModel.dialogIsFatal)
            Icon(Icons.Default.Error, contentDescription = null)
          else
            Icon(Icons.Default.Warning, contentDescription = null)
        },
        text = {
          Text(
            text = viewModel.dialogMessage,
            style = MaterialTheme.typography.bodyLarge
          )
        }
      )
    }
    if (viewModel.askTitle != "") {
      if ((viewModel.askEditTextValue != "")||(viewModel.askEditTextHint != "")) {
        val editTextValue = remember { mutableStateOf(viewModel.askEditTextValue) }
        val editTextTag = remember { mutableStateOf(viewModel.askEditTextTag) }
        askAlertDialog(
          viewModel = viewModel,
          confirmHandler = {
            viewModel.askEditTextConfirmHandler(editTextValue.value, editTextTag.value)
          },
          content = {
            Column(
              modifier = Modifier
                .wrapContentHeight()
            ) {
              viewModel.askEditTextValueChangeHandler(editTextValue.value)
              GDTextField(
                value = editTextValue.value,
                textStyle = MaterialTheme.typography.bodyLarge,
                label = {
                  Text(
                    text = viewModel.askMessage
                  )
                },
                onValueChange = {
                  editTextValue.value = it
                  viewModel.askEditTextValueChangeHandler(it)
                },
                trailingIcon = {
                  IconButton(
                    onClick = { editTextValue.value="" }
                  ) {
                    Icon(
                      imageVector = Icons.Default.Clear,
                      contentDescription = null,
                    )
                  }
                }
              )
              if (viewModel.askEditTextHint != "") {
                Text(
                  modifier = Modifier
                    .padding(top = layoutParams.itemPadding, start = layoutParams.hintIndent),
                  text = viewModel.askEditTextHint
                )
              }
              if (viewModel.askEditTextError != "") {
                Text(
                  modifier = Modifier
                    .padding(top = layoutParams.itemPadding, start = layoutParams.hintIndent),
                  text = viewModel.askEditTextError,
                  color = MaterialTheme.colorScheme.error
                )
              }
              if (viewModel.askEditTextTag != "") {
                Spacer(Modifier.height(layoutParams.itemDistance))
                val editTextTagListExpanded = remember { mutableStateOf(false) }
                val editTextTagListExpandIconAngle: Float by animateFloatAsState(
                  targetValue= if (editTextTagListExpanded.value) 180f else 0f
                )
                Column() {
                  val textFieldWidth = remember { mutableStateOf(0.dp) }
                  BoxWithConstraints() {
                    textFieldWidth.value=this.maxWidth
                    GDTextField(
                      value = editTextTag.value,
                      textStyle = MaterialTheme.typography.bodyLarge,
                      label = {
                        Text(
                          text = viewModel.askEditTextTagLabel
                        )
                      },
                      onValueChange = {
                        editTextTag.value = it
                      },
                      trailingIcon = {
                        Row(
                          modifier = Modifier
                            .align(Alignment.CenterEnd),
                        ) {
                          IconButton(
                            onClick = { editTextTag.value="" }
                          ) {
                            Icon(
                              imageVector = Icons.Default.Clear,
                              contentDescription = null,
                            )
                          }
                          IconButton(
                            onClick = { editTextTagListExpanded.value = true }
                          ) {
                            Icon(
                              modifier = Modifier
                                .rotate(editTextTagListExpandIconAngle),
                              imageVector = Icons.Default.ArrowDropDown,
                              contentDescription = null,
                            )
                          }
                        }
                      }
                    )
                  }
                  GDDropdownMenu(
                    modifier = Modifier
                      .width(textFieldWidth.value)
                      .heightIn(max=layoutParams.askMaxDropdownMenuHeight)
                      .background(SurfaceColorAtElevation(6.dp)), // a hack since dialog background color not accessible
                    expanded = editTextTagListExpanded.value,
                    onDismissRequest = { editTextTagListExpanded.value = false }
                  ) {
                    for (tag in viewModel.askEditTextTagList) {
                      GDDropdownMenuItem(
                        onClick = {
                          editTextTag.value=tag
                          editTextTagListExpanded.value = false
                        }
                      ) {
                        Text(tag)
                      }
                    }
                  }
                }
              }
            }
          }
        )
      } else if (viewModel.askMultipleChoiceList.isNotEmpty()) {
        askAlertDialog(
          viewModel = viewModel,
          confirmHandler = {
            viewModel.askMultipleChoiceConfirmHandler(viewModel.getSelectedChoices())
          },
          content = {
            Column(
              modifier = Modifier
                .fillMaxWidth()
                .heightIn(max = height - layoutParams.dialogButonRowHeight)
            ) {
              if (viewModel.askMessage != "") {
                Text(
                  modifier = Modifier,
                  text = viewModel.askMessage,
                )
                Spacer(Modifier.height(layoutParams.askMultipleChoiceMessageOffset))
              }
              LazyColumn(
                modifier = Modifier
                  .fillMaxWidth()
              ) {
                itemsIndexed(viewModel.askMultipleChoiceList) { index, item ->
                  val checked = remember { mutableStateOf(item.checked) }
                  Row(
                    modifier = Modifier
                      .fillMaxWidth()
                      .wrapContentHeight(),
                    verticalAlignment = Alignment.CenterVertically
                  ) {
                    Checkbox(
                      checked = checked.value,
                      onCheckedChange = {
                        checked.value = it
                        item.checked = it
                        viewModel.askMultipleChoiceCheckedHandler()
                      }
                    )
                    Text(
                      text = item.text,
                      softWrap = false,
                      overflow = TextOverflow.Ellipsis,
                      style = MaterialTheme.typography.bodyLarge
                    )
                  }
                }
              }
            }
          }
        )
      } else if (viewModel.askSingleChoiceList.isNotEmpty()) {
        askAlertDialog(
          viewModel = viewModel,
          content = {
            Column(
              modifier = Modifier
                .fillMaxWidth()
                .heightIn(max = height - layoutParams.dialogButonRowHeight)
            ) {
              if (viewModel.askMessage != "") {
                Text(
                  modifier = Modifier,
                  text = viewModel.askMessage
                )
                Spacer(Modifier.height(layoutParams.askMultipleChoiceMessageOffset))
              }
              LazyColumn(
                modifier = Modifier
                  .fillMaxWidth(),
              ) {
                itemsIndexed(viewModel.askSingleChoiceList) { index, item ->
                  TextButton(
                    onClick = {
                      viewModel.askSingleChoiceConfirmHandler(item)
                      viewModel.closeQuestion()
                    }
                  ) {
                    Text(
                      text = item,
                      softWrap = false,
                      overflow = TextOverflow.Ellipsis,
                      style = MaterialTheme.typography.bodyLarge
                    )
                  }
                }
              }
            }
          }
        )
      } else {
        askAlertDialog(
          viewModel = viewModel,
          confirmHandler = {
            viewModel.askQuestionConfirmHandler()
          },
          content = {
            Text(
              text = viewModel.askMessage,
              style = MaterialTheme.typography.bodyLarge
            )
          }
        )
      }
    } else {
      viewModel.allAskAlertDialogsClosed()
    }
  }

  // Alert dialog for multiple use cases
  @Composable
  private fun askAlertDialog(
    viewModel: ViewModel,
    confirmHandler: () -> Unit = {},
    content: @Composable () -> Unit
  ) {
    AlertDialog(
      modifier = Modifier
        .wrapContentHeight(),
      onDismissRequest = {
        viewModel.closeQuestion()
      },
      confirmButton = {
        if(viewModel.askConfirmText!="") {
          TextButton(
            modifier = Modifier,
            onClick = {
              confirmHandler()
              viewModel.closeQuestion()
            }
          ) {
            Text(viewModel.askConfirmText)
          }
        }
      },
      dismissButton = {
        if(viewModel.askDismissText!="") {
          TextButton(
            onClick = {
              viewModel.askQuestionDismissHandler()
              viewModel.closeQuestion()
            }
          ) {
            Text(viewModel.askDismissText)
          }
        }
      },
      title = {
        Text(text = viewModel.askTitle)
      },
      text = {
        content()
      }
    )
  }
}

