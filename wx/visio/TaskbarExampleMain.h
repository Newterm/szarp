#ifndef TASKBAREXAMPLEMAIN_H
#define TASKBAREXAMPLEMAIN_H
#include "wx/taskbar.h"
#include "httpcl.h"
#include <szarp_config.h>
#include "TaskbarExampleApp.h"
#include "conversion.h"
#include "fetchparams.h"
#include "serverdlg.h"
#include "visioParamadd.h"
#include "parlist.h"
#include "TransparentFrame.h"

enum
{
    PU_RESTORE = 10001,
    PU_NEW_ICON,
    PU_OLD_ICON,
    PU_EXIT,
    PU_CHECKMARK,
    PU_SUB1,
    PU_SUB2,
    PU_SUBMAIN,
};

class TaskbarExampleFrame: public TransparentFrame
{
    szHTTPCurlClient *m_http;
    static TSzarpConfig *ipk;
    wxString m_server;
    szVisioAddParam  *m_apd;
    static szParamFetcher *m_pfetcher;
    static szProbeList m_probes;
public:
    TaskbarExampleFrame(wxFrame *frame);
    ~TaskbarExampleFrame();
private:
    virtual void OnClose(wxCloseEvent& event);
    virtual void OnQuit(wxCommandEvent& event);
    virtual void OnAbout(wxCommandEvent& event);
};

#endif // TASKBAREXAMPLEMAIN_H
