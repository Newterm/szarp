PATH=/bin:/sbin:/usr/sbin:/usr/bin:/usr/sbin
0 10 * * *	root	touch /opt/szarp/.update_debs
0 * * * *	root	[ -e /opt/szarp/.update_debs ] && { rm /opt/szarp/.update_debs; /opt/szarp/bin/update_debs || touch /opt/szarp/.update_debs; }	

