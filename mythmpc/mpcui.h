#ifndef MYTHMPC_UI_H
#define MYTHMPC_UI_H

// MythTV headers
#include <mythcontext.h>
#include <mythtv/libmythui/mythscreentype.h>
#include <mythtv/libmythui/mythuibutton.h>
#include <mythtv/libmythui/mythuispinbox.h>
#include <mythtv/libmythui/mythuitext.h>
#include <QTcpSocket>
#include <QTimer>


#ifndef LOG_M
#define LOG_M(level, message) LOG(VB_GENERAL, level, QString("MPC: %1") \
                                    .arg(message))
#endif

class Mpc : public MythScreenType
{
    Q_OBJECT

    public:
        Mpc(MythScreenStack *parent);
        bool create(void);

    public slots:
        void newClicked(void);
        void itemClicked(MythUIButtonListItem *);
        void onEditCompleted(bool started);
        void sendCommand(QString cmd);

    private slots:
        void stop() { sendCommand("stop"); };
        void togglePlay(){ sendCommand("play");};
        void next(){ sendCommand("next");};
        void prev(){ sendCommand("previous");};
        void volUp(){ sendCommand("volume +5");};
        void volDown(){ sendCommand("volume -5");};

    private slots:
        void readFromMpd();
        void displayMpdConnError(QAbstractSocket::SocketError socketError);
        void sendPing();

    private:
        void openEditScreen();
        void poll();

        MythUIButton     *m_Stop;
        MythUIButton     *m_PlayPause;
        MythUIButton     *m_Next;
        MythUIButton     *m_Prev;
        MythUIText       *m_InfoText;
        MythUIText       *m_TitleText;
        MythUIButtonList *m_ButtonList;
        QTcpSocket       *m_MpcSocket;
        QTimer           *m_PingTimer;
        int               m_PingTimeout;
        MythUIButton *m_VolUp;
        MythUIButton *m_VolDown;
};

#endif /* MYTHTEATIME_UI_H*/
