#!/usr/bin/perl
#============================================================================
# Name        : GeoTIFF2GDM.pl
# Author      : Matthias Gruenewald
# Copyright   : Copyright 2014 Matthias Gruenewald
#
# This file is part of GeoTIFF2GDM.
#
# GeoTIFF2GDM is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# GeoTIFF2GDM is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GeoTIFF2GDM. If not, see <http://www.gnu.org/licenses/>.
#
#============================================================================

use File::Find;
use Data::Dumper;
use Image::Magick;
use XML::Simple;
use Math::Trig;
use Geo::Proj4;
use POSIX;

# Get arguments
if (scalar @ARGV != 9) {
  die "Usage: GeoTIFF2GDM.pl <GeoTIFF filename> <x> <y> <width> <height> <gravity> <GDM base name> <GDM zoom level> <single image?>";
}
my $geotiff_file = shift;
my $crop_x = shift;
my $crop_y = shift;
my $crop_width = shift;
my $crop_height = shift;
my $crop_gravity = shift;
my $gdm_base = shift;
my $gdm_zoom_level = shift;
my $single_image = shift;

# Extrac the geographic coordinates
print "Extracting geographic coordinates...\n";
my @lng = ( undef, undef, undef );
my @lat = ( undef, undef, undef );
my $proj4args = undef;
open(my $cmd, "listgeo -proj4 -d '$geotiff_file'|");
while (<$cmd>) {
  if ($_ =~ m/Upper Left\s*\((.*),(.*)\)\s*\((.*),(.*)\)/ ) {
    $lng[0] = $3;
    $lat[0] = $4;
  }
  if ($_ =~ m/Upper Right\s*\((.*),(.*)\)\s*\((.*),(.*)\)/ ) {
    $lng[1] = $3;
    $lat[1] = $4;
  }
  if ($_ =~ m/Lower Left\s*\((.*),(.*)\)\s*\((.*),(.*)\)/ ) {
    $lng[2] = $3;
    $lat[2] = $4;
  }
  if ($_ =~ m/PROJ.4 Definition:\s*(.*)\s*/) {
    $proj4args = $1;
  }
}
close($cmd);

# Init the projection
#print $proj4args . "\n";
if (! defined $proj4args) {
  die "Could not extract PROJ.4 arguments of used map projection";
}
my $proj = Geo::Proj4->new($proj4args) or die Geo::Proj4->error;

# Compute the cartesian coordinates of the reference points
my @cartx;
my @carty;
for (my $i=0;$i<3;$i++) {
  if ((! defined $lng[$i]) || (! defined $lat[$i])) {
    die "Could not extract reference geographic coordinates from geo tiff!";
  }
  my ($x, $y) = $proj->forward($lat[$i], $lng[$i]);
  push(@cartx, $x);
  push(@carty, $y);
}
#print Dumper(@lng);
#print Dumper(@lat);
#print Dumper(@cartx);
#print Dumper(@carty);

# Read the image
print "Reading image data...\n";
my $map = Image::Magick->new;
$map->Read($geotiff_file);

# Compute the pixel to geographic coordinate transformation parameters 
print "Creating transformation model (pixel coordinates to cartesian coordinates)...\n";
my ($width, $height) = $map->Get('width', 'height');
my @x = ( 0, $width-1, 0         );
my @y = ( 0, 0       , $height-1 );
my @c_horiz = solve_z_equals_c0x_plus_c1y_plus_c2(\@x, \@y, \@cartx);
my @c_vert = solve_z_equals_c0x_plus_c1y_plus_c2(\@x, \@y, \@carty);
#print Dumper(@c_horiz);
#print Dumper(@c_vert);

# Cropping image
print "Cropping image...\n";
$map->Crop("${crop_width}x${crop_height}+${crop_x}+${crop_y}");
$map->Set(page=>'0x0+0+0');
$map->Write(filename=>$gdm_base . '.cropped.png');

# Adapt width and height to be a multiple of 256
$crop_width = 256 * ceil($crop_width / 256.0);
$crop_height = 256 * ceil($crop_height / 256.0);
print "Adapted image dimension to obey multiple of 256 constraint: $crop_width x $crop_height\n";

# Extent the image to match the required with and height
my ($new_width, $new_height) = $map->Get('width', 'height');
my $extent_x=0;
my $extent_y=0;
if (($new_width!=$crop_width) or ($new_height!=$crop_height)) {
  print "Extending size of image...\n";
  # Does not work due to bug in perl magick
  #my $result = $map->Extent(width=>$crop_width,height=>$crop_height,gravity=>$crop_gravity);
  #warn "$result" if "$result";
  #$map->Set(page=>'0x0+0+0');
  #$map->Write(filename=>$gdm_base . '.extented.png', depth=>8);
  if ($crop_gravity eq "NorthWest") {
    $extent_x = 0;
    $extent_y = 0;
  } elsif ($crop_gravity eq "NorthEast") {
    $extent_x = $crop_width - $new_width;
    $extent_y = 0;
  } elsif ($crop_gravity eq "SouthEast") {
    $extent_x = $crop_width - $new_width;
    $extent_y = $crop_height - $new_height;
  } elsif ($crop_gravity eq "SouthWest") {
    $extent_x = 0;
    $extent_y = $crop_height - $new_height;
  } else {
    die "Unsupported crop gravity!";
  }
  #print $extent_x . "\n";
  #print $extent_y . "\n";
  my $cmd = "convert " . $gdm_base . ".cropped.png -extent " . $crop_width . "x" . $crop_height . "-" . $extent_x . "-" . $extent_y . " " . $gdm_base . ".extented.png";
  #print $cmd . "\n";
  system($cmd);
  $map = Image::Magick->new;
  $map->Read($gdm_base . ".extented.png");
}

# Compute row and column count
my $col_count = 1;
my $row_count = 1;
if (! $single_image) {
  $col_count=$crop_width/1024;
  $row_count=$crop_height/1024;
}

# Go through the requested columns and rows
print "Creating maps for GeoDiscoverer...\n";
my $width = $crop_width/$col_count;
my $height = $crop_height/$row_count;
#print $width . "\n";
#print $height . "\n";
for (my $col=0; $col<$col_count; $col++) {
  for (my $row=0; $row<$row_count; $row++) {

    # Create the image
    my $cx = $col * $width;
    my $cy = $row * $height;
    #print $extent_x . "\n";
    #print $extent_y . "\n";
    my $x = $crop_x + $cx - $extent_x;
    my $y = $crop_y + $cy - $extent_y;
    my $crop_cmd = "${width}x${height}+${cx}+${cy}";
    my $gdm_image = $map->clone();
    $gdm_image->Crop($crop_cmd);
    $gdm_image->Set(page=>'0x0+0+0');
    my $gdm_image_filename = sprintf("${gdm_base}_%d_%d.png",$col,$row);
    print $gdm_image_filename . "\n";
    $gdm_image->Write(filename=>$gdm_image_filename, depth=>8);

    # Create the calibration file
    my $gdm_calib_filename = sprintf("${gdm_base}_%d_%d.gdm",$col,$row);
    print $gdm_calib_filename . "\n";
    my %gdm;
    $gdm{GDM}->{version}="1.0";
    $gdm{GDM}->{imageFileName}=[ $gdm_image_filename ];
    $gdm{GDM}->{zoomLevel}=[ $gdm_zoom_level ];
    $gdm{GDM}->{mapProjection}=[ "proj4" ];
    $gdm{GDM}->{mapProjectionArgs}=[ $proj4args ];
    my @cpoints;
    push(@cpoints,create_cpoint(0,0,$x,$y));
    push(@cpoints,create_cpoint($width-1,0,$x+$width-1,$y));
    push(@cpoints,create_cpoint($width-1,$height-1,$x+$width-1,$y+$height-1));
    push(@cpoints,create_cpoint(0,$height-1,$x,$y+$height-1));
    $gdm{GDM}->{calibrationPoint}=\@cpoints;
    my $xs = new XML::Simple(); 
    my $gdmdata = $xs->XMLout(\%gdm,KeepRoot=>1);
    open(my $out,">$gdm_calib_filename") or die "Can not write to $gdmfname!";
    print $out '<?xml version="1.0" encoding="UTF-8"?>' . "\n";
    print $out $gdmdata;
    close($out);

  }
}

# Computes the constants c0-c2 of the equation c0*x+c1*y+c2
sub solve_z_equals_c0x_plus_c1y_plus_c2 {
  my $x = shift;
  my $y = shift;
  my $z = shift;
  my @c = ( 0.0, 0.0, 0.0 );
  
  # Compute differences between factors
  my $x2x0=$$x[2]-$$x[0];
  my $x1x0=$$x[1]-$$x[0];
  my $y2y0=$$y[2]-$$y[0];
  my $y1y0=$$y[1]-$$y[0];
  my $z1z0=$$z[1]-$$z[0];
  my $z2z0=$$z[2]-$$z[0];

  # Compute c1
  my $t=$x2x0*$y1y0-$x1x0*$y2y0;
  if ($t==0.0) {
    die "can not compute c[1] (divisor is zero)";
  }
  $c[1]=($x2x0*$z1z0-$x1x0*$z2z0)/$t;

  # Compute c0
  if (($x2x0==0.0)&&($x1x0==0.0)) {
    die "can not compute c[0] (divisor is zero)";
  }
  if ($x2x0==0.0) {
    $c[0]=($z1z0-$c[1]*$y1y0)/$x1x0;
  } else {
    $c[0]=($z2z0-$c[1]*$y2y0)/$x2x0;
  }

  # Compute c2
  $c[2]=$$z[0]-$c[0]*$$x[0]-$c[1]*$$y[0];
  return @c;
}

# Computes the longitude and latiude of the given image pixel
sub create_cpoint {
  my $x_dst = shift;
  my $y_dst = shift;
  my $x_src = shift;
  my $y_src = shift;
  my $cpoint;
  my $cartx = $c_horiz[0]*$x_src+$c_horiz[1]*$y_src+$c_horiz[2];
  my $carty = $c_vert[0]*$x_src+$c_vert[1]*$y_src+$c_vert[2]; 
  my ($lat, $lng) = $proj->inverse($cartx, $carty);
  $cpoint=();
  $$cpoint{x}=          [ $x_dst ];
  $$cpoint{y}=          [ $y_dst ];
  $$cpoint{longitude}=  [ $lng ];
  $$cpoint{latitude}=   [ $lat ];
  return $cpoint;
}

