
#include "sccreloader.h"
#include "sccapp.h"

ReloadTimer::ReloadTimer(SCCApp* sccPtr, long int period): wxTimer(), sccApp(sccPtr)
{	
	this->wxTimer::Start(period, false);
}

void ReloadTimer::Notify() 
{
	if (this->sccApp != nullptr) {
		this->sccApp->ReloadMenu();
	} else {
		delete this;
	}
}
