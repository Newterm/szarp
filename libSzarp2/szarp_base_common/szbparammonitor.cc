/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "config.h"

#include <cstring>

#include <sys/types.h>
#ifndef MINGW32
#include <sys/socket.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#endif

#include <vector>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>

#include "liblog.h"
#include "conversion.h"

#include "szarp_base_common/szbparamobserver.h"
#include "szarp_base_common/szbparammonitor.h"

typedef int SzbMonitorTokenType;

bool SzbParamMonitorImplBase::start_monitoring_dir(const std::string& path, TParam* param, SzbParamObserver* observer, unsigned order) {
	command cmd;
	cmd.cmd = ADD_CMD;
	cmd.path = path;
	cmd.observer = observer;
	cmd.param = param;
	cmd.order = order;
	{
		boost::mutex::scoped_lock lock(m_mutex);
		m_queue.push_back(cmd);
	}
	return trigger_command_pipe();
}

bool SzbParamMonitorImplBase::end_monitoring_dir(SzbMonitorTokenType token) {
	command cmd;
	cmd.cmd = DEL_CMD;
	cmd.token = token;
	{
		boost::mutex::scoped_lock lock(m_mutex);
		m_queue.push_back(cmd);
	}
	return trigger_command_pipe();
}

bool SzbParamMonitorImplBase::terminate() {
	command cmd;
	cmd.cmd = END_CMD;
	{
		boost::mutex::scoped_lock lock(m_mutex);
		m_queue.push_back(cmd);
	}
	if (!trigger_command_pipe())
		return false;

	m_thread.join();
	return true;
}


#ifndef MINGW32
SzbParamMonitorImpl::SzbParamMonitorImpl(SzbParamMonitor* monitor, const std::string&) : m_monitor(monitor) {
	m_terminate = false;
	m_cmd_socket[0] = m_cmd_socket[1] = -1;

	if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, m_cmd_socket))
		throw std::runtime_error("Failed to craete socketpair");

	
	m_inotify_socket = inotify_init1(IN_NONBLOCK);
	if (m_inotify_socket == -1)
		throw std::runtime_error("Failed to init initify");
}


void SzbParamMonitorImpl::process_notification() {
	char buf[sizeof(inotify_event) + PATH_MAX + 1];
	std::vector<std::pair<SzbMonitorTokenType, std::string> > tokens_and_paths;

	while (true) {
		ssize_t r = read(m_inotify_socket, buf, sizeof(buf));
		if (r < 0) {
			if (errno == EINTR)
				continue;
			else if (errno == EWOULDBLOCK || errno == EAGAIN)
				break;
			else
				throw std::runtime_error("Failed to read from inotify socket");
		}

		ssize_t p = 0;
		while (r > 0) {
			struct inotify_event* e = (struct inotify_event*) (buf + p);
			if ((e->mask & (IN_MODIFY | IN_MOVED_TO)) && e->len) {
				size_t l = strlen(e->name);
				if (l > 4 && e->name[l - 4] == '.' && e->name[l - 3] == 's' && e->name[l - 2] == 'z'
						&& (e->name[l - 1] == '4' || e->name[l - 1] == 'b'))
					tokens_and_paths.push_back(std::make_pair(SzbMonitorTokenType(e->wd), std::string(e->name)));
			} else if ((e->mask & IN_CREATE) && e->len) {
				auto it = m_monitor->m_token_path.find(e->wd);
				if (it != m_monitor->m_token_path.end()) {
					boost::filesystem::path dest_dir = boost::filesystem::absolute(it->second);
					boost::filesystem::path current_watch = dest_dir;

					while (!boost::filesystem::exists(current_watch)) {
						current_watch = current_watch.parent_path();
					}

					int new_wd;
					if (dest_dir == current_watch)
						new_wd = inotify_add_watch(m_inotify_socket, it->second.c_str(), IN_MOVED_TO | IN_MODIFY);
					else
						new_wd = inotify_add_watch(m_inotify_socket, current_watch.native().c_str(), IN_CREATE);

					if (inotify_rm_watch(m_inotify_socket, e->wd))
						sz_log(3, "Failed to remove watch, errno: %d", errno);

					if (new_wd < 0) {
						sz_log(3, "Failed to add watch for path:%s, errno: %d", it->second.c_str(), errno);
					} else {
						m_monitor->modify_dir_token(e->wd, new_wd);
					}
				} else {
					sz_log(3, "No entries for given token (path: %s), something is wrong!", it->second.c_str());
				}
			}
			r -= sizeof(*e) + e->len;
			p += sizeof(*e) + e->len;
		}

	}

	if (tokens_and_paths.size())
		m_monitor->files_changed(tokens_and_paths);
}

void SzbParamMonitorImpl::process_cmds() {
	while (true) {
		char c;	
		int wd;

		if (read(m_cmd_socket[1], &c, 1) != 1) {
			if (errno == EINTR)
				continue;
			else {
				if (errno != EWOULDBLOCK && errno != EAGAIN)
					sz_log(3, "Failed to read from cmd pipe, errno:%d", errno);
				break;
			}
		}

		boost::mutex::scoped_lock lock(m_mutex);
		while (m_queue.size()) {
			command cmd = m_queue.front();
			m_queue.pop_front();

			switch (cmd.cmd) {
				case ADD_CMD:
					if (boost::filesystem::exists(cmd.path)) {
						wd = inotify_add_watch(m_inotify_socket, cmd.path.c_str(), IN_MOVED_TO | IN_MODIFY);
					} else {
						boost::filesystem::path dest_dir = boost::filesystem::absolute(cmd.path);;
						boost::filesystem::path current_watch = dest_dir;

						while (!boost::filesystem::exists(current_watch)) {
							current_watch = current_watch.parent_path();
						}

						if (dest_dir == current_watch) {
							wd = inotify_add_watch(m_inotify_socket, cmd.path.c_str(), IN_MOVED_TO | IN_MODIFY);
						} else {
							wd = inotify_add_watch(m_inotify_socket, current_watch.native().c_str(), IN_CREATE);
						}
					}
					if (wd < 0) {
						sz_log(3, "Failed to add watch for path:%s, errno: %d", cmd.path.c_str(), errno);
						m_monitor->failed_to_register_dir(cmd.param, cmd.observer, cmd.path);
					} else
						m_monitor->dir_registered(wd, cmd.param, cmd.observer, cmd.path, cmd.order);
					break;
				case DEL_CMD:
					if (inotify_rm_watch(m_inotify_socket, cmd.token))
						sz_log(3, "Failed to remove watch, errno: %d", errno);
					break;
				case END_CMD:
					m_terminate = true;
					close(m_inotify_socket);
					close(m_cmd_socket[0]);
					close(m_cmd_socket[1]);
					return;
			}
		}
	}
}

void SzbParamMonitorImpl::loop() {
	while (!m_terminate) {
		struct pollfd fds[2];

		memset(fds, 0, sizeof(fds));

		fds[0].fd = m_inotify_socket;
		fds[0].events = POLLIN;
		
		fds[1].fd = m_cmd_socket[1];
		fds[1].events = POLLIN;

		int r = poll(fds, 2, -1);
		if (r == -1)  {
			if (errno == EINTR)
				continue;
			else
				throw std::runtime_error("SzbParamMonitorImpl::poll failed");
		}

		if (fds[0].revents & POLLIN)
			process_notification();

		if (fds[1].revents & POLLIN)
			process_cmds();
	}
}

void SzbParamMonitorImpl::run() {
	m_thread = boost::thread(boost::bind(&SzbParamMonitorImpl::loop, this));
}

bool SzbParamMonitorImpl::trigger_command_pipe() {
again:
	ssize_t r = write(m_cmd_socket[0], "a", 1);
	if (r == -1)
		switch (errno) {
			case EINTR:
				goto again;
			case EWOULDBLOCK:
				break;
			default: {
				sz_log(4, "Failed to write to command pipe errno: %d", errno);
				boost::mutex::scoped_lock lock(m_mutex);
				m_queue.pop_back();
				return false;
			}
		}
	return true;
}

#else

SzbParamMonitorImpl::SzbParamMonitorImpl(SzbParamMonitor *monitor, const std::string& szarp_data_dir)
			: m_monitor(monitor), m_token(0), m_szarp_data_dir(szarp_data_dir)
{
	m_dir_handle = CreateFile(
			szarp_data_dir.c_str(), 
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			NULL);

	if (m_dir_handle == INVALID_HANDLE_VALUE)
		throw std::runtime_error("Failed to open directory to monitor");

	std::memset(&m_overlapped, 0, sizeof(m_overlapped));

	m_event[0] = CreateEvent(NULL, true, false, "monitor event");
	if (m_overlapped.hEvent == INVALID_HANDLE_VALUE) {
		CloseHandle(m_dir_handle);
		throw std::runtime_error("Failed to open directory to monitor");
	}

	m_event[1] = CreateEvent(NULL, true, false, "signal event");
	if (m_event[1] == INVALID_HANDLE_VALUE) {
		CloseHandle(m_dir_handle);
		CloseHandle(m_event[1]);
		throw std::runtime_error("Failed to open directory to monitor");
	}

	wait_for_changes();
}

void SzbParamMonitorImpl::wait_for_changes() {
	ResetEvent(m_event[0]);
	std::memset(&m_overlapped, 0, sizeof(m_overlapped));

	m_overlapped.hEvent = m_event[0];

	auto ret = ReadDirectoryChangesW(m_dir_handle, m_buffer, sizeof(m_buffer), true,
					FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE |
					FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_LAST_WRITE,
					nullptr, &m_overlapped, nullptr);
	if (!ret)
		throw std::runtime_error("Failed to intiate listening for directory chagnes");
}

SzbParamMonitorImpl::~SzbParamMonitorImpl() {
	CloseHandle(m_dir_handle);
	CloseHandle(m_event[0]);
	CloseHandle(m_event[1]);
}

void SzbParamMonitorImpl::loop() {
	while (!m_terminate) {
		auto r = WaitForMultipleObjects(2, m_event, false, INFINITE);
		switch (r) {
			case WAIT_OBJECT_0:
				process_notification();
				break;

			case WAIT_OBJECT_0 + 1:
				process_cmds();
				break;

			default:
				throw std::runtime_error("Invalid return value from WaitForMultiplesObjects");
		}
	}
}

void SzbParamMonitorImpl::process_file(PFILE_NOTIFY_INFORMATION info, std::vector<std::pair<SzbMonitorTokenType, std::string>>& tokens_and_paths) {
	if (info->Action != FILE_ACTION_ADDED
			&& info->Action != FILE_ACTION_MODIFIED
			&& info->Action != FILE_ACTION_RENAMED_NEW_NAME)
		return;

	size_t length = info->FileNameLength / sizeof(info->FileName[0]);
	if (length < 4)
		return;

	WCHAR* name = info->FileName;

	if (name[length - 4] != L'.')
		return;

	if (name[length - 3] != L's')
		return;

	if (name[length - 2] != L'z')
		return;

	if (name[length - 1] != L'b' && name[length - 1] != L'4')
		return;

	boost::filesystem::path path(name, name + length);
	auto dir = path.parent_path();
	auto i = m_registry.find(dir.wstring());
	if (i == m_registry.end()) 
		return;

	tokens_and_paths.push_back(std::make_pair(i->second, (m_szarp_data_dir / path).string()));
}

void SzbParamMonitorImpl::process_notification() {
	DWORD bytes_tx;
	auto ret = GetOverlappedResult(m_dir_handle, &m_overlapped, &bytes_tx, true);
	if (!ret)
		throw std::runtime_error("Failed to get the result");

	PFILE_NOTIFY_INFORMATION info = (PFILE_NOTIFY_INFORMATION)m_buffer;

	std::vector<std::pair<SzbMonitorTokenType, std::string>> tokens_and_paths;

	while (true) {
		process_file(info, tokens_and_paths);

		if (!info->NextEntryOffset)
			break;

		info = PFILE_NOTIFY_INFORMATION((uint8_t*) info + info->NextEntryOffset);
	}

	if (tokens_and_paths.size())
		m_monitor->files_changed(tokens_and_paths);

	wait_for_changes();
}

void SzbParamMonitorImpl::process_cmds() {
	boost::mutex::scoped_lock lock(m_mutex);
	while (m_queue.size()) {
		command cmd = m_queue.front();
		m_queue.pop_front();

		switch (cmd.cmd) {
			case ADD_CMD:
				add_dir(cmd.path, cmd.param, cmd.observer, cmd.order);
				break;
			case DEL_CMD:
				del_dir(cmd.token);
				break;
			case END_CMD:
				m_terminate = true;
				break;
		}
	}

	ResetEvent(m_event[1]);
}

bool SzbParamMonitorImpl::trigger_command_pipe() {
	SetEvent(m_event[1]);
	return true;
}

void SzbParamMonitorImpl::add_dir(const std::string& path, TParam* param, SzbParamObserver* observer, unsigned order) {
	boost::filesystem::path root_dir(m_szarp_data_dir);
	boost::filesystem::path dir_path(path);

	auto root_iter = root_dir.begin();
	auto path_iter = dir_path.begin();

	while (root_iter != root_dir.end() && path_iter != dir_path.end() && *root_iter == *path_iter) {
		root_iter++;
		path_iter++;
	}

	boost::filesystem::path relative_path;
	while (path_iter != dir_path.end()) {
		relative_path /= *path_iter;
		path_iter++;
	}

	auto token = m_token++;

	m_registry.insert(std::make_pair(relative_path.wstring(), token));
	m_monitor->dir_registered(token, param, observer, path, order);
}

void SzbParamMonitorImpl::del_dir(SzbMonitorTokenType token) {
	auto i = m_registry.begin();
	while (i != m_registry.end()) {
		if (i->second == token)
			i = m_registry.erase(i);
		else
			i++;
	}
}

void SzbParamMonitorImpl::run()
{
	m_thread = boost::thread(boost::bind(&SzbParamMonitorImpl::loop, this));
}

#endif

SzbParamMonitor::SzbParamMonitor(const std::wstring& szarp_data_dir) : m_monitor_impl(this, SC::S2L(szarp_data_dir)) {
	m_monitor_impl.run();
}

void SzbParamMonitor::dir_registered(SzbMonitorTokenType token, TParam *param, SzbParamObserver* observer, const std::string& path, unsigned order) {
	boost::mutex::scoped_lock lock(m_mutex);

	m_token_observer_param[token].insert(std::make_pair(order, std::make_pair(observer, param)));
	m_observer_token_param[observer].push_back(std::tr1::make_tuple(token, param, path));
	m_path_token[path] = token;
	m_token_path[token] = path;

	m_cond.notify_one();
}

void SzbParamMonitor::failed_to_register_dir(TParam *param, SzbParamObserver* observer, const std::string& path) {
	boost::mutex::scoped_lock lock(m_mutex);
	m_cond.notify_one();
}

void SzbParamMonitor::files_changed(const std::vector<std::pair<SzbMonitorTokenType, std::string> >& tokens_and_paths) {
	boost::mutex::scoped_lock lock(m_mutex);
	for (std::vector<std::pair<SzbMonitorTokenType, std::string> >::const_iterator i = tokens_and_paths.begin();
			i != tokens_and_paths.end();
			i++) {
		std::multimap<unsigned, std::pair<SzbParamObserver*, TParam*> >& opv = m_token_observer_param[i->first];
		for (std::multimap<unsigned, std::pair<SzbParamObserver*, TParam*> >::iterator j = opv.begin();
				j != opv.end();
				j++)
			j->second.first->param_data_changed(j->second.second, i->second);
	}
}

void SzbParamMonitor::add_observer(SzbParamObserver* observer, TParam* param, const std::string& path, unsigned order) {
	std::vector<std::string> vp(1, path);
	add_observer(observer, std::vector<std::pair<TParam*, std::vector<std::string> > >(1, std::make_pair(param, vp)), order);
}

void SzbParamMonitor::add_observer(SzbParamObserver* observer, const std::vector<std::pair<TParam*, std::vector<std::string> > >& observed, unsigned order) {
	boost::mutex::scoped_lock lock(m_mutex);
	for (std::vector<std::pair<TParam*, std::vector<std::string> > >::const_iterator i = observed.begin();
			i != observed.end();
			i++)
		for (std::vector<std::string>::const_iterator j = i->second.begin();
				j != i->second.end();
				j++) {
			std::tr1::unordered_map<std::string, SzbMonitorTokenType>::iterator k = m_path_token.find(*j);
			if (k == m_path_token.end()) {
				if (m_monitor_impl.start_monitoring_dir(*j, i->first, observer, order))
					m_cond.wait(lock);
				else
					sz_log(4, "Failed to to add dir to monitor");
			} else {
				m_token_observer_param[k->second].insert(std::make_pair(order, std::make_pair(observer, i->first)));
				m_observer_token_param[observer].push_back(std::tr1::make_tuple(k->second, i->first, *j));
			}
		}
}

void SzbParamMonitor::remove_observer(SzbParamObserver* obs) {
	boost::mutex::scoped_lock lock(m_mutex);
	std::vector<std::tr1::tuple<SzbMonitorTokenType, TParam*, std::string> >& tpv = m_observer_token_param[obs];
	for (std::vector<std::tr1::tuple<SzbMonitorTokenType, TParam*, std::string> >::iterator i = tpv.begin();
			i != tpv.end();
			i++) {
		SzbMonitorTokenType token = std::tr1::get<0>(*i);
		const std::string& path = std::tr1::get<2>(*i);
		std::multimap<unsigned, std::pair<SzbParamObserver*, TParam*> >& opv = m_token_observer_param[token];
		if (opv.size() == 1) {
			m_monitor_impl.end_monitoring_dir(token);

			m_path_token.erase(path);
			m_token_path.erase(token);
			m_token_observer_param.erase(token);
		} else {
			for (std::multimap<unsigned, std::pair<SzbParamObserver*, TParam*> >::iterator j = opv.begin();
					j != opv.end();
					j++)
				if (j->second.first == obs) {
					opv.erase(j);
					break;
				}
		}
	}
	m_observer_token_param.erase(obs);
}

void SzbParamMonitor::modify_dir_token(SzbMonitorTokenType old_wd, SzbMonitorTokenType new_wd) {
	boost::mutex::scoped_lock lock(m_mutex);

	std::multimap<unsigned, std::pair<SzbParamObserver*, TParam*> >& opv = m_token_observer_param[old_wd];
	for (std::multimap<unsigned, std::pair<SzbParamObserver*, TParam*> >::iterator j = opv.begin(); j != opv.end(); j++) {
		m_token_observer_param[new_wd].insert(*j);
		std::vector<std::tr1::tuple<SzbMonitorTokenType, TParam*, std::string> >& tpv = m_observer_token_param[j->second.first];
		for (std::vector<std::tr1::tuple<SzbMonitorTokenType, TParam*, std::string> >::iterator i = tpv.begin();
				i != tpv.end();
				i++) {
			if (old_wd == std::tr1::get<0>(*i))
				std::tr1::get<0>(*i) = new_wd;
		}
	}
	m_token_observer_param.erase(old_wd);
	std::string path = m_token_path[old_wd];
	m_token_path[new_wd] = path;
	m_token_path.erase(old_wd);
	m_path_token[path] = new_wd;
}

void SzbParamMonitor::terminate() {}

SzbParamMonitor::~SzbParamMonitor() {
	m_monitor_impl.terminate();
}
