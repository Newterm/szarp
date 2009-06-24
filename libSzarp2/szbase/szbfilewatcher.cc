/* 
  SZARP: SCADA software 

*/
/* 
 * szbase - szbfilewatcher
 * $Id$
 * <schylek@praterm.com.pl>
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include "szbfilewatcher.h"
#include "szbfile.h"

class WatchEntryPredicate {
	public:

		std::wstring condition;

		WatchEntryPredicate(std::wstring prefix):condition(prefix){};
	
		bool operator()(WatchEntry w) {
			return condition == w.prefix;
		}
};

Watcher::Watcher( SzbFileWatcher * watcher ): m_watcher(watcher){};

void Watcher::operator()() {

	do {
		boost::xtime xt;
		boost::xtime_get(&xt, boost::TIME_UTC);
		xt.sec = xt.sec - (xt.sec % 600) + 900;
		boost::thread::sleep(xt);

		m_watcher->CheckModifications();
		
	} while(1);

}

SzbFileWatcher::SzbFileWatcher(): start_flag(false) {}

void
SzbFileWatcher::AddFileWatch(std::wstring file, std::wstring prefix, void (*callback)(std::wstring, std::wstring)) {

	boost::mutex::scoped_lock datalock(datamutex);

	if (!start_flag) {
		boost::thread startthread(Watcher(this));
		start_flag = true;
	}

	WatchEntry w;
	w.file = file;
	w.prefix = prefix;
	w.callback = callback;

	bool result = szb_file_exists ( file.c_str(), &w.last_mod_date);
	if (!result)
		w.last_mod_date = -1;

	watchlist.push_back(w);

}

void
SzbFileWatcher::CheckModifications() {
	boost::mutex::scoped_lock datalock(datamutex);

	std::list<WatchEntry>::iterator i;
	for (i = watchlist.begin(); i != watchlist.end(); i++) {
		time_t tmp;
		bool result = szb_file_exists ( i->file.c_str(), &tmp);
		if (result && tmp > i->last_mod_date ) {
			i->last_mod_date = tmp;
			i->callback(i->file, i->prefix);
		}
	}

}

void
SzbFileWatcher::RemoveFileWatch( std::wstring prefix ) {
	boost::mutex::scoped_lock datalock(datamutex);

	WatchEntryPredicate pred(prefix);
	watchlist.remove_if(pred);
	
}

