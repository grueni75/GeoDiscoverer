#!/usr/bin/perl
use Data::Dumper;
use MIME::Base64;

# Get args
die "Usage: analyze_native_crash.pl <html report saved as text in firefox | binary crash dump>" if (@ARGV != 1);
my $report = shift;
my $lib = shift;

# Extract the breakpad dump
my $dump = "";
my $arch;
if ($report !~ m/.dmp$/) {
  print "Extracting native dump from crash report... ";
  open(my $in, $report) or die "Can not open <$report> for reading!";
  my $state = 0;
  while (my $line = <$in>) {
    chomp $line;
    #print $line . "\n";
    if ($state==0) {
      $state=1 if ($line eq 'APPLICATION_LOG');
    } elsif ($state==1) {
      $state=2 if ($line =~ m/index\s*value/);
    } elsif ($state==2) {
      if ($line !~ m/^\d*$/) {
        $state=4;
      } else { 
        $state=3;
      } 
    } elsif ($state==3) {
      $dump = $dump . $line . "\n";
      $state=2;
    } elsif ($state==4) {
      if ($line =~ m/CPU_ABI\s*(\S*)/) {
        $arch = $1;
        last;
      }
    }
    #print $state . "\n";
  }
  close($in);
  #print $dump;
  print "Done!\n";
}

# Translate the cpu architecture
$arch = 'armeabi-v7a' if (! $arch); 
print $arch . "\n";
if ($arch eq 'armeabi-v7a') {
  $arch = 'arm';
} else {
  die "CPU architecture $arch not supported!";
}

# Decode the dump and write it to disk
if ($report !~ m/.dmp$/) {
  print "Decoding native dump... ";
  my $decoded_dump = decode_base64($dump);
  open (my $out, ">/tmp/crash.dmp") or die "Can not open </tmp/crash.dmp> for writing!";
  print $out $decoded_dump;
  close($out);
  print "Done!\n";
} else {
  system("cp \"" . $report . "\" /tmp/crash.dmp");
}

# Output the stack trace
system("minidump_stackwalk /tmp/crash.dmp sym/$arch");


