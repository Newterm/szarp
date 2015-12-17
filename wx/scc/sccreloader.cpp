
#include "sccreloader.h"
#include "sccapp.h"

ReloadTimer::ReloadTimer(SCCApp* sccPtr): wxTimer(), sccApp(sccPtr)
{	
	this->wxTimer::Start(200000, false);
}

void ReloadTimer::Notify() 
{
	if (this->sccApp != nullptr) {
		this->sccApp->ReloadMenu();
	} else {
		delete this;
	}
}
