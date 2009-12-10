#ifdef WX_PRECOMP
#include "wx_pch.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#include "TaskbarExampleApp.h"
#include "TaskbarExampleMain.h"

IMPLEMENT_APP(TaskbarExampleApp);

bool TaskbarExampleApp::OnInit()
{
    TaskbarExampleFrame* frame = new TaskbarExampleFrame(0L);
    frame->Show();
    return true;
}

