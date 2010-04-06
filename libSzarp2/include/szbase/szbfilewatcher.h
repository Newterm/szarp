/* 
  SZARP: SCADA software 

*/
/*
 * szbase - szbfilewatcher.h
 * $Id$
 * <schylek@praterm.com.pl>
 */

#ifndef __SZBFILEWATCHER_H__
#define __SZBFILEWATCHER_H__

#include <config.h>
#include <string>
#include <list>
#include <boost/thread/thread.hpp>

typedef struct {
	std::wstring file;
	std::wstring prefix;
	void (*callback)(std::wstring, std::wstring);
	time_t last_mod_date;
} WatchEntry;

class SzbFileWatcher;

class Watcher {
	public:
		Watcher ( SzbFileWatcher * watcher );
		void operator()();
	private:
		SzbFileWatcher * m_watcher;
};

class SzbFileWatcher {
	public:
		SzbFileWatcher();
		void AddFileWatch(std::wstring file, std::wstring prefix, void (*callback)(std::wstring, std::wstring));
		void RemoveFileWatch( std::wstring prefix );
		void CheckModifications();
		void Terminate();
		void operator()();
	private:
		friend class Watcher;
		bool start_flag;
		std::list<WatchEntry> watchlist;
		boost::mutex datamutex;
		boost::thread thread;
};

#endif //__SZBFILEWATCHER_H__

