#!/usr/bin/perl
#
# Skrypt do sprawdzania martwych linków do dokumentacji wywo³ywanych przez polecenie Dokumentacja parametru (Ctrl + H)
# z programu draw 3.
# Pawe³ Kolega 2008
#

use XML::LibXML;
use XML::LibXML::Common;
use URI::Escape 'uri_escape_utf8';
use File::Temp ;
use File::stat;
use Getopt::Long;
use MIME::QuotedPrint;
use Mail::Send;
use vars qw ($VERSION);

@list_of_params = (); ##Lista parametrów
@list_of_devices = (); ##Lista urz±dzeñ
@list_of_draw_names = (); ##Lista nazw wykresów
@list_of_draw_names = (); ##Lista nazw wykresów
@list_of_sections = (); ##Lista typów sekcji 0 - device; 1 - defined; 2 - drawdefinable;

$SECTION_DEVICE = 0;
$SECTION_DEFINED = 1;
$SECTION_DRAWDEFINABLE = 2;

@CONF_SECTIONS = ('device',
	 	  'defined',
		  'drawdefinable',
	         );

@list_of_prefixes = (); ##Lista konfiguracji z pominiêciem @DIRS_IGNORE

$mail_header = "Znaleziono nastêpuj±ce martwe linki w¶ród nastêpuj±cych parametrów:\r\n\r\n";
$mail_subject = "[TestDoc] Znaleziono martwe linki do dokumentacji w konfiguracjach baz" ;
$mail_footer = "\r\n\r\n--\r\nPozdrowienia test_doc.pl\r\n";

$mail_device = "Sprawdzanie parametrów DEVICE: ";
$mail_definable = "Sprawdzanie parametrów DEFINED && DRAWDEFINABLE: ";
@NO_YES = ('NIE', 'TAK'); 

$verbose_flag = 0; ## Flaga trybu gadatliwego
$help_flag = 0; ## Flaga pomocy
$all_flag = 0; ## Flaga sprawdzenia wszystkich prefixów
$prefix_flag = ""; ## Flaga konkretnego prefixu
$notify_flag = ""; ## Flaga powiadomienia userów o errorze
$device_flag = 0; ##Flaga sprawdzania parametrów z sekcji device
$definable_flag = 0; ##Flaga sprawdzania parametrów z sekcji device 
$dad_flag = 0; ##$device_flag && $definable_flag
$quiet_flag = 0; ##Flaga trybu cichego
$auto_string = "Dokumentacja wygenerowana automatycznie na podstawie definicji parametru."; ##Workaround string dla dokumentacji wygenerowanych automatycznie na podstawie RPN.

## Konfiguracja
$PATH_TO_DATABASES = "/opt/szarp"; #Domy¶lna ¶cie¿ka (mo¿e byæ zmieniona opcj± -s|--src-dir)
$PATH_TO_CONFIGURATION = "/config/params.xml"; 

@DIRS_IGNORE = ("bin", #Katalogi ignorowane przy opcji -a|-all
		"resources",
		"lib",
		"var", 
		"prat.s22041",
		"sun",
		"sunc",
		"gliw",
		"gcwp",
		"gcie",
		"wodz",
		"zory",
		"7188",
		"porK",
		"porP",
		"sztK",
		"athr",
		"prat",
		"chck",
		"glwX",
		"gizy",
		"glw1",
		"glw2",
		"glw3",
		"glw4",
		"glw5",
		"glw6",
		"glw7",
		"glw8",
		"glw9",
		"kato",
		"ktw1",
		"raci",
		"rcw1",
		"star",
		"sunX",
		"rawX",
		"radm",
		"leg1",
		"leg2",
		"leg3",
		"legX",
		"glws",
		"pnew",
		"pabi",
		"ptmX",
		"stpd",
		"rww1",
		"rww2",
		"rww3",
		"rww4",
		"rww5",
		"rww6",
		"rww7",
		"rww8",
		"rww9",
		"lucyfer",
		"arch",
		"elk1",
		"kepn",
		"logs",
		"lucy",
		"racX",
		"rawc",
		"snew",
		"staX",
		"sztP",
		"wars",
		"wasp",
		"wloc",
		"skis",
		"chw2",
	);


$MIN_DOC_SIZE = 500 ; ## Minimalna wielko¶æ pliku z docgenem

## Typy b³êdów jakie jest w stanie wykryæ procedura obs³ugi b³êdnych linków ##
$ERROR_DOC_NO_ERROR = 0;
$ERROR_DOC_NO_URL = 1;
$ERROR_DOC_NO_RED_URL = 2;
$ERROR_DOC_TOO_SMALL = 3;
@ERROR_DOC_MESSAGES = ('brak b³êdów', 
		       'nieprawid³owy URL', 
		       'nieprawid³owy URL przekierowany', 
		       'wielko¶æ pliku z dokumentacj± za ma³a'
	              );


# £aduje prefixy do @list_of_prefixes
# @param - ¶cie¿ka do baz danych - domy¶lnie /opt/szarp
sub LoadPrefixes{
	my $path = shift;
    	print "SUB: LoadPrefixes '$path'\n" if $verbose_flag;
	# Odczytujemy wszystkie katalogi
	opendir (DIR, $path);  
	@prefixes = readdir(DIR);
	foreach $prefix (@prefixes) {
		if ((-d ($path."/".$prefix)) and not (grep $_ eq $prefix, @DIRS_IGNORE) and  $prefix ne "." and $prefix ne ".."){
   			push @list_of_prefixes, $prefix;
		}
	} 
	closedir (DIR);
}

## Funkcja tworzy listê parametrów urz±dzeñ (bez sekcji defined i drawdefinable)
# @param - pe³na ¶cie¿ka do params.xml e.g. /opt/szarp/byto/config/params.xml
sub CreateListOfDevices{ 
	my $path_to_conf = shift;
	my $draw_name="";
	my $tmp_name = "";
	$parser = new XML::LibXML;
    	print "SUB: CreateListOfDevices '$path_to_conf'\n" if $verbose_flag;
	$struct = $parser -> parse_file($path_to_conf);
	$rootel = $struct -> getDocumentElement();
	$elname = $rootel -> getName();
	@kids = $rootel -> childNodes();
	$device_path="";
foreach $child(@kids) {
        $device_name = $child -> getName(); #Czytanie tagu device
	$device_type = $child -> getType();
	@device_attributes_list = $child -> getAttributes(); 
	$device_path = "";	
	foreach $device_attribute (@device_attributes_list) {
		if ($device_attribute -> getName() eq "path"){
			$device_path = $device_attribute -> getValue();  
			@units_list = $child->getChildnodes(); #Odczytujemy sekcjê unit
			foreach $unit (@units_list) {
				$unit_name = $unit -> getName();	
				$unit_type = $unit -> getType();	
				if ($unit_type == XML_ELEMENT_NODE){
					@params_list = $unit -> getChildnodes(); #Odczytujemy sekcjê unit
					foreach $param (@params_list) {
						$param_name = $param -> getName();	
						$param_type = $param -> getType();	
						if ($param_type == XML_ELEMENT_NODE){
							@param_attributes_list = $param -> getAttributes();
							$tmp_name="";
							$draw_name="";

							foreach $param_attribute (@param_attributes_list) { #Odczytywanie atrybutów sekcji param
								if ($param_attribute -> getName() eq "draw_name"){
										$draw_name = decodeFromUTF8("iso-8859-2", $param_attribute -> getValue()) ;
								}
								if ($param_attribute -> getName() eq "name"){
									$tmp_name = decodeFromUTF8("iso-8859-2", $param_attribute -> getValue());
									#$tmp_name =~ s/{.*} //; #Support dla parametrów zagregowanych
								}

							}

							@draws_list = $param -> getChildnodes(); #Kod do sprawdzania czy jest sekcja param. 
								
								foreach $draw (@draws_list) { #Szukamy elementu draw
						
									if ($draw->getName() eq "draw" and $tmp_name ne "" and $draw_name ne ""){
										push @list_of_params, $tmp_name;
										push @list_of_devices, $device_path ;
										push @list_of_draw_names, $draw_name ;
										push @list_of_sections, $SECTION_DEVICE ;
									}
								}	
							
								#}
						}

					}
					
				}

			}
		}
	}
}
}

## Funkcja tworzy listê parametrów seekcje defined i drawdefinable
# @param - pe³na ¶cie¿ka do params.xml e.g. /opt/szarp/byto/config/params.xml
sub CreateListOfDefined{ 
	my $path_to_conf = shift;
	my $draw_name = "";
	my $tmp_name = "";
	$parser = new XML::LibXML;
    	print "SUB: CreateListOfDefined '$path_to_conf'\n" if $verbose_flag;
	$struct = $parser -> parse_file($path_to_conf);
	$rootel = $struct -> getDocumentElement();
	$elname = $rootel -> getName();
	@kids = $rootel -> childNodes();
	foreach $child(@kids) {
	        $defined_name = $child -> getName(); #Czytanie tagu defined lub drawdefinable
		$defined_type = $child -> getType();
		if ($defined_type == XML_ELEMENT_NODE and ($defined_name eq "defined" or  $defined_name eq "drawdefinable")){
			@params_list = $child -> getChildnodes(); #Odczytujemy sekcjê unit
			foreach $param (@params_list) {
				$param_name = $param -> getName();	
				$param_type = $param -> getType();	
				if ($param_type == XML_ELEMENT_NODE){
					@param_attributes_list = $param -> getAttributes(); 
					$draw_name = "";
					$tmp_name = "";
					foreach $param_attribute (@param_attributes_list) { #Odczytywanie atrybutów sekcji param
						if ($param_attribute -> getName() eq "draw_name"){
							$draw_name = decodeFromUTF8("iso-8859-2", $param_attribute -> getValue()) ;
						}
						
						if ($param_attribute -> getName() eq "name"){
							$tmp_name = decodeFromUTF8("iso-8859-2", $param_attribute -> getValue()); 
							#$tmp_name =~ s/{.*} //; #Support dla parametrów zagregowanych
						}
	
					}
					@draws_list = $param -> getChildnodes(); #Kod do sprawdzania czy jest sekcja param. 

						foreach $draw (@draws_list) { #Szukamy elementu draw
						
							if ($draw -> getName () eq "draw" and $tmp_name ne "" and $draw_name ne ""){
								push @list_of_params, $tmp_name;
								push @list_of_devices, "null" ;
								push @list_of_draw_names, $draw_name ;
								if ($defined_name eq "defined"){
									push @list_of_sections, $SECTION_DEFINED ;
								}
								else{
									push @list_of_sections, $SECTION_DRAWDEFINABLE ;
								}
							}
						}	
						#}
				}
			}
		}
	}
}

sub CheckDocLink{
	my ($prefix, $param_name, $path) = @_;
	my $red_url;
	my $auto_flag = 0;
    	print "SUB: CheckdocLink '$prefix', '$param_name', '$path'\n" if $verbose_flag;
	($fh, $file) = mkstemp( "tmpfileXXXXX" );
	$param_name = encodeToUTF8("iso-8859-2", $param_name);
	$param_name = uri_escape_utf8($param_name);
	if ($path eq "" or $path eq "null"){
	 	`wget --no-check-certificate -q -O $file "https://www.szarp.com.pl/cgi-bin/param_docs.py?prefix=$prefix&param=$param_name"`;
 	}
	else{
	 	`wget --no-check-certificate -q -O $file "https://www.szarp.com.pl/cgi-bin/param_docs.py?prefix=$prefix&param=$param_name&path=$path"`;
 	}
	$red_url = "";
	if (not open(FILE, $file)) {
		unlink ($file);
		unlink ($file_doc);
		return $ERROR_DOC_NO_URL;
	}
	while (<FILE>) {
		if (/^.*url=(.*)".*$/){
			$red_url = $1;	
		}

		if (/^.*$auto_string.*$/){
			$auto_flag = 1;
		}

	}
	close (FILE);

	if ($auto_flag){
		unlink ($file);
		unlink ($file_doc);
		return $ERROR_DOC_NO_ERROR;
	}

	if ($red_url eq ""){
		unlink ($file);
		unlink ($file_doc);
		return $ERROR_DOC_NO_URL;
	}

	($fh_doc, $file_doc) = mkstemp( "tmpfileXXXXX" );
	if (not (-e $file_doc)){
		unlink ($file);
		unlink ($file_doc);
		return $ERROR_DOC_NO_RED_URL; 
	}


	`wget -q -O $file_doc "$red_url"`;
	 if (stat($file_doc)->size < $MIN_DOC_SIZE){
		unlink ($file);
		unlink ($file_doc);
	 	return $ERROR_DOC_TOO_SMALL;
	}
	unlink ($file);
	unlink ($file_doc);

	return $ERROR_DOC_NO_ERROR;
}

sub Help
{ 
 print "
Skrypt do sprawdzania martwych linków do dokumentacji wywo³ywanych przez polecenie Dokumentacja parametru (Ctrl + H)
z programu draw 3.

Uzycie:
   $0 prefix [-v|--verbose] [-p|--prefix <prefix>] [-a|--all] [-s|--src-dir <path>] 
             [-n|--notify <email>] [-d|--devices] [-e|--definable] [-q|--quiet]
   -v|--verbose - tryb gadatliwy.
   -a|--all - sprawdzenie wszystkich prefixów dostepnych w ¶cie¿ce --src-dir
   -p|--prefix <prefix> - prefix.
   -s|--src-dir <path> - ¶cie¿ka z bazami domy¶lnie /opt/szarp. 
   -n|--notify <email> - powiadomienie na mejla.
   -d|--devices - sprawdzanie parametrów tylko z sekcji device
   -e|--definable - sprawdzanie parametrów tylko z sekcji definable
   -g|--d-a-d - zastêpuje opcje -d|--devices && -e|--definable
   -q|--quiet - tryb cichy
   
   ";
};

# wysyla maila do $to o temacie $subject i tresci $body
sub sendmail
{
    my $to = shift;
    my $subject = shift;
    my $body = shift;
	
    print "SUB: sendmail '$to' '$subject' '$body'\n" if $verbose_flag;

    my $mail = new Mail::Send;

    $mail->to($to);

    $mail->add("Content-Type", "text/plain; charset=iso-8859-2");
    $mail->add("Content-Transfer-Encoding", "quoted-printable");
    
    $subject = encode_qp($subject);
    $subject =~ s/ /_/;
    $subject =~ s/\n//;
    $subject =~ s/=$//;
    
    $subject = "=?ISO-8859-2?Q?" . $subject . "?=";
    $mail->subject($subject);
    my $mail_fh = $mail->open;
    print $mail_fh encode_qp($body);
    
    $mail_fh->close;
}

## Sprawdza parametry ze wszystkich prefixów dostêpnych w ¶cie¿ce $PATH_TO_DATABASES
sub DoAllPrefixes{
	my $counter = 0;
	my $mail_body = $mail_header ;
	my $mail_flag = 0;
	print "SUB: DoAllPrefixes\n" if $verbose_flag;
	$mail_body = $mail_body . "$mail_device $NO_YES[$device_flag] \r\n";
	$mail_body = $mail_body . "$mail_definable $NO_YES[$definable_flag] \r\n";
	$mail_body = $mail_body . "\r\n";
	print "$mail_device $NO_YES[$device_flag] \r\n" if not $quiet_flag;
	print "$mail_definable $NO_YES[$definable_flag] \r\n" if not $quiet_flag;

	&LoadPrefixes($PATH_TO_DATABASES); #Wyszukanie wszystkich prefixów baz danych
	if ($verbose_flag){
		print "List of prefixes: \n" if $verbose_flag;
		foreach $verbose_prefix (@list_of_prefixes) {
			print "$verbose_prefix \n";	
		}
	}
	foreach $prefix (@list_of_prefixes) {
		$full_path_to_configuration = $PATH_TO_DATABASES."/".$prefix. "/". $PATH_TO_CONFIGURATION ; 
		print "Full path to configuration: " . $full_path_to_configuration ."\n" if  $verbose_flag; 
			if (-e $full_path_to_configuration){
				@list_of_params = (); 
				@list_of_devices = ();
				@list_of_draw_names = ();
				&CreateListOfDevices($full_path_to_configuration) if $device_flag;
				&CreateListOfDefined($full_path_to_configuration) if $definable_flag;
				$counter = 0;
				foreach $param (@list_of_params){
					$stat = &CheckDocLink($prefix, $param, $list_of_devices[$counter]);
					print "Proces prefix: '$prefix'; param: '$param'\n" if  $verbose_flag;
					if (not ($stat == $ERROR_DOC_NO_ERROR)){
						print " + prefix: '$prefix'; section: $CONF_SECTIONS[$list_of_sections[$counter]]; param: '$param'; draw_name: '$list_of_draw_names[$counter]'; ERROR Code status: '$stat'; Description: $ERROR_DOC_MESSAGES[$stat]; \n" if not $quiet_flag ;
						if ($notify_flag ne ""){
							$mail_flag = 1;
							$mail_body = $mail_body . " + prefix: '$prefix'; section: $CONF_SECTIONS[$list_of_sections[$counter]]; param: '$param'; draw_name: '$list_of_draw_names[$counter]'; ERROR Code status: '$stat'; Description: $ERROR_DOC_MESSAGES[$stat]; \r\n" ;
						}
					}
					$counter++;
				}		
			}
	}
	if ($notify_flag ne ""){
		$mail_body = $mail_body . $mail_footer;
	}
	if ($mail_flag == 1){
		&sendmail ($notify_flag, $mail_subject, $mail_body);
	}
}

## Sprawdza parametry ze wskazanego prefixu
# @param prefix - prefix bazy danych
sub DoPrefix{
	my $prefix = shift;
	my $mail_body = $mail_header ;
	my $mail_flag = 0;
	my $counter = 0;
  	
	print "SUB: DoPrefix '$prefix'\n" if $verbose_flag;
	$mail_body = $mail_body . "$mail_device $NO_YES[$device_flag] \r\n";
	$mail_body = $mail_body . "$mail_definable $NO_YES[$definable_flag] \r\n";
	$mail_body = $mail_body . "\r\n";
	
	print "$mail_device $NO_YES[$device_flag] \r\n" if not $quiet_flag;
	print "$mail_definable $NO_YES[$definable_flag] \r\n" if not $quiet_flag;
	
	$full_path_to_configuration = $PATH_TO_DATABASES."/".$prefix. "/". $PATH_TO_CONFIGURATION ; 
		if (-e $full_path_to_configuration){
			@list_of_params = (); 
			@list_of_devices = ();
			@list_of_draw_names = ();
			&CreateListOfDevices($full_path_to_configuration) if $device_flag;
			&CreateListOfDefined($full_path_to_configuration) if $definable_flag;
			$counter = 0;
			foreach $param (@list_of_params){
				$stat = &CheckDocLink($prefix, $param, $list_of_devices[$counter]);
				print "Proces prefix: '$prefix'; param: '$param'\n" if $verbose_flag;
				if (not ($stat == $ERROR_DOC_NO_ERROR)){
					print " + prefix: '$prefix'; section: $CONF_SECTIONS[$list_of_sections[$counter]]; param: '$param'; draw_name: '$list_of_draw_names[$counter]'; ERROR Code status: '$stat'; Description: $ERROR_DOC_MESSAGES[$stat]; \n" if not $quiet_flag ;
					if ($notify_flag ne ""){
						$mail_flag = 1;
						$mail_body = $mail_body . " + prefix: '$prefix'; section: $CONF_SECTIONS[$list_of_sections[$counter]]; param: '$param'; draw_name: '$list_of_draw_names[$counter]'; ERROR Code status: '$stat'; Description: $ERROR_DOC_MESSAGES[$stat]; \r\n" ;
					}
				}
				$counter++;
			}		
		}
		if ($notify_flag ne ""){
			$mail_body = $mail_body . $mail_footer;
		}
		if ($mail_flag == 1){
			&sendmail ($notify_flag, $mail_subject, $mail_body);
		}
}

	GetOptions ('h|help' => \$help_flag,
 	'v|verbose' => \$verbose_flag,
 	'a|all' => \$all_flag,
 	's|src-dir' => \$PATH_TO_DATABASES,
 	'p|prefix=s' => \$prefix_flag,
 	'n|notify=s' => \$notify_flag,	
	'd|devices' => \$device_flag,
	'e|definable' => \$definable_flag,
	'g|definable' => \$dad_flag,
	'q|quiet' => \$quiet_flag,
	);

	if ($help_flag == 1){
		&Help;
		exit 0;
	}
	
	if ($all_flag == 0 && $prefix_flag eq ""){
		&Help;
		exit 0;	
	}

	if ($dad_flag){
		$device_flag = 1; 
		$definable_flag = 1;	
	}

	if ($device_flag == 0 && $definable_flag == 0){
		&Help;
		exit 0;		
	}

	if ($all_flag == 1){
		&DoAllPrefixes;
		exit 0;
	}

	if ($prefix_flag ne ""){
		&DoPrefix($prefix_flag);
		exit 0;
	}

