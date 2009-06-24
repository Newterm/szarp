#Skrypt do odczytu danych z koncentratora liczników Energii elektrycznej MKI-4 (nowsza wersja) firmy Pozyton
# (c) Pawe³ Kolega 2008

function get_data_from_mki(){
arg1=$1
URL="192.168.1.10"

#HTML_CH=`mktemp` #Warto¶ci chwilowe
XML_CH=`mktemp` 
HTML_CH=`mktemp`
HTML_ST=`mktemp`


#HTML_ST=`mktemp` #Warto¶ci liczyde³ 
XML_ST=`mktemp`

TMP=`mktemp`

#U¿ytkownik i has³o
USER="admin"
PASS="nimda"

TOTAL_URL_CH="http://$URL/onlch$arg1.php"
TOTAL_URL_ST="http://$URL/onlst$arg1.php"

t_code[0]="- Moc chwilowa P+ sum."; 		t_type[0]=16 
t_code[1]="- Moc chwilowa P- sum."; 		t_type[1]=16 
t_code[2]="- Moc chwilowa P+ faza 1"; 		t_type[2]=16 
t_code[3]="- Moc chwilowa P- faza 1"; 		t_type[3]=16 
t_code[4]="- Moc chwilowa P+ faza 2"; 		t_type[4]=16 
t_code[5]="- Moc chwilowa P- faza 2"; 		t_type[5]=16 
t_code[6]="- Moc chwilowa P+ faza 3"; 		t_type[6]=16 
t_code[7]="- Moc chwilowa P- faza 3"; 		t_type[7]=16 
t_code[8]="- Moc chwilowa Q+ sum."; 		t_type[8]=16 
t_code[9]="- Moc chwilowa Q- sum."; 		t_type[9]=16 
t_code[10]="- Moc chwilowa Q+ faza 1"; 		t_type[10]=16 
t_code[11]="- Moc chwilowa Q- faza 1"; 		t_type[11]=16 
t_code[12]="- Moc chwilowa Q+ faza 2"; 		t_type[12]=16 
t_code[13]="- Moc chwilowa Q- faza 2"; 		t_type[13]=16 
t_code[14]="- Moc chwilowa Q+ faza 3"; 		t_type[14]=16 
t_code[15]="- Moc chwilowa Q- faza 3"; 		t_type[15]=16 
t_code[16]="- Moc chwilowa S+ sum."; 		t_type[16]=16 
t_code[17]="- Moc chwilowa S- sum."; 		t_type[17]=16 
t_code[18]="- Moc chwilowa S+ faza 1"; 		t_type[18]=16 
t_code[19]="- Moc chwilowa S- faza 1"; 		t_type[19]=16 
t_code[20]="- Moc chwilowa S+ faza 2"; 		t_type[20]=16 
t_code[21]="- Moc chwilowa S- faza 2"; 		t_type[21]=16 
t_code[22]="- Moc chwilowa S+ faza 3"; 		t_type[22]=16 
t_code[23]="- Moc chwilowa S- faza 3"; 		t_type[23]=16 
t_code[24]="- Napiêcie w fazie 1"; 		t_type[24]=16 
t_code[25]="- Napiêcie w fazie 2"; 		t_type[25]=16 
t_code[26]="- Napiêcie w fazie 3"; 		t_type[26]=16 
t_code[27]="- Pr±d w fazie 1"; 			t_type[27]=16 
t_code[28]="- Pr±d w fazie 2"; 			t_type[28]=16 
t_code[29]="- Pr±d w fazie 3"; 			t_type[29]=16 
t_code[30]="- Stan liczyd³a P+ sum."; 		t_type[30]=32 
t_code[31]="- Stan liczyd³a P- sum."; 		t_type[31]=32 
t_code[32]="- Stan liczyd³a P+ strefa 1";	t_type[32]=32 
t_code[33]="- Stan liczyd³a P- strefa 1"; 	t_type[33]=32 
t_code[34]="- Stan liczyd³a P+ strefa 2";	t_type[34]=32 
t_code[35]="- Stan liczyd³a P- strefa 2";	t_type[35]=32 
t_code[36]="- Stan liczyd³a P+ strefa 3";	t_type[36]=32 
t_code[37]="- Stan liczyd³a P- strefa 3";	t_type[37]=32 
t_code[38]="- Stan liczyd³a P+ strefa 4";	t_type[38]=32 
t_code[39]="- Stan liczyd³a P- strefa 4"; 	t_type[39]=32 
t_code[40]="- Stan liczyd³a Q+ sum.";		t_type[40]=32 
t_code[41]="- Stan liczyd³a Q- sum.";		t_type[41]=32 
t_code[42]="- Stan liczyd³a Q+ strefa 1"; 	t_type[42]=32  
t_code[43]="- Stan liczyd³a Q- strefa 1"; 	t_type[43]=32 
t_code[44]="- Stan liczyd³a Q+ strefa 2";	t_type[44]=32 
t_code[45]="- Stan liczyd³a Q- strefa 2";	t_type[45]=32 
t_code[46]="- Stan liczyd³a Q+ strefa 3";	t_type[46]=32 
t_code[47]="- Stan liczyd³a Q- strefa 3";	t_type[47]=32 
t_code[48]="- Stan liczyd³a Q+ strefa 4";	t_type[48]=32 
t_code[49]="- Stan liczyd³a Q- strefa 4";	t_type[49]=32 
t_code[50]="- Stan liczyd³a S+ sum."; 		t_type[50]=32 
t_code[51]="- Stan liczyd³a S- sum."; 		t_type[51]=32 

#Konwersja polskich znaków na UTF-8
CTR=0
for i in "${t_code[@]}" 
do
	t_code[$CTR]=`echo $i|iconv --from-code ISO88592 --to-code UTF8`
	((CTR++))
done

#logowanie do MKI
WGET_RESULT=`wget "http://$URL/login" --ignore-length --output-document $TMP --timeout 3 -t 3 --random-wait --post-data="user=$USER&pass=$PASS&form=login" --quiet`


if [ ${#t_code[@]} -ne ${#t_type[@]} ]; then
echo "Error in script configuration - inproper numer of elements in t_code table (${#t_code[@]}) ant t_type table (${#t_type[@]})"
exit
fi

WGET_RESULT=`wget --ignore-length --output-document $HTML_CH --timeout 3 -t 3 --wait=7 --random-wait $TOTAL_URL_CH --quiet`
WGET_RESULT=`wget --ignore-length --output-document $HTML_ST --timeout 3 -t 3 --wait=7 --random-wait $TOTAL_URL_ST --quiet`

tidy -raw -c -asxml $HTML_CH 2> /dev/null | xmllint --encode iso8859-2 --html --xmlout - > $XML_CH || die "Error parsing XML"
tidy -raw -c -asxml $HTML_ST 2> /dev/null | xmllint --encode iso8859-2 --html --xmlout - > $XML_ST || die "Error parsing XML"

idx="0"

for i in "${t_code[@]}"
do
	IFS=$'\n' # Separator stringa
	RESULT=`xmlstarlet sel -N "N=http://www.w3.org/1999/xhtml" -t -v "//N:table/N:tr[N:td='$i']" $XML_CH` 
	CTR=0
	FOUND=0
	for j in $RESULT
	do
		((CTR++)) 
		if [ $i == $j ]
		then
			FOUND=$CTR
		fi
	done

	if [ $FOUND -eq 3 ]
	then
		((FOUND=FOUND+2))
	elif [ $FOUND -eq 1 ]
	then
		((FOUND=FOUND+1))
	fi

	if [ $FOUND -gt 0 ]
	then
		RESULT=`xmlstarlet sel -N "N=http://www.w3.org/1999/xhtml" -t -v "//N:table/N:tr[N:td='$i']/N:td[$FOUND]/N:input/@value" $XML_CH | sed -e 's/\.//g' |sed -e 's/^0*//g'`  
		if [ "$RESULT" == "" ]
		then
			RESULT=0	
		fi
	else
		#Je¶li nie znale¼li¶my w CH, szukamy w ST
		IFS=$'\n' # Separator stringa
		RESULT=`xmlstarlet sel -N "N=http://www.w3.org/1999/xhtml" -t -v "//N:table/N:tr[N:td='$i']" $XML_ST` 
		CTR=0
		FOUND=0
		for j in $RESULT
		do
			((CTR++)) 
			if [ $i == $j ]
			then
				FOUND=$CTR
			fi
		done

		if [ $FOUND -eq 3 ]
		then
			((FOUND=FOUND+2))
		elif [ $FOUND -eq 1 ]
		then
			((FOUND=FOUND+1))
		fi

		if [ $FOUND -eq 0 ]
		then
			echo "Error in script configuration. code $i is not accessible"
			exit
		fi
		RESULT=`xmlstarlet sel -N "N=http://www.w3.org/1999/xhtml" -t -v "//N:table/N:tr[N:td='$i']/N:td[$FOUND]/N:input/@value" $XML_ST | sed -e 's/\.//g' |sed -e 's/^0*//g'`  
		if [ "$RESULT" == "" ]
		then
			RESULT=0	
		fi
	fi
	if [[ ${t_type[$idx]} -eq 16 ]]
	then
		echo $RESULT;
	elif [[ ${t_type[$idx]} -eq 32 ]]
	then
		VALUE_MSW=RESULT
		VALUE_LSW=RESULT
		let "VALUE_MSW >>= 16"
		echo $VALUE_MSW
		let "VALUE_LSW &= 0xffff"
		echo $VALUE_LSW
	else
		echo "Error in script configuration. Each element of t_type must be either 16 or 32 (bits) this has ${t_type[$idx]}"
		exit
	fi
    let "idx += 1"
done

rm -fr $HTML_CH
rm -fr $HTML_ST
rm -fr $XML_CH
rm -fr $XML_ST
rm -fr $TMP
}

A_TIME=`date +'%H:%M'`
HOUR=`echo $A_TIME |sed -e 's/^0*//g'|sed 's/^\(.*\):\(.*\)$/\1/'`
MIN=`echo $A_TIME |sed -e 's/^0*//g'|sed 's/^\(.*\):\(.*\)$/\2/'`
A_TIMESTAMP=0
((A_TIMESTAMP = 60 * HOUR + MIN))

CHANNEL=0
B_TIME=""
E_TIME=""
while (( "$#" )) ; do

	if [[ $1 == "-c" || $1 == "--channel" ]]
	then
		shift
		CHANNEL=$1	
	fi
	
	if [[ $1 == "-b" || $1 == "--begin-time" ]]
	then
		shift
		B_TIME=$1	
	fi

	if [[ $1 == "-e" || $1 == "--end-time" ]]
	then
		shift
		E_TIME=$1	
	fi

	shift 
done

if [[ $CHANNEL == "0" ]];then
echo "Error. Script require least one param (channel 1..4)"
echo "Full usage:"
echo "get_mki.sh <-c channel>|<--channel> [-b|--begin-time <hh:mm>] [-e|--end-time <hh:mm>]"
exit 1
fi

if [[ $B_TIME != "" && $E_TIME != "" ]]
then
HOUR=`echo $B_TIME |sed -e 's/^0*//g'|sed 's/^\(.*\):\(.*\)$/\1/'`
MIN=`echo $B_TIME |sed -e 's/^0*//g'|sed 's/^\(.*\):\(.*\)$/\2/'`
B_TIMESTAMP=0
let "B_TIMESTAMP = 60 * $HOUR + $MIN"

HOUR=`echo $E_TIME |sed 's/^\(.*\):\(.*\)$/\1/'`
MIN=`echo $E_TIME |sed 's/^\(.*\):\(.*\)$/\2/'`
E_TIMESTAMP=0
let "E_TIMESTAMP = 60 * $HOUR + $MIN"

#DEBUG echo $B_TIMESTAMP $E_TIMESTAMP $A_TIMESTAMP

	if [[ $A_TIMESTAMP -gt $B_TIMESTAMP && $A_TIMESTAMP -lt $E_TIMESTAMP ]]
	then
		exit
	fi
fi

get_data_from_mki $CHANNEL
