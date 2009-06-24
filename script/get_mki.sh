#Skrypt do odczytu danych z koncentratora liczników Energii elektrycznej MKI firmy Pozyton
# (c) Pawe³ Kolega 2008

function get_data_from_mki(){
arg1=$1
URL="192.168.1.30"

HTML=`mktemp`
XML=`mktemp`

TMP=`mktemp`

USER="admin"
PASS="admin"

SZARP_NO_DATA=-32768

TOTAL_URL="http://$URL/mtronl$arg1.php"

t_code[0]="spp"; t_type[0]=32 #Stan liczyd³a P+ sum.
t_code[1]="mcp"; t_type[1]=16 #Moc chwilowa P123
t_code[2]="pp1"; t_type[2]=32 #Stan liczyd³a P+ strefa 1 
t_code[3]="mp1"; t_type[3]=16 #Moc chwilowa P1 
t_code[4]="pp2"; t_type[4]=32 #Stan liczyd³a P+ strefa 2 
t_code[5]="mp2"; t_type[5]=16 #Moc chwilowa P2 
t_code[6]="pp3"; t_type[6]=32 #Stan liczyd³a P+ strefa 3 
t_code[7]="mp3"; t_type[7]=16 #Moc chwilowa P3 
t_code[8]="spm"; t_type[8]=32 #Stan liczyd³a P- sum. 
t_code[9]="mq1"; t_type[9]=16 #Moc chwilowa Q1 
t_code[10]="pm1"; t_type[10]=32 #Stan liczyd³a P- strefa 1 
t_code[11]="mq2"; t_type[11]=16 #Moc chwilowa Q2 
t_code[12]="pm2"; t_type[12]=32 #Stan liczyd³a P- strefa 2 
t_code[13]="mq3"; t_type[13]=16 #Moc chwilowa Q3 
t_code[14]="pm3"; t_type[14]=32 #Stan liczyd³a P- strefa 3 
t_code[15]="mcs"; t_type[15]=16 #Moc chwilowa S123 
t_code[16]="pm4"; t_type[16]=32 #Stan liczyd³a P- strefa 4 
t_code[17]="ms1"; t_type[17]=16 #Moc chwilowa S1 
t_code[18]="sqp"; t_type[18]=32 #Stan liczyd³a  Q+ sum. 
t_code[19]="ms2"; t_type[19]=16 #Moc chwilowa S2 
t_code[20]="qp1"; t_type[20]=32 #Stan liczyd³a  Q+ strefa 1 
t_code[21]="ms3"; t_type[21]=16 #Moc chwilowa S3 
t_code[22]="qp2"; t_type[22]=32 #Stan liczyd³a  Q+ strefa 2 
t_code[23]="mu1"; t_type[23]=16 #Napiêcie na fazie 1 
t_code[24]="qp3"; t_type[24]=32 #Stan liczyd³a  Q+ strefa 3 
t_code[25]="mu2"; t_type[25]=16 #Napiêcie na fazie 2 
t_code[26]="qp4"; t_type[26]=32 #Stan liczyd³a  Q+ strefa 4 
t_code[27]="mu3"; t_type[27]=16 #Napiêcie na fazie 3 
t_code[28]="sqm"; t_type[28]=32 #Stan liczyd³a  Q- sum 
t_code[29]="mi1"; t_type[29]=16 #Pr±d na fazie 1 
t_code[30]="qm1"; t_type[30]=32 #Stan liczyd³a  Q- sterfa 1 
t_code[31]="mi2"; t_type[31]=16 #Pr±d na fazie 2 
t_code[32]="qm2"; t_type[32]=32 #Stan liczyd³a  Q- sterfa 2 
t_code[33]="mi3"; t_type[33]=16 #Pr±d na fazie 3 
t_code[34]="qm3"; t_type[34]=32 #Stan liczyd³a  Q- sterfa 3 
t_code[35]="qm4"; t_type[35]=32 #Stan liczyd³a Q- strefa 4 
t_code[36]="ssp"; t_type[36]=32 #Stan liczyd³a  S+ sum. 
t_code[37]="ssm"; t_type[37]=32 #Stan liczyd³a S- sum.

#logowanie do MKI
WGET_RESULT=`wget "http://$URL/login" --output-document $TMP --timeout 15 -t 5 --random-wait --post-data="user=$USER&pass=$PASS&form=login" --quiet`

if [ ${#t_code[@]} -ne ${#t_type[@]} ]; then
echo "Error in script configuration - inproper numer of elements in t_code table (${#t_code[@]}) ant t_type table (${#t_type[@]})"
exit
fi

WGET_RESULT=`wget --output-document $HTML --timeout 15 -t 5 --wait=7 --random-wait --quiet $TOTAL_URL`

tidy -c -asxml $HTML 2> /dev/null | xmllint --html --xmlout - > $XML || die "Error parsing XML"

idx="0"

for i in "${t_code[@]}"
do
	VALUE=`xmlstarlet sel -N "h=http://www.w3.org/1999/xhtml" -t -v "//h:input[@type='text' and @name='$i$arg1']/@value" $XML | sed -e 's/\.//g' | sed -e 's/\.//g'`
	if [ "$VALUE" == "" ]; then
		echo "Error in script configuration. code $i is not accessible"
		exit
	fi
	VALUE=`echo $VALUE |sed -e 's/^0*//g'`
	if [ "$VALUE" == "" ]
	then
		VALUE=0	
	fi
	if [ "$VALUE" == "------" ]
	then
		VALUE=SZARP_NO_DATA	
	fi
	if [[ ${t_type[$idx]} -eq 16 ]]
	then
		echo $VALUE;
	elif [[ ${t_type[$idx]} -eq 32 ]]
	then
		if [[ $VALUE -eq $SZARP_NO_DATA ]]
		then
			echo $SZARP_NO_DATA
			echo $SZARP_NO_DATA
		else
			VALUE_MSW=VALUE
			VALUE_LSW=VALUE
			let "VALUE_MSW >>= 16"
			echo $VALUE_MSW
			let "VALUE_LSW &= 0xffff"
			echo $VALUE_LSW
		fi
	else
		echo "Error in script configuration. Each element of t_type must be either 16 or 32 (bits) this has ${t_type[$idx]}"
		exit
	fi
     let "idx += 1"
done
rm -fr $HTML
rm -fr $XML
rm -fr $TMP
}

A_TIME=`date +'%H:%M'`
HOUR=`echo $A_TIME |sed -e 's/^0*//g'|sed 's/^\(.*\):\(.*\)$/\1/'`
MIN=`echo $A_TIME |sed -e 's/^0*//g'|sed 's/^\(.*\):\(.*\)$/\2/'`
A_TIMESTAMP=0
let "A_TIMESTAMP = 60 * $HOUR + $MIN"
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
