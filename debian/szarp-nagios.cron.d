# Reload szarp-nagios configuration once a day, in case new SZARP 
# configuration becomes available.

18 5 * * *	root	/opt/szarp/bin/update-nagios.py -v; /etc/init.d/nagios3 reload; /etc/init.d/szarp-nagios reload
