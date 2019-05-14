//============================================================================
// Name        : GDAddressHistoryAdapter.java
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

package com.untouchableapps.android.geodiscoverer;

import android.content.res.ColorStateList;
import android.opengl.Visibility;
import android.support.v4.graphics.drawable.DrawableCompat;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.TextView;

import com.afollestad.materialdialogs.MaterialDialog;
import com.untouchableapps.android.geodiscoverer.core.GDCore;
import com.untouchableapps.android.geodiscoverer.core.GDTools;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedList;

import org.apache.commons.lang3.StringUtils;
import org.apache.commons.text.StringEscapeUtils;

public class GDAddressHistoryAdapter extends ArrayAdapter<String> {

  private final ViewMap viewMap;
  private final MaterialDialog dialog;
  private GDCore coreObject;
  private ColorStateList itemIconTint;
  private ListView listView;
  public LinkedList<String> groupNames;
  public ArrayAdapter<String> groupNamesAdapter;
  public String selectedGroupName;

  static class ViewHolder {
    public LinearLayout row;
    public TextView text;
    public EditText editText;
    public LinearLayout editor;
    public Spinner group;
    public ImageButton removeButton;
    public ImageButton editButton;
    public ImageButton clearButton;
    public ImageButton confirmButton;
  }

  private void closeRowEditor(ViewHolder holder) {
    holder.editor.setVisibility(View.GONE);
    holder.confirmButton.setVisibility(View.GONE);
    holder.clearButton.setVisibility(View.GONE);
    holder.text.setVisibility(View.VISIBLE);
    holder.removeButton.setVisibility(View.VISIBLE);
    holder.editButton.setVisibility(View.VISIBLE);
    updateListViewSize(holder);
  }

  private void closeAllRowEditors() {
    final int firstListItemPosition = listView.getFirstVisiblePosition();
    final int lastListItemPosition = firstListItemPosition + listView.getChildCount() - 1;
    for (int i=0;i<listView.getChildCount();i++) {
      View rowView = listView.getChildAt(i);
      if (rowView!=null) {
        ViewHolder holder = (ViewHolder) rowView.getTag();
        closeRowEditor(holder);
      }
    }
  }

  private void updateListViewSize(ViewHolder holder) {
    int desiredWidth = View.MeasureSpec.makeMeasureSpec(listView.getWidth(), View.MeasureSpec.AT_MOST);
    holder.row.measure(desiredWidth, View.MeasureSpec.UNSPECIFIED);
    int diff = holder.row.getMeasuredHeight()-holder.row.getHeight();
    ViewGroup.LayoutParams params = listView.getLayoutParams();
    if (listView.getHeight()<holder.row.getMeasuredHeight()) {
      params.height = listView.getHeight() + diff;
    } else {
      params.height = -1;
    }
    listView.setLayoutParams(params);
    listView.requestLayout();
  }

  public GDAddressHistoryAdapter(ViewMap viewMap, MaterialDialog dialog, ListView listView) {
    super(viewMap, R.layout.dialog_address_history_entry);

    // Remember variables
    coreObject=GDApplication.coreObject;
    this.viewMap = viewMap;
    this.dialog = dialog;
    this.itemIconTint = viewMap.navDrawerList.getItemIconTintList();
    this.listView = listView;
    groupNames = new LinkedList<String>();
    groupNamesAdapter = new ArrayAdapter<String>(getContext(), R.layout.dialog_address_history_group_entry);

    // Get the address list and sort it according to time
    updateAddresses();

    // Create the group names adapter
    if (!groupNames.contains("Default")) {
      addGroupName("Default");
    }
  }

  private void updateAddressesInternal() {
    selectedGroupName = coreObject.configStoreGetStringValue("Navigation","selectedAddressPointGroup");
    clear();
    String temp[] = coreObject.configStoreGetAttributeValues("Navigation/AddressPoint","name");
    LinkedList<String> unsortedNames = new LinkedList<String>(Arrays.asList(temp));
    while (unsortedNames.size()>0) {
      String newestName="";
      long newestTimestamp = 0;
      for (String name : unsortedNames) {
        String path = "Navigation/AddressPoint[@name='" + name + "']";
        long t = Long.parseLong(coreObject.configStoreGetStringValue(path, "timestamp"));
        if (t > newestTimestamp) {
          newestTimestamp = t;
          newestName = name;
        }
      }
      String groupName = coreObject.configStoreGetStringValue("Navigation/AddressPoint[@name='" + newestName + "']","group");
      if (!groupNames.contains(groupName)) {
        addGroupName(groupName);
      }
      if (groupName.equals(selectedGroupName))
        add(newestName);
      unsortedNames.remove(newestName);
    }
    notifyDataSetChanged();
  }

  public void updateAddresses() {
    closeAllRowEditors();
    updateAddressesInternal();
  }

  public void addGroupName(String groupName) {
    if ((groupName!="")&&(!groupNames.contains(groupName))) {
      closeAllRowEditors();
      groupNames.add(groupName);
      Collections.sort(groupNames);
      groupNamesAdapter.clear();
      groupNamesAdapter.addAll(groupNames);
      groupNamesAdapter.notifyDataSetChanged();
    }
  }

  public void removeGroupName(String groupName) {
    if ((groupName!="")&&(groupNames.contains(groupName))&&(!groupName.equals("Default"))) {

      // Remove the group name from the list
      groupNames.remove(groupName);
      groupNamesAdapter.remove(groupName);

      // Go through all addresses that use this group name and reset them to default
      for (int i=0;i<getCount();i++) {
        String address = getItem(i);
        String path = "Navigation/AddressPoint[@name='"+address+"']";
        String addressGroupName = coreObject.configStoreGetStringValue(path,"group");
        if (addressGroupName.equals(groupName)) {
          coreObject.configStoreSetStringValue(path,"group","Default");
        }
      }

      // Close all rows that are in edit mode
      closeAllRowEditors();

      // Inform all spinners that data set has changed
      groupNamesAdapter.notifyDataSetChanged();
    }
  }

  @Override
  public View getView(final int position, View convertView, ViewGroup parent) {
    View rowView = convertView;

    // Reuse views
    if (rowView == null) {
      LayoutInflater inflater = viewMap.getLayoutInflater();
      rowView = inflater.inflate(R.layout.dialog_address_history_entry, null);
      ViewHolder viewHolder = new ViewHolder();
      viewHolder.row = (LinearLayout) rowView;
      viewHolder.text = (TextView) rowView.findViewById(R.id.dialog_address_history_entry_text);
      viewHolder.editText = (EditText) rowView.findViewById(R.id.dialog_address_history_entry_edit_text);
      viewHolder.editText.setImeOptions(EditorInfo.IME_ACTION_DONE);
      viewHolder.removeButton = (ImageButton) rowView.findViewById(R.id.dialog_address_history_entry_remove_button);
      DrawableCompat.setTintList(viewHolder.removeButton.getDrawable(), itemIconTint);
      viewHolder.editButton = (ImageButton) rowView.findViewById(R.id.dialog_address_history_entry_edit_button);
      DrawableCompat.setTintList(viewHolder.editButton.getDrawable(), itemIconTint);
      viewHolder.confirmButton = (ImageButton) rowView.findViewById(R.id.dialog_address_history_entry_confirm_button);
      DrawableCompat.setTintList(viewHolder.confirmButton.getDrawable(), itemIconTint);
      viewHolder.clearButton = (ImageButton) rowView.findViewById(R.id.dialog_address_history_entry_clear_button);
      DrawableCompat.setTintList(viewHolder.clearButton.getDrawable(), itemIconTint);
      viewHolder.editor = (LinearLayout) rowView.findViewById(R.id.dialog_address_history_entry_editor);
      DrawableCompat.setTintList(((ImageView)rowView.findViewById(R.id.dialog_address_history_entry_group_image)).getDrawable(), itemIconTint);
      viewHolder.group = (Spinner) rowView.findViewById(R.id.dialog_address_history_entry_group_spinner);
      rowView.setTag(viewHolder);
    }

    // Fill data
    final ViewHolder holder = (ViewHolder) rowView.getTag();
    final String name = getItem(position);
    holder.text.setText(name);
    holder.removeButton.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        coreObject.scheduleCoreCommand("removeAddressPoint",name);
        remove(name);
      }
    });
    holder.text.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        coreObject.scheduleCoreCommand("setTargetAtAddressPoint",name);
        dialog.dismiss();
      }
    });
    holder.editButton.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        holder.group.setAdapter(groupNamesAdapter);
        String groupName = coreObject.configStoreGetStringValue("Navigation/AddressPoint[@name='"+holder.text.getText().toString()+"']","group");
        holder.group.setSelection(groupNamesAdapter.getPosition(groupName));
        holder.editor.setVisibility(View.VISIBLE);
        holder.confirmButton.setVisibility(View.VISIBLE);
        holder.clearButton.setVisibility(View.VISIBLE);
        holder.text.setVisibility(View.GONE);
        holder.removeButton.setVisibility(View.GONE);
        holder.editButton.setVisibility(View.GONE);
        holder.editText.setText(name);
        updateListViewSize(holder);
      }
    });
    holder.clearButton.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        holder.editText.setText("");
      }
    });
    final View.OnClickListener confirmOnClickListener = new View.OnClickListener() {
      @Override
      public void onClick(View v) {

        // Skip if edit text is empty or equal
        String newName = holder.editText.getText().toString();
        if ((!newName.equals("")) && (!newName.equals(name))) {

          // Rename the entry
          newName = coreObject.executeCoreCommand("renameAddressPoint",name, newName);
          remove(name);
          insert(newName, position);
          holder.text.setText(newName);

          // Remove any duplicates
          ArrayList<String> duplicates = new ArrayList<String>();
          for (int i = 0; i < getCount(); i++) {
            String otherName = getItem(i);
            if ((otherName != newName) && (otherName.equals(newName))) {
              duplicates.add(otherName);
            }
          }
          for (String otherName : duplicates)
            remove(otherName);

        }

        // Change the group if another one is selected
        String group = groupNamesAdapter.getItem(holder.group.getSelectedItemPosition());
        if (!group.equals(selectedGroupName)) {
          String path = "Navigation/AddressPoint[@name='"+newName+"']";
          coreObject.configStoreSetStringValue(path,"group",group);
          coreObject.executeCoreCommand("addressPointGroupChanged");
          remove(newName);
        }

        // Make the original views visible again
        closeRowEditor(holder);
        updateAddressesInternal();
      }
    };
    holder.confirmButton.setOnClickListener(confirmOnClickListener);
    holder.editText.setOnEditorActionListener(new EditText.OnEditorActionListener() {
      @Override
      public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
        if (actionId == EditorInfo.IME_ACTION_DONE) {
          confirmOnClickListener.onClick(null);
          return true;
        }
        return false;
      }
    });
    updateListViewSize(holder);
    return rowView;
  }
}
