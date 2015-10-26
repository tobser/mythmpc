
// POSIX headers
#include <unistd.h>

// MythTV headers
#include <mythtv/mythcontext.h>
#include <mythtv/mythcoreutil.h>
#include <mythtv/libmythui/mythuibutton.h>
#include <mythtv/libmythui/mythuitext.h>
#include <mythtv/libmythui/mythmainwindow.h>
#include <mythtv/mythdirs.h>
#include <mythtv/libmythui/mythdialogbox.h>
#include <mythtv/libmythui/mythuiutils.h>

// MythMpc headers
#include "mpcconf.h"

MpcConf::MpcConf(MythScreenStack *parent):
    MythScreenType(parent, "mpcconf"),
       m_Ok(NULL),
       m_Cancel(NULL),
       m_HostEdit(NULL),
       m_PortEdit(NULL),
       m_PassEdit(NULL)
{
}


bool MpcConf::create(void)
{
    // Load the theme for this screen
    bool foundtheme = false;
    foundtheme = LoadWindowFromXML("mpcui.xml", "mpc_config", this);
    if (!foundtheme)
    {
        LOG_("window mpc in mpcui.xml is missing."); 
        return  false;
    }

    bool err = false;
    UIUtilE::Assign(this, m_Ok, "ok", &err);
    UIUtilE::Assign(this, m_Cancel, "cancel", &err);
    UIUtilE::Assign(this, m_HostEdit, "mpd_host", &err);
    UIUtilE::Assign(this, m_PortEdit, "mpd_port", &err);
    UIUtilE::Assign(this, m_PassEdit, "mpd_pass", &err);
    if (err)
    {
        LOG_("Theme is missing required elements.");
        return  false;
    }

    connect(m_Cancel,      SIGNAL(Clicked()), this, SLOT(Close()));
    connect(m_Ok, SIGNAL(Clicked()), this, SLOT(onEditCompleted()));

    BuildFocusList();
    SetFocusWidget(m_HostEdit);

    QString host = gCoreContext->GetSetting("mpd-host", "localhost");
    QString port = gCoreContext->GetSetting("mpd-port", "6600");
    QString pass = gCoreContext->GetSetting("mpd-pass", "");

    m_HostEdit->SetText(host);
    m_PortEdit->SetText(port);
    m_PassEdit->SetText(pass);

    LOG_("created...");
    return true;
}

void MpcConf::onEditCompleted(){
    gCoreContext->SaveSetting("mpd-host", m_HostEdit->GetText());
    gCoreContext->SaveSetting("mpd-port", m_PortEdit->GetText());
    gCoreContext->SaveSetting("mpd-pass", m_PassEdit->GetText());
    LOG_("save..");
    Close();
}
