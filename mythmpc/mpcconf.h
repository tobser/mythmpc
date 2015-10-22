#ifndef MYTHMPC_CONF_H
#define MYTHMPC_CONF_H

#include <mythcontext.h>
#include <mythtv/libmythui/mythscreentype.h>
#include <mythtv/libmythui/mythuibutton.h>
#include <mythtv/libmythui/mythuispinbox.h>
#include <mythtv/libmythui/mythuitext.h>
#include "mpcui.h"


class MpcConf : public MythScreenType
{
    Q_OBJECT

    public:
        MpcConf(MythScreenStack *parent);
        bool create(void);

    private slots:
        void onEditCompleted();

    private:
        MythUIButton     *m_Ok;
        MythUIButton     *m_Cancel;
        MythUITextEdit   *m_HostEdit;
        MythUITextEdit   *m_PortEdit;
        MythUITextEdit   *m_PassEdit;
};

#endif /* MYTHTEATIME_UI_H*/
