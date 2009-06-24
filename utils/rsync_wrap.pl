#!/usr/bin/perl

# $Id: rsync_wrap.pl 898 2003-09-19 14:58:52Z ecto $

# rsync_wrap.pl: wraper do rsynca
# robi synchronizacje dwóch katalogów, z po¿±dnym liczeniem postêpu

use strict;
use English;
#use Data::Dumper;

$OUTPUT_AUTOFLUSH=1;

my $src = shift or die;
my $dst = shift or die;

#print "d: rsync_wrap $src $dst\n"; #D

my $rsync = `which rsync`;
chomp $rsync;
my $rsync_opts = '--rsh=ssh';

# podstawowe komendy
my %commands = (
  # rozmiary plików
  'sizes' => "$rsync $rsync_opts -r $src|",
  # lista prawdopodobnych zmian
  'to_get' => "$rsync $rsync_opts -naz $src $dst|",
  # w³a¶ciwa synchronizacja
  'sync' => "$rsync $rsync_opts -Pvvaz --delete $src $dst|",
);

# najpierw: lista wszystkich plików (z rozmiarami)

my %r_files = ();

print "getting remote list...\n";
open RS_LIST, $commands{'sizes'};
while (<RS_LIST>) {
  #print $_; #D
  next if !/^[-]/;
  my (undef, $size, undef, undef, $name) = split;
  #print "$size - $name\n"; #D
  $r_files{$name}=$size;
}
close RS_LIST;

# w:
# %r_files powinni¶my mieæ plik -> rozmiar

# potem: lista plików (do pobrania)

my @dw_files = ();
my $to_get_sum = 0;

print "getting difference...\n";
open RS_TO_GET, $commands{'to_get'};
while (<RS_TO_GET>) {
  chomp;
  if (exists $r_files{$_}) {
    push @dw_files, $_;
    $to_get_sum += $r_files{$_};
  }
}
close RS_TO_GET;

print "done..." and exit if not $to_get_sum;

# w:
# @dw_files - lista plików do ¶ci±gniêcia
# $to_get_sum - sumaryczny rozmiar do ¶ci±gniêcia

#print Data::Dumper->Dump([\%r_files, \@dw_files, $to_get_sum]); #D

# out: w formie ilepobrano/ilewsumie\r

print "syncing...\n";
open RS_SYNC, $commands{'sync'};
my $act_downloaded = 0;
my $act_file = undef;
my $act_file_down = 0;
my $act_file_n = 0;
while (<RS_SYNC>) {
  if (/(\S+) is uptodate/) {
    if (defined $act_file) {
      $act_downloaded += $r_files{$act_file} if defined $r_files{$act_file};
      $act_file = undef;
      $act_file_down = 0;
      #print "done\n"; #D
    }
    #print "$1 don't need downloading\n"; #D
  }elsif (/^(\S+)\s*$/) {
    if (defined $act_file) {
      #print "done\n"; #D
      $act_downloaded += $r_files{$act_file} if defined $r_files{$act_file};
    }
    $act_file = $1;
    $act_file_down = 0;
    #print "downloading $act_file "; #D
  }elsif (/^\s*(\d+)\s+\d+\%/) {
    $act_file_down = $1;
    #print "."; #D
  }else {
    #print "other\n"; #D
    if (defined $act_file) {
      $act_downloaded += $r_files{$act_file} if defined $r_files{$act_file};
      $act_file = undef;
      $act_file_down = 0;
      #print "done\n"; #D
    }
  }

  my $dw_tmp = ($act_downloaded+$act_file_down);
  print "$dw_tmp/$to_get_sum\r";
}
close RS_SYNC;
print "done...\n";
