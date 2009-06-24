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
# switch_controller - simple script to change "connected" controller
# 
# Argument should be the path to controller.
# $Id: switch_controller.pl 4514 2007-10-04 17:27:07Z reksio $
# Praterm S.A 2005
# Michał Blajerski <nameless@praterm.com.pl>

use warnings;
use XML::Parser;
use Data::Dumper;

my $controller;
my %real_path;
my %regulators;
my $temp = 1;

my $prefix=`hostname -s`; 




for(@ARGV) {
	if(/--prefix=/i) {
		my @tmp=split "=",$_;
		$prefix=$tmp[1];
	}
	elsif (/-p/i) {
		$prefix="1";
	}
	elsif ($prefix eq "1") {
		$prefix=$_;
	}
	else {
		$controller=$_;
	}
}
chomp $prefix;
if(!$controller) {
	print "Wrong usage.\n";
	exit;
}
chomp $controller;


my $parser = new XML::Parser(ErrorContext  => 2,
			Namespaces    => 1,
			ParseParamEnt => 1,
			Handlers      => {Start => \&parse_line}
			);

#Run parser
$parser->parsefile("/opt/szarp/$prefix/config/params.xml");

			
if(!$real_path{$controller}) {
	print "Nie moge znalezc podanego kontrolera w params.xml\n";
	exit;
}

foreach my $link (@{$regulators->{($real_path{$controller})}->{'name'}}) {
	 system ("rm -f $link");
}


system("ln -s $real_path{$controller} $controller");
system("/etc/init.d/parstart restart");
system("echo $regulators->{$controller}->{'nr'} > /opt/szarp/$prefix/regulator_name"); 
			
#DEBUG
print Dumper($regulators);

# END MAIN


#Parsing...
sub parse_line {
	my $hash = shift; 
	my $element = shift; 
	my $last_path;
	my $name_xmlns_index = 1;

    if (@_) {
	for (my $i = 0; $i < @_; $i += 2) {
		my $name = $_[$i];
		my $name_xmlns = $hash->namespace($name);
		$_[$i] = defined($name_xmlns) ? "$name_xmlns\01$name" : "\01$name";
    }
	my %atts = @_;
	my @ids = sort keys %atts;
	foreach my $id (@ids) {
	  
		my ($name_xmlns, $name) = split(/\01/, $id);
		my $val = $hash->xml_escape($atts{$id}, '"', "\x9", "\xA", "\xD");
		if (length($name_xmlns)) {
			my $pfx = 'n' . $name_xmlns_index++;
				if($name eq "real_path") {
					push(@{$regulators->{$val}->{'name'}},$last_path);
					$regulators->{$last_path}->{'nr'} = $temp;
					$real_path{$last_path}=$val;
					$temp++; 
				}
		}
		else {
	  		 if($name eq "path") {
				$last_path=$val;
			}

		}
	}
}
}
