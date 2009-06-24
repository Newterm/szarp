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
# ipk2mobile.pl - script to converte ipk to mobile-computer files
# argument should be -p prefix or --prefix=prefix
#
# Remember: before use ipk2mobile, link config directory to new configYYYYMMDD.
# It change /opt/szarp/$prefix/config/params.xml
# Old params.xml is in params.old.
# 
# $Id: ipk2mobile.pl 5155 2008-02-14 11:04:29Z reksio $
# Praterm S.A 2005
# Micha³ Blajerski <nameless@praterm.com.pl>


use warnings;
use XML::Parser;

my $execdmn=0;
my $prefix=`hostname -s`;
my $filename;
my $defname_function=0;
my $backname_function=0;


# Read arguments
for(@ARGV) {
	if($_ eq "defname")
	{
		$defname_function=1;
	}
	elsif(/--prefix=/i)
	{
		my @tmp=split "=",$_;
		$prefix=$tmp[1];
	}
	elsif(/-p/i) {
		$prefix="1";
	}
	elsif($prefix eq "1") {
		$prefix=$_;
	}
	else {
		$filename=$_;
	}
}
	
chomp $prefix;
 
my %controllers;
my %id2path;
my %path2name;
my %id2defname;

my $pfx_path="/opt/szarp/$prefix/config";
my $path2name="/tmp/new_name.tmp";
my $sudouser;
my $id2=0;

my $parser = new XML::Parser(ErrorContext  => 2,
			Namespaces    => 1,
			ParseParamEnt => 1,
			Handlers      => {Start => \&parse_line}
			);

# Change the name of controller if defname argument is given
			
if($defname_function) {
	$parser->parsefile("$pfx_path/params.xml");
	defname();
	exit;
}

			
			
# Default action. Parse params.xml and create:
# switch_menu.in ($scc_menu_in$ to include in szarp.cfg)
# sudoers (users who can changed controllers)

$parser->parsefile("$pfx_path/params.xml");
create_scc_file();
create_sudoers_file();

# END MAIN SECTION


# Parsing...
# It fill the %controllers, %id2defname, %id2path, %path2name.
sub parse_line {
	my $hash = shift; 
	my $element = shift; 
	my $last_path;
	my $ns_index = 1;
    my $def_id=0;
	
    if (@_) {
	for (my $i = 0; $i < @_; $i += 2) {
		my $nm = $_[$i];
		my $ns = $hash->namespace($nm);
		$_[$i] = defined($ns) ? "$ns\01$nm" : "\01$nm";
    }
	my %atts = @_;
	my @ids = sort keys %atts;
	foreach my $id (@ids) {
		my ($ns, $nm) = split(/\01/, $id);
		my $val = $hash->xml_escape($atts{$id}, '"', "\x9", "\xA", "\xD");
		if (length($ns)) {
			my $pfx = 'n' . $ns_index++;
			if($nm eq "name") {
				$path2name{$last_path}=$val;
			}		
			if($nm eq "user") {
				$sudouser=$val;				
			}
		}
		else {
			 if($element eq "value" and $nm eq "int" and $execdmn==1) {
				 $def_id=$val; 
			 }
			 if($element eq "value" and $nm eq "name" and $def_id!=0 and $execdmn==1) {
				 $id2defname{$def_id}=$val;
				 $def_id=0; 
			 }
			
			 if($nm eq "daemon" and $val eq "/opt/szarp/bin/testdmn") {
				 $execdmn=1; 
			 }
			 
	  		 if($nm eq "path" and $val=~/regulator_name/i) {
				$last_path=$val; 
			} elsif ($nm eq "path") {
				$id2++; 
				$id2path{$id2}=$val;
				$last_path=$val;
			}

		}
	}
}
}


# Change name function
sub defname {

my $name;
my @tmp=%id2defname;
my $i=1;
my $choice;

system("echo \"#!/bin/sh\" > /tmp/def.sh");
system("echo \"Xdialog --title \\\"Zmieñ nazwê wêz³a\\\" --menu \\\"Wybierz poprawn± nazwê wêz³a:\\\" 30 40 20 \\\\\">> /tmp/def.sh");
system("echo \"\\\"Dodaj now± nazwê\\\"  \\\" \\\" \\\\\" >> /tmp/def.sh");	
	
while($tmp[$i]) {
	system("echo \"\\\"$tmp[$i]\\\"  \\\" \\\" \\\\\" >> /tmp/def.sh");	
	$i+=2;
}

system("echo \"2>/tmp/choice.tmp\"  >> /tmp/def.sh");		
system("sh /tmp/def.sh");

if($? == 0) {
	$choice=`cat /tmp/choice.tmp`;
	system("rm -f /tmp/choice.tmp");
	chomp $choice;
} else {
	exit;
}

if($choice eq "Dodaj now± nazwê") {
	system("Xdialog --title \"Zmiana nazwy wêz³a\" --inputbox \"Podaj now± nazwê wêz³a\" 10 50 2>$path2name");
	if(defined $path2name) {
		$name=`cat /tmp/new_name.tmp`;
		system("rm -f $path2name");
		chomp $name;
	}
} else {
	%id2defname=reverse @tmp;
	system("echo $id2defname{$choice} > /opt/szarp/$prefix/regulator_name");
	exit;
}

if($name) { 
	my $present=0;
	my $execdmn=0;
				
	open(PARAMS,"<$pfx_path/params.xml") or die "Nie moge otworzyc params.xml";
	while(<PARAMS>) {
		if(/regulator_name/i) {
			$execdmn=1; 
		}
		if($execdmn==1 and /\/>/) {
			$present=($i+1)/2;
			system("echo $present > /opt/szarp/$prefix/regulator_name");
			system("cat $pfx_path/params.xml | head -n $. > /tmp/params.new");
			$_=`echo "		<value int=\\"$present\\" name=\\"$name\\"/>" >> /tmp/params.new`;
			my $nr=`wc -l $pfx_path/params.xml`; chomp $nr;
			my @tmp=split " ",$nr;
			$nr=$tmp[0];
			$nr=$nr-$.;
			system("cat $pfx_path/params.xml | tail -n $nr >> /tmp/params.new");
			$execdmn=0;
		}
	}
	close PARAMS;

	system("mv $pfx_path/params.xml $pfx_path/params.old");
	system("mv /tmp/params.new $pfx_path/params.xml");
	
	system("cd $pfx_path && /opt/szarp/bin/i2smo -p $prefix");
	
	}
	else {
		print "Nie podales nowej nazwy wêz³a.\n";
		exit;
	}
}

# Function create switch_menu.in
sub create_scc_file {

open(SCC, ">/opt/szarp/$prefix/switch_menu.in");
print SCC "\$scc_switch_menu\$ := \" \\\n";
print SCC "\tMENU(\\\"Prze³±cz na\\\", \\\n";
	
my $i;
for($i=1; $i<=$id2; $i++) {
	if($i != $id2) {
		print SCC "\t\tEXEC(\\\"$path2name{$id2path{$i}}\\\", \\\"sudo /opt/szarp/bin/switch_controller.pl -p $prefix $id2path{$i}\\\"), \\\n";			
	}
	elsif($i == $id2) {
		print SCC "\t\tEXEC(\\\"$path2name{$id2path{$i}}\\\", \\\"sudo /opt/szarp/bin/switch_controller.pl -p $prefix $id2path{$i}\\\")), \\\n";				
	}
}

print SCC "\tEXEC(\\\"Zmieñ nazwê wêz³a\\\", \\\"sudo /opt/szarp/bin/ipk2mobile.pl -p $prefix defname \\\"), \\\n";				
print SCC "\tSEPARATOR \" ";
close SCC;	
}

# Function create sudoers in /opt/szarp/$prefix and move old sudoers to
# sudoers.old, and linked proper file to /etc/sudoers
sub create_sudoers_file {
	
if($sudouser) {
	open(SUDOERS, ">/opt/szarp/$prefix/sudoers") or die "Nie moge stworzyc /etc/sudoers";
	print SUDOERS "root    ALL=(ALL) ALL\n";	
	print SUDOERS "$sudouser ALL=NOPASSWD:/opt/szarp/bin/switch_controller.pl\n";
	print SUDOERS "$sudouser ALL=NOPASSWD:/opt/szarp/bin/ipk2mobile.pl\n";
	close SUDOERS;
	system("chmod 0440 /etc/sudoers");
	if(-e "/etc/sudoers") {
		system("mv /etc/sudoers /etc/sudoers.old");
	}
	system("cp /opt/szarp/$prefix/sudoers /etc/sudoers");
} else {
	print "Warning: Can't find user to switch controllers. Now only root can do it!\n";
}
}
