#============================================================================
# Name        : GrabMaps.py
# Author      : Matthias Gruenewald
# Copyright   : Copyright 2014 Matthias Gruenewald
#
# This file is part of GrabMaps.
#
# GrabMaps is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# GrabMaps is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GrabMaps. If not, see <http://www.gnu.org/licenses/>.
#
#============================================================================

import win32process
import win32api
import win32gui
import win32con
import win32clipboard
import win32com.client
import pywin.mfc.dialog
import time
import os
import struct
import sys
import math
import Image
from threading import Thread
import Tkinter
import socket
import pprint

# Stores the coordinates of the map
class map_info:

    def __init__(self):
        self.north=float(0)
        self.west=float(0)
        self.south=float(0)
        self.east=float(0)
        self.upper_left_lng_x=float(0)
        self.upper_left_lat_y=float(0)
        self.upper_right_lng_x=float(0)
        self.upper_right_lat_y=float(0)
        self.lower_left_lng_x=float(0)
        self.lower_left_lat_y=float(0)
        self.lower_right_lng_x=float(0)
        self.lower_right_lat_y=float(0)
        self.center_lng_x=float(0)
        self.center_lat_y=float(0)
        self.width=0
        self.height=0
        #self.zoom_level=0
        self.lng_per_pixel=float(0)
        self.lat_per_pixel=float(0)
        self.scale_x=float(0)
        self.scale_y=float(0)

# Performs the grabbing
class grabber(Thread):

    def __init__(self, debug, mode, map_folder, map_file_name, initial_zoom, zoom_steps):

        Thread.__init__(self)

        # Global arguments
        self.mode = mode
        if (mode=="Google Maps"):
            self.app_win_title = "Google Maps - Mozilla Firefox"
        if (mode=="Deutschland Digital"):
            self.app_win_title = "Deutschland Digital - Landkarte 1:25000"            
        self.map_folder = map_folder
        self.map_file_name = map_file_name
        self.initial_zoom = initial_zoom
        self.zoom_steps = zoom_steps
        self.minicap_path = r'C:\Programme\MiniCap\MiniCap.exe'
        self.scale=1                                     # Scale to apply to the images

        if (mode=="Google Maps"):        
            self.offset_to_map_left=0*self.scale         # Distance to the map from the left of the screen shot
            self.offset_to_map_top=119*self.scale        # Distance to the map from the top of the screen shot
            self.offset_to_map_right=0*self.scale        # Distance to the map from the left of the screen shot
            self.offset_to_map_bottom=0*self.scale       # Distance to the map from the top of the screen shot
            self.horizontal_map_border=30*self.scale     # Number of pixels to remove from image at the lower and upper part
            self.vertical_map_border=64*self.scale       # Number of pixels to remove from image at the left and right part
            self.required_map_width=800*self.scale       # Width of the final map
            self.required_map_height=800*self.scale      # Height of the final map

        if (mode=="Deutschland Digital"):
            self.offset_to_map_left=6*self.scale         # Distance to the map from the left of the screen shot
            self.offset_to_map_top=104*self.scale        # Distance to the map from the top of the screen shot
            self.offset_to_map_right=27*self.scale       # Distance to the map from the left of the screen shot
            self.offset_to_map_bottom=48*self.scale      # Distance to the map from the top of the screen shot
            self.horizontal_map_border=0*self.scale      # Number of pixels to remove from image at the lower and upper part
            self.vertical_map_border=0*self.scale        # Number of pixels to remove from image at the left and right part
            self.required_map_width=1024*self.scale      # Width of the final map
            self.required_map_height=768*self.scale      # Height of the final map
            
        self.map_overlap_x=160*self.scale                # Number of pixels to overlap the maps in the horizontal direction
        self.map_overlap_y=160*self.scale                # Number of pixels to overlap the maps in the vertical direction
        self.msg_len=256                                 # Size of the message buffer for socket connections
        self.debug=debug                                 # Set to one to enable debug mode
        self.stop=0                                      # Stop indication
        
        # Init
        self.shell = win32com.client.Dispatch("WScript.Shell")

    def set_status(self, content):
        status.configure(text=content)
        print content
        sys.stdout.flush()

    def exit(self):
        if self.mode=="Google Maps":
            if self.repl:
                self.repl.close()
        self.set_status("Ready.")
        grab_button.configure(text="Grab!")
        grab_button.configure(state=Tkinter.ACTIVE)
        sys.exit()        

    def get_hwnd_at_depth ( self, parent_hwnd, target_depth, current_depth ):
        childlist = []
        win32gui.EnumChildWindows( parent_hwnd, lambda hwnd,oldlist: oldlist.append( hwnd ), childlist )
        for child_hwnd in childlist:
            if win32gui.IsWindowVisible(child_hwnd):
                if target_depth==current_depth:
                    return child_hwnd
                else:
                    return self.get_hwnd_at_depth(child_hwnd,target_depth,current_depth+1)
        return 0

    def send_mouse_click(self,hwnd,x,y):
        time.sleep(2)
        lParam = struct.unpack( 'L', struct.pack( 'hh', x, y ) ) [0]
        #lParam = (y << 16) | x
        win32api.SendMessage(hwnd, win32con.WM_LBUTTONDOWN, 1, lParam)
        win32api.SendMessage(hwnd, win32con.WM_LBUTTONUP, 0, lParam)

    def pan_map_right(self,hwnd):
        if self.mode=="Deutschland Digital":
            win32api.PostMessage(hwnd, win32con.WM_KEYDOWN, 0x27, 0x14D0001)
            win32api.PostMessage(hwnd, win32con.WM_KEYUP, 0x27, 0xC14D0001)
            self.wait_for_map()
    
    def pan_map_left(self,hwnd):
        if self.mode=="Deutschland Digital":
            win32api.PostMessage(hwnd, win32con.WM_KEYDOWN, 0x25, 0x14B0001)
            win32api.PostMessage(hwnd, win32con.WM_KEYUP, 0x25, 0xC14B0001)
            self.wait_for_map()

    def pan_map_up(self,hwnd):
        if self.mode=="Deutschland Digital":
            win32api.PostMessage(hwnd, win32con.WM_KEYDOWN, 0x26, 0x1480001)
            win32api.PostMessage(hwnd, win32con.WM_KEYUP, 0x26, 0xC1480001)
            self.wait_for_map()

    def pan_map_down(self,hwnd):
        if self.mode=="Deutschland Digital":
            win32api.PostMessage(hwnd, win32con.WM_KEYDOWN, 0x28, 0x1500001)
            win32api.PostMessage(hwnd, win32con.WM_KEYUP, 0x28, 0xC1500001)
            self.wait_for_map()

    def zoom_map_in(self,hwnd):
        if self.mode=="Google Maps":
            self.send_mouse_click(hwnd,37,280)
        if self.mode=="Deutschland Digital":
            win32api.PostMessage(hwnd, win32con.WM_KEYDOWN, 0xBB, 0x1B0001)
            #win32api.PostMessage(hwnd, win32con.WM_CHAR, 0x2B, 0x1B0001)
            win32api.PostMessage(hwnd, win32con.WM_KEYUP, 0xBB, 0xC01B0001)

    # Save the map into a file
    def create_map(self,hwnd,file_name):

        # Create snapshot
        cmd='%s -save "%s\\%s.png" -capturehwnd %d -exit' % ( self.minicap_path, self.map_folder, file_name, hwnd )
        #print cmd
        self.shell.Run(cmd,0,1)
        
        # Cut out the map
        t='%s\\%s.png' % (self.map_folder, file_name)
        im = Image.open(t)
        if (self.scale>0):
            im = im.resize((self.scale*im.size[0],self.scale*im.size[1]))        
        x0=self.offset_to_map_left+self.vertical_map_border
        x1=im.size[0]-self.offset_to_map_right-self.vertical_map_border
        y0=self.offset_to_map_top+self.horizontal_map_border
        y1=im.size[1]-self.offset_to_map_bottom-self.horizontal_map_border
        box=(x0,y0,x1,y1)
        map = im.crop(box)
        map = map.convert('RGB').convert('P', palette=Image.ADAPTIVE, colors=256)
        map.save('%s\\%s.png' % (self.map_folder,file_name),"PNG",optimize=1)        
        #map.save('%s\\%s.jpg' % (self.map_folder,file_name),"JPEG",quality=75)

        # Do some sanity checking
        if map.size[0]!=self.required_map_width or map.size[1]!=self.required_map_height:
            win32gui.MessageBox(0, "Dimension of map (%dx%d) does not match the required values (%dx%d)!" % (map.size[0],map.size[1],self.required_map_width,self.required_map_height), "GrabMaps", win32con.MB_OK)
            self.exit()

        # Remove temp files
        #system.exit(1)
        #map.show()        
        #os.remove('%s\\%s.png' % (self.map_folder,file_name))

    # Creates the calibration file
    def create_calibration_file(self, file_name, map_info, zoom_step):
        calfile='%s\\%s.gdm' % ( self.map_folder, file_name )
        fh = open(calfile,"w")
        fh.write('<?xml version="1.0" encoding="UTF-8"?>\n')
        fh.write('<GDM version="1.0">\n')        
        fh.write('  <imageFileName>%s.png</imageFileName>\n' % ( file_name ))
        fh.write('  <zoomLevel>%d</zoomLevel>\n' % ( zoom_step ))
        fh.write('  <calibrationPoint>\n')
        fh.write('    <x>%d</x>\n' % (0))
        fh.write('    <y>%d</y>\n' % (0))
        fh.write('    <longitude>%f</longitude>\n' % (map_info.upper_left_lng_x))
        fh.write('    <latitude>%f</latitude>\n' % (map_info.upper_left_lat_y))
        fh.write('  </calibrationPoint>\n')
        fh.write('  <calibrationPoint>\n')
        fh.write('    <x>%d</x>\n' % (map_info.width-1))
        fh.write('    <y>%d</y>\n' % (0))
        fh.write('    <longitude>%f</longitude>\n' % (map_info.upper_right_lng_x))
        fh.write('    <latitude>%f</latitude>\n' % (map_info.upper_right_lat_y))
        fh.write('  </calibrationPoint>\n')
        fh.write('  <calibrationPoint>\n')
        fh.write('    <x>%d</x>\n' % (map_info.width-1))
        fh.write('    <y>%d</y>\n' % (map_info.height-1))
        fh.write('    <longitude>%f</longitude>\n' % (map_info.lower_right_lng_x))
        fh.write('    <latitude>%f</latitude>\n' % (map_info.lower_right_lat_y))
        fh.write('  </calibrationPoint>\n')
        fh.write('  <calibrationPoint>\n')
        fh.write('    <x>%d</x>\n' % (0))
        fh.write('    <y>%d</y>\n' % (map_info.height-1))
        fh.write('    <longitude>%f</longitude>\n' % (map_info.lower_left_lng_x))
        fh.write('    <latitude>%f</latitude>\n' % (map_info.lower_left_lat_y))
        fh.write('  </calibrationPoint>\n')
        fh.write('  <calibrationPoint>\n')
        fh.write('    <x>%d</x>\n' % (map_info.width/2-1))
        fh.write('    <y>%d</y>\n' % (map_info.height/2-1))
        fh.write('    <longitude>%f</longitude>\n' % (map_info.center_lng_x))
        fh.write('    <latitude>%f</latitude>\n' % (map_info.center_lat_y))
        fh.write('  </calibrationPoint>\n')
        fh.write('</GDM>\n')        
        fh.close()

    # Copies the coordinates to the clipboard
    def get_coordinates_at_position(self, control_hwnd, x, y):

        # Clear clipboard
        win32clipboard.OpenClipboard(None)
        win32clipboard.EmptyClipboard()
        win32clipboard.SetClipboardText("failed")
        fail_text=win32clipboard.GetClipboardData(win32con.CF_TEXT)
        c=fail_text
        win32clipboard.CloseClipboard()

        # Copy the coordinates in the clipboard
        while c==fail_text:
            lParam_coordinates = struct.unpack( 'L', struct.pack( 'hh', x, y ) ) [0]
            win32api.PostMessage(control_hwnd, win32con.WM_RBUTTONDOWN, 2, lParam_coordinates)
            win32api.PostMessage(control_hwnd, win32con.WM_MOUSEMOVE, 2, lParam_coordinates)
            win32api.PostMessage(control_hwnd, win32con.WM_RBUTTONUP, 0, lParam_coordinates)
            time.sleep(1)
            self.shell.SendKeys("k")
            time.sleep(1)
            win32clipboard.OpenClipboard(None)
            c=win32clipboard.GetClipboardData(win32con.CF_TEXT)        
            win32clipboard.CloseClipboard()

        # Convert the clipboard text into coordinates        
        c2=c.partition("\t")
        #print c2
        long_x=float(c2[0])
        lat_y=float(c2[2])
        return (long_x,lat_y)

    # Get the coordinate of the map's center
    def get_coordinates(self, hwnd):

        if self.mode=="Google Maps":

            # Get the coordinates of the corners and the center
            bounds=self.repl_cmd(r"content.wrappedJSObject.gApplication.getMap().getBounds()")
            bounds=bounds.replace("(","")
            bounds=bounds.replace("))",",")
            bounds=bounds.replace(")","")
            t=bounds.split(",")
            i=map_info()
            i.north=float(t[2])
            i.west=float(t[1])
            i.south=float(t[0])
            i.east=float(t[3])
            center=self.repl_cmd(r"content.wrappedJSObject.gApplication.getMap().getCenter()")
            center=center.replace(")",",")
            center=center.replace("(","")
            t=center.split(",")
            i.center_lat_y=float(t[0])
            i.center_lng_x=float(t[1])

            # Set the corners
            i.upper_left_lng_x=i.west
            i.upper_left_lat_y=i.north
            i.upper_right_lng_x=i.east
            i.upper_right_lat_y=i.north
            i.lower_left_lng_x=i.west
            i.lower_left_lat_y=i.south
            i.lower_right_lng_x=i.east
            i.lower_right_lat_y=i.south

            # Recalculate the bounds and set the pixel coordinates
            i.width=self.required_map_width
            i.height=self.required_map_height
            width=(self.required_map_width+self.offset_to_map_left+self.offset_to_map_right+2*self.vertical_map_border)
            i.lng_per_pixel=(i.east-i.west)/float(width)
            i.west=i.west+i.lng_per_pixel*float(self.vertical_map_border)
            i.east=i.east-i.lng_per_pixel*float(self.vertical_map_border+1)
            height=(self.required_map_height+self.offset_to_map_top+self.offset_to_map_bottom+2*self.horizontal_map_border)
            i.lat_per_pixel=(i.south-i.north)/float(height)
            i.north=i.north+i.lat_per_pixel*float(self.horizontal_map_border)
            i.south=i.south-i.lat_per_pixel*float(self.horizontal_map_border+1)

            if (self.stop==1):
                self.exit()

        if self.mode=="Deutschland Digital":

            # Get the coordinates            
            i=map_info()
            (i.upper_left_lng_x,i.upper_left_lat_y)=self.get_coordinates_at_position(hwnd,0,0)
            if (self.stop==1):
                self.exit()
            (i.upper_right_lng_x,i.upper_right_lat_y)=self.get_coordinates_at_position(hwnd,self.required_map_width-1,0)
            if (self.stop==1):
                self.exit()
            (i.lower_left_lng_x,i.lower_left_lat_y)=self.get_coordinates_at_position(hwnd,0,self.required_map_height-1)
            if (self.stop==1):
                self.exit()
            (i.lower_right_lng_x,i.lower_right_lat_y)=self.get_coordinates_at_position(hwnd,self.required_map_width-1,self.required_map_height-1)
            if (self.stop==1):
                self.exit()
            (i.center_lng_x,i.center_lat_y)=self.get_coordinates_at_position(hwnd,self.required_map_width/2-1,self.required_map_height/2-1)
            if (self.stop==1):
                self.exit()
            i.north=float((i.upper_left_lat_y+i.upper_right_lat_y)/2)
            i.south=float((i.lower_left_lat_y+i.lower_right_lat_y)/2)
            i.west=float((i.upper_left_lng_x+i.lower_left_lng_x)/2)
            i.east=float((i.upper_right_lng_x+i.lower_right_lng_x)/2)
            
            # Recalculate the bounds and set the pixel coordinates
            i.width=self.required_map_width
            i.height=self.required_map_height
            i.lng_per_pixel=(i.east-i.west)/float(i.width)
            i.lat_per_pixel=(i.south-i.north)/float(i.height)
            #i.west=float(i.west-1.0*i.lng_per_pixel)
            #i.north=float(i.north-1.0*i.lat_per_pixel)
            #i.lng_per_pixel=(i.east-i.west)/float(i.width)
            #i.lat_per_pixel=(i.south-i.north)/float(i.height)

        # Calculate the scale level
        i.scale_x=float(i.width)/math.fabs(i.west-i.east)
        i.scale_y=float(i.height)/math.fabs(i.north-i.south)

        return i
    
    # Create and calibrates a map
    def create_and_calibrate_map(self, map_hwnd, control_hwnd, file_name, zoom_step, x, y):

        # Output status        
        file_name="%s_%d_%d_%d" % (file_name,zoom_step,x,y)
        #print file_name

        # Get the coordinates
        if self.mode=="Deutschland Digital":
            i=self.get_coordinates(control_hwnd)
        if self.mode=="Google Maps":
            i=self.get_coordinates(map_hwnd)

        # Create the map and the calibration file
        self.create_map(map_hwnd,file_name)
        self.create_calibration_file(file_name,i,zoom_step)
        return i

    def repl_read(self, end_marker):
        msg = ''
        self.repl.setblocking(0)
        counter=0
        max_counter=100
        while counter<max_counter:
            try:
                chunk=""
                chunk = self.repl.recv(self.msg_len)
            except:
                counter=counter+1
            msg = msg + chunk
            if (msg.endswith(end_marker)):
                break
            else:
                time.sleep(0.1)
            counter=counter+1
        self.repl.setblocking(1)
        print msg
        #for c in msg:
        #    print "%#x" % ord(c),
        if counter>=max_counter:
            win32gui.MessageBox(0, "Could not receive result from MozRepl! Please restart Firefox and try again.", "GrabMaps", win32con.MB_OK)
            exit()
        return msg

    def repl_cmd(self,cmd):
        self.repl.send(cmd)
        result=self.repl_read("repl> ")
        return result

    def wait_for_map(self):
        
        if self.mode=="Google Maps":
            found=0
            while found==0:
                msg=self.repl_cmd(r"content.wrappedJSObject.gApplication.getMap().isLoaded()")
                if msg=="true\nrepl> ":
                    found=1
        if self.mode=="Deutschland Digital":
            time.sleep(1)
    

    def pan_map_to(self, hwnd, x, y):
        
        if self.mode=="Google Maps":
            self.repl_cmd(r"repl.enter(content.wrappedJSObject)")
            self.repl_cmd(r"repl.enter(gApplication)")
            self.repl_cmd(r"repl.enter(getMap())")
            self.repl_cmd("panTo(new GLatLng(%f,%f))" % ( y, x ))
            self.repl_cmd(r"repl.home()")
            time.sleep(3)

        if self.mode=="Deutschland Digital":

            # Find out where we are
            i=self.get_coordinates(hwnd)

            # First go the horizontal direction
            long_x=i.west
            while x<long_x:
                self.pan_map_left(hwnd)
                (long_x, lat_y)=self.get_coordinates_at_position(hwnd,0,self.required_map_height/2-1)
            long_x=i.east
            while x>long_x:
                self.pan_map_right(hwnd)
                (long_x, lat_y)=self.get_coordinates_at_position(hwnd,self.required_map_width-1,self.required_map_height/2-1)
            lat_y=i.north
            while y>lat_y:
                self.pan_map_up(hwnd)
                (long_x, lat_y)=self.get_coordinates_at_position(hwnd,self.required_map_width/2-1,0)
            lat_y=i.south
            while y<lat_y:
                self.pan_map_down(hwnd)
                (long_x, lat_y)=self.get_coordinates_at_position(hwnd,self.required_map_width/2-1,self.required_map_height-1)
                
    def run(self):

        # Deactivate button
        #grab_button.configure(state=Tkinter.DISABLED)

        # Open the connection to Firefox's repl
        if self.mode=="Google Maps":
            self.repl = socket.socket()
            success=0
            while success==0:
                try:
                    self.repl.connect(("127.0.0.1", 4242))
                    success=1
                except:
                    win32gui.MessageBox(0, "Please start Firefox and ensure that the MozRepl server is active on port 4242 . Press OK when finished.", "GrabMaps", win32con.MB_OK)
                    success=0
            self.repl_read("repl> ")

        # Go to Google Maps
        if self.mode=="Google Maps":
            href=self.repl_cmd(r"content.location.href")
            if not href.startswith(r'"http://maps.google.com'):
                self.repl_cmd(r"content.location.href = 'http://maps.google.com'")
                self.wait_for_map()

        # Get the window handles
        map_hwnd=None
        control_hwnd=None
        if self.mode=="Google Maps":
            hwnd = win32gui.FindWindowEx(0, 0, 0, self.app_win_title)
            if hwnd==0:
                win32gui.MessageBox(0, "Could not retrieve handle of Firefox's window!", "GrabMaps", win32con.MB_OK)
                self.exit()
            map_hwnd = self.get_hwnd_at_depth(hwnd,6,1)
            if map_hwnd==0:
                win32gui.MessageBox(0, "Could not retrieve handle of Firefox's map window!", "GrabMaps", win32con.MB_OK)
                self.exit()
            self.shell.AppActivate(self.app_win_title)
        if self.mode=="Deutschland Digital":
            success=0
            while success==0:
                hwnd = win32gui.FindWindowEx(0, 0, 0, self.app_win_title)
                if hwnd==0:
                    win32gui.MessageBox(0, "Please start Deutschland Digital. Press OK when finished.", "GrabMaps", win32con.MB_OK)
                else:
                    success=1
            map_hwnd=hwnd
            childlist = []
            win32gui.EnumChildWindows( hwnd, lambda hwnd,oldlist: oldlist.append( hwnd ), childlist )
            control_hwnd=0
            for child_hwnd in childlist:
                if win32gui.IsWindowVisible(child_hwnd):
                    classname=win32gui.GetClassName(child_hwnd)
                    if classname=="AfxFrameOrView80s":
                        control_hwnd=child_hwnd
            if control_hwnd==0:
                win32gui.MessageBox(0, "Could not retrieve handle of Deutschland Digital's control window!", "GrabMaps", win32con.MB_OK)
                self.exit()                
            self.shell.AppActivate(self.app_win_title)
        
        # Get fullscreen map
        if self.mode=="Google Maps":
            self.set_status("Resizing firefox...")
            self.repl_cmd(r"window.resizeTo(%d,%d)" % (146+self.required_map_width,336+self.required_map_height))
        if self.mode=="Deutschland Digital":
            self.set_status("Resizing Deutschland Digital...")
            win32gui.SetWindowPos(map_hwnd,win32con.HWND_TOP,200,16,self.required_map_width+33,self.required_map_height+152,0)

        # Wait for the map selection
        self.set_status("Waiting for user...")
        self.shell.AppActivate(self.app_win_title)
        if self.debug==0:
            win32gui.MessageBox(0, "Select the map of interest. Ensure that the complete map is visible. Press OK when finished.", "GrabMaps", win32con.MB_OK)
        self.shell.AppActivate(self.app_win_title)

        # Activate this for calibration
        if 0:
            win32gui.MessageBox(0, "Select center.", "GrabMaps", win32con.MB_OK)
            self.create_map(map_hwnd,"center")
            win32gui.MessageBox(0, "Select upper left.", "GrabMaps", win32con.MB_OK)
            self.create_map(map_hwnd,"upper_left_corner")
            win32gui.MessageBox(0, "Select lower right.", "GrabMaps", win32con.MB_OK)
            self.create_map(map_hwnd,"lower_right_corner")
            self.exit()

        # Grab the map and its border
        self.set_status("Grabbing: z=%d/%d => x=%s/%s, y=%s/%s (%.0f%%)..." % (0,int(self.zoom_steps),0,0,0,0,100.0))
        map_info = self.create_and_calibrate_map(map_hwnd,control_hwnd,self.map_file_name,0,0,0)
        
        # Process the zoom steps if requested
        if int(self.zoom_steps)!=0:

            # First go to the given initial zoom
            for initial_zoom in range(1,int(self.initial_zoom)+1):

                # Zoom one level in
                self.set_status("Setting initial zoom...")
                #print zoom_step
                if self.mode=="Google Maps":
                    self.zoom_map_in(map_hwnd)
                if self.mode=="Deutschland Digital":
                    self.zoom_map_in(control_hwnd)
                self.wait_for_map()
                time.sleep(3)

            # Iterate through zoom levels
            for zoom_step in range(1,int(self.zoom_steps)+1):

                # Zoom one level in
                self.set_status("Zooming in...")
                #print zoom_step
                if self.mode=="Google Maps":
                    self.zoom_map_in(map_hwnd)
                if self.mode=="Deutschland Digital":
                    self.zoom_map_in(control_hwnd)
                self.wait_for_map()
                time.sleep(3)

                # Go to the upper left corner
                self.set_status("Going to upper left corner...")
                self.pan_map_to(control_hwnd, map_info.west, map_info.north)

                if self.mode=="Google Maps":

                    # Compute the number of slices
                    start=self.get_coordinates(control_hwnd)
                    x_offset=float(start.width-self.map_overlap_x)*start.lng_per_pixel
                    y_offset=float(start.height-self.map_overlap_y)*start.lat_per_pixel
                    slices_x=int(math.ceil(math.fabs(float(map_info.east-map_info.west)/float(x_offset))))
                    slices_y=int(math.ceil(math.fabs(float(map_info.south-map_info.north)/float(y_offset))))

                    # Get all slices
                    total=float(slices_x*slices_y)
                    iteration=1.0
                    first_slice=1
                    for x in range(slices_x):
                        for y in range(slices_y):
                            self.set_status("Grabbing: z=%d/%d => x=%s/%s, y=%s/%s (%.0f%%)..." % (zoom_step,int(self.zoom_steps),x,slices_x-1,y,slices_y-1,iteration/total*100))
                            x_lng=start.center_lng_x+float(x)*x_offset
                            y_lat=start.center_lat_y+float(y)*y_offset
                            self.pan_map_to(control_hwnd,x_lng,y_lat)
                            i=self.create_and_calibrate_map(map_hwnd,control_hwnd,self.map_file_name,zoom_step,x,y)
                            iteration=iteration+1.0

                if self.mode=="Deutschland Digital":

                    # First find out the number of slices in y direction
                    end_found=0
                    slices_y=0
                    slices_x=0
                    x=0
                    y=0
                    while end_found==0:
                        self.set_status("Grabbing: z=%d/%d => x=%s/?, y=%s/? (?%%)..." % (zoom_step,int(self.zoom_steps),x,y))
                        i=self.create_and_calibrate_map(map_hwnd,control_hwnd,self.map_file_name,zoom_step,x,y)
                        slices_y=slices_y+1
                        y=y+1
                        if map_info.south>=i.south:
                            end_found=1
                        else:
                            self.pan_map_down(control_hwnd)
                            self.pan_map_down(control_hwnd)
                    y=y-1

                    # Then find out the number of slices in x direction
                    end_found=0
                    self.pan_map_right(control_hwnd)
                    self.pan_map_right(control_hwnd)
                    x=1
                    slices_x=1
                    while end_found==0:
                        self.set_status("Grabbing: z=%d/%d => x=%s/?, y=%s/%s (?%%)..." % (zoom_step,int(self.zoom_steps),x,y,slices_y-1))
                        i=self.create_and_calibrate_map(map_hwnd,control_hwnd,self.map_file_name,zoom_step,x,y)
                        slices_x=slices_x+1
                        x=x+1
                        if map_info.east<=i.east:
                            end_found=1
                        else:
                            self.pan_map_right(control_hwnd)
                            self.pan_map_right(control_hwnd)
                    x=x-1

                    # Get the remaining one
                    total=float(slices_x*slices_y)
                    iteration=slices_y+slices_x
                    direction=-1
                    x_end=0
                    y=y-1
                    self.pan_map_up(control_hwnd)
                    while y>=0:
                        while x<>x_end:
                            self.set_status("Grabbing: z=%d/%d => x=%s/%s, y=%s/%s (%.0f%%)..." % (zoom_step,int(self.zoom_steps),x,slices_x-1,y,slices_y-1,iteration/total*100))
                            i=self.create_and_calibrate_map(map_hwnd,control_hwnd,self.map_file_name,zoom_step,x,y)
                            x=x+direction
                            if x<>x_end:
                                if direction==-1:
                                    self.pan_map_left(control_hwnd)
                                    self.pan_map_left(control_hwnd)
                                else:
                                    self.pan_map_right(control_hwnd)
                                    self.pan_map_right(control_hwnd)
                            iteration=iteration+1.0
                        direction=-1*direction
                        x=x+direction;
                        if direction==1:
                            x_end=slices_x
                        else:
                            x_end=0
                        y=y-1
                        if y>=0:
                            self.pan_map_up(control_hwnd)
                            self.pan_map_up(control_hwnd)

        # That's it
        win32gui.MessageBox(0, "Grabbing finished. Please visually inspect the maps to verify if they are complete.", "GrabMaps", win32con.MB_OK)
        self.exit()
        
def control_grabber():
    global grabber_thread
    create_thread=0
    try:
        grabber_thread
    except NameError:
        create_thread=1
    else:
        if (grabber_thread.isAlive()):
            grabber_thread.stop=1
        else:
            create_thread=1
    if (create_thread):
        grabber_thread=grabber(debug,application.get(application.curselection()),map_folder.get(),map_name.get(),initial_zoom.get(),zoom_steps.get())
        if debug:
            grabber_thread.run()            
        else:
            grabber_thread.start()

# Create the gui
win = Tkinter.Tk()
win.title("GrabMaps")
win.geometry('+1350+16')
f=Tkinter.Frame(win,relief="groove",borderwidth=3)
f.pack()
Tkinter.Label(f, text="Application:").grid(row=0, column=0, sticky=Tkinter.W)
application=Tkinter.Listbox(f,width=30,height=2,selectmode="single")
application.insert(Tkinter.END,"Deutschland Digital")
application.insert(Tkinter.END,"Google Maps")
application.selection_set(0)
application.grid(row=0, column=1, sticky=Tkinter.E)
Tkinter.Label(f, text="Map folder:").grid(row=1, column=0, sticky=Tkinter.W)
map_folder = Tkinter.StringVar()
map_folder.set(r"Z:\Downloads\Maps")
Tkinter.Entry(f, textvariable=map_folder,width=30).grid(row=1, column=1, sticky=Tkinter.E)
Tkinter.Label(f, text="Map name:").grid(row=2, column=0, sticky=Tkinter.W)
map_name = Tkinter.StringVar()
map_name.set(r"Map")
Tkinter.Entry(f, textvariable=map_name,width=30).grid(row=2, column=1, sticky=Tkinter.E)
Tkinter.Label(f, text="Initial zoom:").grid(row=3, column=0, sticky=Tkinter.W)
initial_zoom = Tkinter.StringVar()
initial_zoom.set(r"0")
Tkinter.Entry(f, textvariable=initial_zoom,width=30).grid(row=3, column=1, sticky=Tkinter.E)
Tkinter.Label(f, text="Zoom steps:").grid(row=4, column=0, sticky=Tkinter.W)
zoom_steps = Tkinter.StringVar()
zoom_steps.set(r"0")
Tkinter.Entry(f, textvariable=zoom_steps,width=30).grid(row=4, column=1, sticky=Tkinter.E)
grab_button=Tkinter.Button(win,text="Grab!",width=40,height=1,command=control_grabber)
grab_button.pack()
status=grab_button
debug=0

# Debugging
#debug=1
#control_grabber()

# Do the main loop
win.mainloop()
