#ifndef __SCCRELOADER_H__
#define __SCCRELOADER_H__

#include <wx/timer.h>

class SCCApp;

class ReloadTimer : public wxTimer
{
	SCCApp* sccApp;

public:
	ReloadTimer(SCCApp* sccPtr, long int period = 200000);
	virtual void Notify();
};

#endif
