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

#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/inotify.h>
#include <sys/poll.h>

#include <vector>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

#include "liblog.h"

#include "szbase/szbparamobserver.h"
#include "szbase/szbparammonitor.h"

typedef int SzbMonitorTokenType;

SzbParamMonitorImpl::SzbParamMonitorImpl(SzbParamMonitor* monitor) : m_monitor(monitor) {
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
	std::vector<SzbMonitorTokenType> tokens;

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
			if (e->len) {
				size_t l = strlen(e->name);
				if (l > 4 && e->name[l - 4] == '.' && e->name[l - 3] == 's' && e->name[l - 2] == 'z'
						&& (e->name[l - 1] == '4' || e->name[l - 1] == 'b'))
					tokens.push_back(e->wd);
			}
			r -= sizeof(*e) + e->len;
			p += sizeof(*e) + e->len;
		}

	}

	if (tokens.size())
		m_monitor->dirs_changed(tokens);
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

		command cmd;
		{
			boost::mutex::scoped_lock lock(m_mutex);
			cmd = m_queue.front();
			m_queue.pop_front();
		}

		switch (cmd.cmd) {
			case ADD_CMD:
				wd = inotify_add_watch(m_inotify_socket, cmd.path.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO);
				if (wd < 0)
					sz_log(3, "Failed to add watch for path:%s, errno: %d", cmd.path.c_str(), errno);
				else
					m_monitor->dir_registered(wd, cmd.param, cmd.observer, cmd.path);
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

void SzbParamMonitorImpl::loop() {
	while (!m_terminate) {
		struct pollfd fds[2];

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
	if (r == -1) {
		if (errno == EINTR)
			goto again;
		else {
			sz_log(4, "Failed to write to command pipe errno: %d", errno);
			boost::mutex::scoped_lock lock(m_mutex);
			m_queue.pop_back();
			return false;
		}
	}
	return true;
}

bool SzbParamMonitorImpl::start_monitoring_dir(const std::string& path, TParam* param, SzbParamObserver* observer) {
	command cmd;
	cmd.cmd = ADD_CMD;
	cmd.path = path;
	cmd.observer = observer;
	cmd.param = param;
	{
		boost::mutex::scoped_lock lock(m_mutex);
		m_queue.push_back(cmd);
	}
	return trigger_command_pipe();
}

bool SzbParamMonitorImpl::end_monitoring_dir(SzbMonitorTokenType token) {
	command cmd;
	cmd.cmd = DEL_CMD;
	cmd.token = token;
	{
		boost::mutex::scoped_lock lock(m_mutex);
		m_queue.push_back(cmd);
	}
	return trigger_command_pipe();
}

bool SzbParamMonitorImpl::terminate() {
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


SzbParamMonitor::SzbParamMonitor() : m_monitor_impl(this) {
	m_monitor_impl.run();
}

void SzbParamMonitor::dir_registered(SzbMonitorTokenType token, TParam *param, SzbParamObserver* observer, const std::string& path) {
	boost::mutex::scoped_lock lock(m_mutex);

	m_token_observer_param[token].push_back(std::make_pair(observer, param));
	m_observer_token_param[observer].push_back(std::tr1::make_tuple(token, param, path));
	m_path_token[path] = token;

	m_cond.notify_one();
}

void SzbParamMonitor::failed_to_register_dir(TParam *param, SzbParamObserver* observer, const std::string& path) {
	m_cond.notify_one();
}

void SzbParamMonitor::dirs_changed(const std::vector<SzbMonitorTokenType>& dirs) {
	boost::mutex::scoped_lock lock(m_mutex);
	for (std::vector<SzbMonitorTokenType>::const_iterator i = dirs.begin();
			i != dirs.end();
			i++) {
		std::vector<std::pair<SzbParamObserver*, TParam*> >& opv = m_token_observer_param[*i];
		for (std::vector<std::pair<SzbParamObserver*, TParam*> >::iterator j = opv.begin();
				j != opv.end();
				j++)
			j->first->param_data_changed(j->second);
	}
}

void SzbParamMonitor::add_observer(SzbParamObserver* observer, const std::vector<std::pair<TParam*, std::vector<std::string> > >& observed) {
	boost::mutex::scoped_lock lock(m_mutex);
	for (std::vector<std::pair<TParam*, std::vector<std::string> > >::const_iterator i = observed.begin();
			i != observed.end();
			i++)
		for (std::vector<std::string>::const_iterator j = i->second.begin();
				j != i->second.end();
				j++) {
			std::tr1::unordered_map<std::string, SzbMonitorTokenType>::iterator k = m_path_token.find(*j);
			if (k == m_path_token.end()) {
				if (m_monitor_impl.start_monitoring_dir(*j, i->first, observer))
					m_cond.wait(lock);
				else
					sz_log(4, "Failed to to add dir to monitor");
			} else {
				m_token_observer_param[k->second].push_back(std::make_pair(observer, i->first));
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
		std::vector<std::pair<SzbParamObserver*, TParam*> >& opv = m_token_observer_param[token];
		if (opv.size() == 1) {
			m_monitor_impl.end_monitoring_dir(token);

			m_path_token.erase(path);
			m_token_observer_param.erase(token);
		} else {
			for (std::vector<std::pair<SzbParamObserver*, TParam*> >::iterator j = opv.begin();
					j != opv.end();
					j++)
				if (j->first == obs) {
					opv.erase(j);
					break;
				}
		}
	}
	m_observer_token_param.erase(obs);
}

SzbParamMonitor::~SzbParamMonitor() {
	m_monitor_impl.terminate();
}
