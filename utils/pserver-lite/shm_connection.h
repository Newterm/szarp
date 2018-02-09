#ifndef SHM_CONNECTION_H
#define SHM_CONNECTION_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "szarp_config.h"
#include "exception.h"

#include <string>
#include <vector>
#include <algorithm>
#include <memory>

#include <cstdint>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>

extern volatile sig_atomic_t g_signals_blocked;
extern volatile sig_atomic_t g_signum_received;

class ShmError : public SzException {
	SZ_INHERIT_CONSTR(ShmError, SzException)
};

class ShmConnection {
	public:
		ShmConnection();
		
		void register_signals();
		
		bool configure();
		bool connect();

		void update_segment();
	
		int param_index_from_path(std::string path);

		std::vector<int16_t> get_values(int param_index);	
		
		int get_pos();
		int get_count();

		bool registered()
		{ return _registered; }
		bool configured()
		{ return _configured; }
		bool connected()
		{ return _connected; }

	private:
		bool _registered;
		bool _configured;
		bool _connected;
		
		std::unique_ptr<TSzarpConfig> _szarp_config;
			
		bool shm_open(struct sembuf* semaphores);
		void shm_close(struct sembuf* semaphores);
		int16_t* attach();
		void detach(int16_t** segment);

		std::string _parcook_path;
		
		int _shm_desc;			
		int _sem_desc;			

		int _values_count;		
		int _params_count;

		std::vector<int16_t> _shm_segment;
};

#endif /*SHM_CONNECTION_H*/
