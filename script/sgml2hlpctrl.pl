#!/usr/bin/perl
# SZARP: SCADA software 
# 
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#
# sgml2hlpctrl.pl - generate hhp hhk hhc files needed by szHelpController class
# $Id: sgml2hlpctrl.pl 4393 2007-09-13 16:37:10Z reksio $
# Michal Blajerski <nameless@praterm.com.pl>





use warnings;
use strict;

my $path_to_sgml=shift  || "not_defined";
my $dir=`dirname $path_to_sgml`; 
my $name=`basename $dir`;
my $id=1;

if($dir =~ /\./)  {
	$dir=`pwd`;
	$name=`basename $dir`; 
}

chomp $path_to_sgml;
chomp $dir;
chomp $name;


if($path_to_sgml eq "not_defined") {
	print "Use:sgml2hlpctrl <full_path_to_sgml>\n";
	print "example:\n";
	print "./sgml2hlpctrl.pl /usr/work/szarp/resources/documetation/new/ekstraktor3/ekstraktro3.sgml\n";
	exit;
	}

open(SGMLFILE,"<$path_to_sgml");
open(HHP,">$dir/html/$name.hhp");
open(HHK,">$dir/html/$name.hhk");
open(HHC,">$dir/html/$name.hhc");
open(MAP,">$dir/html/$name.map");


my $title_of_book="not_defined";	
my $default_page=`cat $path_to_sgml | grep "book lang" -i`;
if ($default_page eq "") {
	$default_page=`cat $path_to_sgml | grep "article lang" -i`;
}
my @page = split /"/, $default_page;
$default_page=$page[3];	
my $object=0;
my $chapter_level=0;
my $part_level=0;
my $section_level=0;
my $comment=0;
my $more_in_one_line=0;
my %entity;
my @entity_name;
my $nr_entity=0;
my $line;
my @lines;
my @mem_line;
my $nr_of_files=0;
my @path_to_file;
$path_to_file[0]=$path_to_sgml;
	
print HHP "Contents file=$name.hhc\n";
print HHP "Index file=$name.hhk\n";

	
print HHK "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//PL\">\n";
print HHK "<HTML>\n<HEAD>\n";
print HHK "<meta name=\"GENERATOR\" content=\"sgml2hlpctrl\">\n";
print HHK "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-2\">\n";
print HHK "<!-- Sitemap 1.0 -->\n</HEAD><BODY>\n";
print HHK "<OBJECT type=\"text/site properties\">\n";
print HHK "<param name=\"ImageType\" value=\"Folder\">\n";
print HHK "</OBJECT>\n<UL>\n";
	
print HHC "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//PL\">\n";
print HHC "<HTML>\n<HEAD>\n";
print HHC "<meta name=\"GENERATOR\" content=\"sgml2hlpctrl\">\n";
print HHC "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-2\">\n";
print HHC "<!-- Sitemap 1.0 -->\n</HEAD><BODY>\n";
print HHC "<OBJECT type=\"text/site properties\">\n";
print HHC "<param name=\"ImageType\" value=\"Folder\">\n";
print HHC "</OBJECT>\n<UL>\n";

read_file();

print HHK "</UL>\n</BODY></HTML>\n";
print HHC "</BODY></HTML>\n";

close HHP;
close HHK;
close HHC;
close SGMLFILE;
close MAP;



sub read_file {


while (<SGMLFILE>) { 
	$line=$_; 
	if(/<!--/) { 
		$comment=1; }
	if(/-->/) {
		$comment=0; }
	if($comment==0) {	

	if(/ENTITY/) {
		my @tmp=split " ", $line;
		$entity_name[$nr_entity]=$tmp[1];
		$tmp[3]=~tr/">"/  /;
		chomp $tmp[3];
		$tmp[3]=~s/^\s+//;
		$tmp[3]=~s/\s+$//;
		$entity{$tmp[1]}=$tmp[3];
		$nr_entity++;
	}
	
	if(/\&/ and /;/ and !/lt/ and !/qt/) {
		chomp $line; 
		$line= substr $line, 2;
				
		for(@entity_name) {
			
			if("$_;" eq $line) {
				
				$mem_line[$nr_of_files]=$.;
				close SGMLFILE;
				$nr_of_files++;
				$path_to_file[$nr_of_files]=$entity{$_};
				open (SGMLFILE,"<$path_to_file[$nr_of_files]") or die "Nie moge otworzyc pliku";
				read_file();
				close SGMLFILE;
				$nr_of_files--;
				open(SGMLFILE,"<$path_to_file[$nr_of_files]");
				while($. != $mem_line[$nr_of_files]) {
					$_=<SGMLFILE>;
				}
				
			}
		}
	}
					
		
		
	}
		
	if(/part id/i) {
		@lines = split /"/, $line;
		$more_in_one_line=1;
		if(!$lines[1]) {
			@lines = split /=/, $line;	
			@lines= split />/, $lines[1];
			$line=$lines[0];

			
		} else {
		$line=$lines[1]; }
		$line=~ tr/ABCDEFGHIJKLMNOPRSTUWXYZV/abcdefghijklmnoprstuwxyzv/;
		
		if((check_file($line))!=0) {
		
			$object=1;
			$part_level++;
			print HHC "\t<UL>\n";
		
			print HHC "\t<LI> <OBJECT type=\"text/sitemap\">\n";
			print HHC "\t\t<param name=\"ID\" value=$id>\n";
			print HHC "\t\t<param name=\"Local\" value=\"$line.html\">\n";
			print HHK "\t<LI> <OBJECT type=\"text/sitemap\">\n";
			print HHK "\t\t<param name=\"Local\" value=\"$line.html\">\n";
			print MAP "$id $line\n";
			$id++;
			}
		}
		
		
	if(/chapter id/i) {
		@lines = split /"/, $line;
		$more_in_one_line=1;

		if(!$lines[1]) {
			@lines = split /=/, $line;	
			@lines= split />/, $lines[1];
			$line=$lines[0];

			
		} else {
		$line=$lines[1]; }
		$line=~ tr/ABCDEFGHIJKLMNOPRSTUWXYZV/abcdefghijklmnoprstuwxyzv/;
		if((check_file($line))!=0) {
			$object=1;
			$chapter_level++; 
			print HHC "\t<UL>\n";
			
			print HHC "\t<LI> <OBJECT type=\"text/sitemap\">\n";
			print HHC "\t\t<param name=\"ID\" value=$id>\n";
			print HHC "\t\t<param name=\"Local\" value=\"$line.html\">\n";
			print HHK "\t<LI> <OBJECT type=\"text/sitemap\">\n";
			print HHK "\t\t<param name=\"Local\" value=\"$line.html\">\n";
			print MAP "$id $line\n";
			$id++;
			}
		}
		
	if(/section id/i) {
		@lines = split /"/, $line;
		$more_in_one_line=1;

		if(!$lines[1]) {
			@lines = split /=/, $line;	
			@lines= split />/, $lines[1];
			$line=$lines[0];

			
		} else {
		$line=$lines[1]; }
		$line=~ tr/ABCDEFGHIJKLMNOPRSTUWXYZV/abcdefghijklmnoprstuwxyzv/;
			if((check_file($line))!=0) {
			$object=1;
		
			if($section_level==0) {		
				$section_level++; 
				print HHC "\t<UL>\n";
				}
			print HHC "\t<LI> <OBJECT type=\"text/sitemap\">\n";
			print HHC "\t\t<param name=\"ID\" value=$id>\n";				
			print HHC "\t\t<param name=\"Local\" value=\"$line.html\">\n";
			print HHK "\t<LI> <OBJECT type=\"text/sitemap\">\n";
			print HHK "\t\t<param name=\"Local\" value=\"$line.html\">\n";
			print MAP "$id $line\n";	
			$id++;
			}
		}
		
	if(/<title>/i) {
		if(/<\/title>/i && /<section id/i) {
			@lines = split /<section id/i, $line;
			$line=$lines[0];
			@lines = split /<\/title>/i, $line;
			$line=$lines[0]; 
			@lines = split /<title>/i, $line;
			$line=$lines[0];

		} elsif(/<\/title>/i && !/<section/i) {

			@lines = split /<\/title>/i, $line;
			$line=$lines[0];
			@lines = split /<title>/i, $line;
			$line=$lines[1];

		}


		else {
			@lines = split /<title>/i, $line;
		}
			
		if($title_of_book eq "not_defined") {
			$title_of_book=$line;
			print HHK "\t<LI> <OBJECT type=\"text/sitemap\">\n";
			print HHK "\t\t<param name=\"Local\" value=\"$default_page.html\">\n";
			print HHK "\t\t<param name=\"Name\" value=\"$title_of_book\">\n";
			print HHK "\t</OBJECT>\n";
			print HHP "Default topic=$default_page.html\n";
			print HHP "Title=$title_of_book\n";
				} elsif($object==1) {
					
				$line =~ s/<application>//;
				$line =~ s/<\/application>//;
				$line =~ s/<emphasis>//;
				$line =~ s/<\/emphasis>//;				
				$line =~ s/<filename>//;
				$line =~ s/<\/filename>//;
				$line =~ s/<command>//;
				$line =~ s/<\/command>//;
				$line =~ s/<title>//;
				$line =~ s/<\/title>//;
				$line =~ s<quote>//;
				$line =~ s<\/quote>//;		
				$line =~ s/<>//;
				$line =~ s/<\/>//;					
				
				print HHK "\t\t<param name=\"Name\" value=\"$line\">\n";
				print HHK "\t</OBJECT>\n";
				print HHC "\t\t<param name=\"Name\" value=\"$line\">\n";
				print HHC "\t</OBJECT>\n";
		
	
				$object=0;
				if($section_level!=0) {
					print HHC "</UL>\n";
					$section_level=0;  
					}
			}
		
		}
		
	if(/<\/part>/i && $part_level!=0) {
		$part_level=0; 
		print HHC "\t</UL>\n";	
		}
	if(/<\/chapter>/i && $chapter_level!=0) {
		$chapter_level=0;
		print HHC "\t</UL>\n";	
		}
	
	}
	$more_in_one_line=0;
}

	



sub check_file {
	my $summary=`ls $dir/html/ | grep -i -c $line.html`;
	if($summary ==0) {
		return 0;
	} else {
		return 1;
	}
}
