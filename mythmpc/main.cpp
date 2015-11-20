// C++ headers
#include <unistd.h>

// QT headers
#include <QApplication>
#include <QTimer>
#include <QString>

// MythTV headers
#include <mythtv/mythcontext.h>
#include <mythtv/mythplugin.h>
#include <mythtv/mythpluginapi.h>
#include <mythtv/mythversion.h>
#include <mythtv/libmythui/mythscreenstack.h>
#include <mythtv/libmythui/mythmainwindow.h>

#include "mpcui.h"

static void openMpcScreen(void);
static void setupKeys(void)
{
    REG_JUMPEX(QT_TRANSLATE_NOOP("MythControls", "MPC"),
            "Open MPD Client Plugin", "" , openMpcScreen, false);

    LOG_("Registered JumpPoint \"MPC\"");
}

int mythplugin_init(const char *libversion)
{
    if (!gCoreContext->TestPluginVersion("mythmpc",
        libversion, MYTH_BINARY_VERSION))
        return -1;

    setupKeys();
    LOG_("mpc plugin started.");
    return 0;
}

int mythplugin_run(void)
{
    openMpcScreen();
    return 0;
}

int mythplugin_config(void)
{
    return 0;
}

void mythplugin_destroy(void)
{
}

static void openMpcScreen(void)
{
    MythScreenStack *st = GetMythMainWindow()->GetMainStack();
    if (!st)
    {
        LOG_("Could not get main stack.");
        return;
    }

    QString result = st->GetLocation(true);
    if (result.contains("Playback"))
    {
            LOG_("Can not create MPC UI while in playback.");
            return;
    }

    if (result.contains("mpc"))
        return;

    Mpc* mpc = new Mpc(st);
    if (!mpc->create())
    {
        LOG_("Could not create MPC UI.");
        delete mpc;
        mpc = NULL;
        return;
    }
    st->AddScreen(mpc);
}

