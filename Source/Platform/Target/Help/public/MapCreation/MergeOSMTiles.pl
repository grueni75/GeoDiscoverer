#!/usr/bin/perl

use 5.006001;
use strict;
use warnings;
use File::Path;
use File::Basename;
use File::Spec;
use File::Copy;
use Cwd qw(cwd);
use Getopt::Long;
use Data::Dumper;
use XML::Simple;
use Math::Trig;
use Image::Magick;

our $usage = qq{Usage: 
   $0 --src=dir --dst=dir [--spawn=number of tiles to combine in horizontal and vertical direction]
};

our @baseurls;
our @baseopacities;

our %opt = (
    src          => undef,
    dst          => undef,
    spawn        => 4,
);


die "$usage\n"
  unless GetOptions(
    \%opt, "src=s", "dst=s", "spawn=s",
  ) && @ARGV == 0;
die "$usage" if (! defined $opt{src});
die "$usage" if (! defined $opt{dst});

# Turn on autoflush
$|++;

# Find out the upper left tile in the source directory
my $dir;
my $file;
my %tilebounds;
opendir($dir, $opt{src}) || die "Cannot open directory <$dir>!\n";
while (my $file = readdir($dir)) {
  #print "\$file = $file\n";
  if ($file =~ m/(\d*)_(\d*)_(\d*)\.gdm/) {
    my $zoom = int($1);
    my $tilex = int($2);
    my $tiley = int($3);
    if (! defined $tilebounds{$zoom}) {
      $tilebounds{$zoom}{min}{x}=$tilex;
      $tilebounds{$zoom}{min}{y}=$tiley;
      $tilebounds{$zoom}{max}{x}=$tilex;
      $tilebounds{$zoom}{max}{y}=$tiley;
    } else {
      if ($tilex < $tilebounds{$zoom}{min}{x}) {
        $tilebounds{$zoom}{min}{x}=$tilex;
      }
      if ($tilex > $tilebounds{$zoom}{max}{x}) {
        $tilebounds{$zoom}{max}{x}=$tilex;
      }
      if ($tiley < $tilebounds{$zoom}{min}{y}) {
        $tilebounds{$zoom}{min}{y}=$tiley;
      }
      if ($tiley > $tilebounds{$zoom}{max}{y}) {
        $tilebounds{$zoom}{max}{y}=$tiley;
      }
     }
  }
}
closedir($dir);
#print Dumper(%tilebounds);

# Go through all zoom levels
foreach my $z(keys %tilebounds) {
  print "Processing zoom level $z... ";

  # Go through all tiles in this zoom level
  for (my $tilex=$tilebounds{$z}{min}{x}; $tilex<=$tilebounds{$z}{max}{x}; $tilex=$tilex+$opt{spawn}) {
    for (my $tiley=$tilebounds{$z}{min}{y}; $tiley<=$tilebounds{$z}{max}{y}; $tiley=$tiley+$opt{spawn}) {

      # Stitch the tile images together
      my $copyUnmergedTiles = 0;
      my $mergedImage = Image::Magick->new;
      for (my $y=0; $y<$opt{spawn}; $y++) {
        for (my $x=0; $x<$opt{spawn}; $x++) {
          my $f = $opt{src} . "/" . $z . "_" . ($tilex+$x) . "_" . ($tiley+$y) . ".png";

          # If the file does not exist, fall back to simple copy
          if (! -e $f) {
            $copyUnmergedTiles=1;
            last;
          }

          $mergedImage->Read($f);
        }
      }
      if ($copyUnmergedTiles) {

        # Copy all existing tiles
        for (my $y=0; $y<$opt{spawn}; $y++) {
          for (my $x=0; $x<$opt{spawn}; $x++) {
            my $f = $z . "_" . ($tilex+$x) . "_" . ($tiley+$y);
            if (-e $opt{src} . "/" . $f . ".png") {
              #print "Copying " . $f . "\n";
              copy($opt{src}."/".$f.".png",$opt{dst}."/".$f.".png");
              copy($opt{src}."/".$f.".gdm",$opt{dst}."/".$f.".gdm");
            }
          }
        }

      } else {

        # Write the new image
        $mergedImage = $mergedImage->Montage(mode=>'Concatenate',tile=>$opt{spawn}.'x'.$opt{spawn});
        my $basename = $z . "_" . $tilex . "_" . $tiley;
        #print "Merging " . $basename . "\n";
        $mergedImage->Write($opt{dst} . "/" . $basename . ".png");

        # Create a new GDM file
        my $t;
        my ($latsouth, $longwest, $latnorth, $longeast);
        ($t, $longwest, $latnorth, $t)=tilexy2latlon($tilex,$tiley,$z);
        ($latsouth, $t, $t, $longeast)=tilexy2latlon($tilex+$opt{spawn}-1,$tiley+$opt{spawn}-1,$z);
        my $width=$mergedImage->Get('width');
        my $height=$mergedImage->Get('height');
        my $imagefname=$basename . ".png";
        my %gdm;
        $gdm{GDM}->{version}="1.0";
        $gdm{GDM}->{imageFileName}=[ $imagefname ];
        $gdm{GDM}->{mapProjection}=[ "mercator" ];
        $gdm{GDM}->{zoomLevel}=[ $z ];
        my @cpoints;
        my $cpoint;
        $cpoint=();
        $$cpoint{x}=          [ 0 ];
        $$cpoint{y}=          [ 0 ];
        $$cpoint{longitude}=  [ $longwest ];
        $$cpoint{latitude}=   [ $latnorth ];
        push(@cpoints,$cpoint);
        $cpoint=();
        $$cpoint{x}=          [ $width-1 ];
        $$cpoint{y}=          [ 0 ];
        $$cpoint{longitude}=  [ $longeast ];
        $$cpoint{latitude}=   [ $latnorth ];
        push(@cpoints,$cpoint);
        $cpoint=();
        $$cpoint{x}=          [ $width-1 ];
        $$cpoint{y}=          [ $height-1 ];
        $$cpoint{longitude}=  [ $longeast ];
        $$cpoint{latitude}=   [ $latsouth ];
        push(@cpoints,$cpoint);
        $cpoint=();
        $$cpoint{x}=          [ 0 ];
        $$cpoint{y}=          [ $height-1 ];
        $$cpoint{longitude}=  [ $longwest ];
        $$cpoint{latitude}=   [ $latsouth ];
        push(@cpoints,$cpoint);
        $gdm{GDM}->{calibrationPoint}=\@cpoints;
        my $xs = new XML::Simple(); 
        my $gdmdata = $xs->XMLout(\%gdm,KeepRoot=>1);
        my $gdmfname = $opt{dst}."/".$basename.".gdm";
        open(my $out,">$gdmfname") or die "Can not write to $gdmfname!";
        print $out '<?xml version="1.0" encoding="UTF-8"?>' . "\n";
        print $out $gdmdata;
        close($out);
      }
    }
  } 
  print "Done!\n";
}

sub ProjectMercToLat($){
    my $MercY = shift;
    return rad2deg(atan(sinh($MercY)));
}

sub tilexy2latlon {
  my ($X,$Y, $Zoom) = @_;
  my $Unit = 1 / (2 ** $Zoom);
  my $relY1 = $Y * $Unit;
  my $relY2 = $relY1 + $Unit;
 
  # note: $LimitY = ProjectF(degrees(atan(sinh(pi)))) = log(sinh(pi)+cosh(pi)) = pi
  # note: degrees(atan(sinh(pi))) = 85.051128..
  #my $LimitY = ProjectF(85.0511);
 
  # so stay simple and more accurate
  my $LimitY = pi;
  my $RangeY = 2 * $LimitY;
  $relY1 = $LimitY - $RangeY * $relY1;
  $relY2 = $LimitY - $RangeY * $relY2;
  my $Lat1 = ProjectMercToLat($relY1);
  my $Lat2 = ProjectMercToLat($relY2);
  $Unit = 360 / (2 ** $Zoom);
  #print $Unit . "\n";
  #print $X . "\n";
  my $Long1 = -180 + $X * $Unit;
  #print $Lat2 . " " . $Long1 . " " . $Lat1 . " " . ($Long1+$Unit) . "\n";
  return ($Lat2, $Long1, $Lat1, $Long1 + $Unit); # S,W,N,E
}

__END__

=head1 SEE ALSO

L<http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames>

=head1 AUTHOR

Matthias Grünewald 

=head1 COPYRIGHT AND LICENCE

Copyright (C) 2015 by Matthias Grünewald

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.8.8 or,
at your option, any later version of Perl 5 you may have available.

=cut

__END__

=head1 NAME

MergeOSMTiles.pl - Merge nearby OSM tiles into a bigger GDM tiles 

=head1 SYNOPSIS

  MergeOSMTiles.pl --src=<directory with OSM tiles> --dst=<directory with GDM tiles> --spawn=<number of OSM tiles to merge in horizontal and vertical direction>

=head1 DESCRIPTION

This script merges nearby OSM tiles stored in the source directory
into bigger GDM tiles in the destination directory. This reduces
the number of files and speeds up the map load time in Geo 
Discoverer.

=head1 COMMAND LINE OPTIONS

Command line options may be abbreviated as long as they remain
unambiguous.

At least C<--src> and C<--dst> must be specified.

=head2 C<--src=directory>

Source directory that contains the OSM tiles.

Default: none

=head2 C<--dst=directory>

Destination directory where the merged GDM tiles are stored in.

Default: none

=head2 C<--spawn=number>

Number of nearby tiles in horizontal and vertical direction 
that are combined into one new GDM tile. 

Default: 4

=head1 SEE ALSO

L<http://wiki.openstreetmap.org/wiki/Slippy_Map>

=head1 AUTHOR

Matthias Grünewald

=head1 COPYRIGHT AND LICENCE

Copyright (C) 2015 by Matthias Grünewald

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.8.8 or,
at your option, any later version of Perl 5 you may have available.

=cut
