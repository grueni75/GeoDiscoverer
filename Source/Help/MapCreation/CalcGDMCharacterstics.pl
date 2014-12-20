#!/usr/bin/perl
#============================================================================
# Name        : CalcGDMCharacteristics.pl
# Author      : Matthias Gruenewald
# Copyright   : Copyright 2014 Matthias Gruenewald
#
# This file is part of CalcGDMCharacteristics.
#
# CalcGDMCharacteristics is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# CalcGDMCharacteristics is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with CalcGDMCharacteristics. If not, see <http://www.gnu.org/licenses/>.
#
#============================================================================
use strict;
use warnings;
use Data::Dumper;
use XML::Simple;

# Get arguments
if (scalar @ARGV != 1) {
  die "Usage: CalcGDMCharacteristics.pl <GDM filename>";
}
my $gdmfilename = shift;

my $gdm = XMLin($gdmfilename);
my $latmin = +90.0;
my $ymin;
my $latmax = -90.0;
my $ymax;
my $lngmin = +180.0;
my $xmin;
my $lngmax = -180.0;
my $xmax;
foreach my $calibration_point(@{$gdm->{calibrationPoint}}) {
  if ($calibration_point->{latitude} < $latmin) {
    $latmin = $calibration_point->{latitude};
    $ymin = $calibration_point->{y};
  }
  if ($calibration_point->{latitude} > $latmax) {
    $latmax = $calibration_point->{latitude};
    $ymax = $calibration_point->{y};
  }
  if ($calibration_point->{longitude} < $lngmin) {
    $lngmin = $calibration_point->{longitude};
    $xmin = $calibration_point->{x};
  }
  if ($calibration_point->{longitude} > $lngmax) {
    $lngmax = $calibration_point->{longitude};
    $xmax = $calibration_point->{x};
  }
}
print "Horizontal pixel/degree: " . ($xmax-$xmin)/($lngmax-$lngmin) . "\n";
print "Vertical pixel/degree:   " . ($ymin-$ymax)/($latmax-$latmin) . "\n";

