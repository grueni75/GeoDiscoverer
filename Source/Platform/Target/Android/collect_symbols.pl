#!/usr/bin/perl
use Data::Dumper;
use MIME::Base64;
use File::Basename;
use File::Path qw(make_path);
use File::stat;

# Get args
die "Usage: collect_symbols.pl <directory>" if (@ARGV != 1);
my $dir = shift;

# Do the symbol generation
collect_symbols($dir);

# Generate the symbols for all .so files in the given dir
sub collect_symbols { 
  my $dir = shift;

  print "Processing <$dir>\n";
  opendir my $in, $dir or die "Could not open dir <$dir>: $!";
  my @files = readdir($in); 
  closedir $in;
  #print Dumper(@files);
  foreach my $file(@files) {
    my $path = $dir . "/" . $file;
    #print $path . "\n";
    if ((-f $path) && ($path =~ m/\.so$/)) {

      # Check if symbols have already been generated
      my $outdated = 0;
      my $flag = $dir . "/." . $file . ".processed";
      if (-e $flag) {
        my $file_stat = stat($path);
        my $flag_stat = stat($flag);
        if ($file_stat->mtime > $flag_stat->mtime) {
          $outdated = 1;
        }
      } else {
        $outdated = 1;
      }
      #print "outdated=" . $outdated . "\n";
      if ($outdated) {

        # Generate the symbols
        open(my $cmd, '-|', "dump_syms \"$path\" 2>/dev/null") or die "Could not execute command: $!";
        my @contents;
        my $hash;
        my $arch;
        my $name;
        while (my $line = <$cmd>) {
          #print $line;
          if ($line =~ m/MODULE Linux (\S*) (\S*) (\S*)/) {
            $hash = $2;
            $arch = $1;
            $name = $3;
          }
          push(@contents,$line);
        }
        #print $hash . "\n" . $arch . "\n" . $name . "\n";
      
        # Write the symbols
        my $symdir = "sym/" . "/" . $arch . "/" . $name . "/" . $hash;
        my $symfile = $name . ".sym";
        make_path($symdir);
        open(my $out, ">" . $symdir . "/" . $symfile) or die "Can not open <${symdir}/${name}> for writing: $!";
        print $out @contents;
        close($out);

        # Create the flag
        system("touch \"$flag\"");
      }
    }
    if ((-d $path) && ($file ne '.') && ($file ne '..')) {
      collect_symbols($path);
    }
  }
}
