#ifndef MYTHMPC_UI_H
#define MYTHMPC_UI_H

// MythTV headers
#include <mythcontext.h>
#include <mythtv/libmythui/mythscreentype.h>
#include <mythtv/libmythui/mythuibutton.h>
#include <mythtv/libmythui/mythuispinbox.h>
#include <mythtv/libmythui/mythuitext.h>
#include <QTimer>

#include <libmpd-1.0/libmpd/libmpd.h>
#include <libmpd-1.0/libmpd/debug_printf.h>

#ifndef _LOG

#define _LOG(message) LOG(VB_GENERAL, LOG_WARNING, message)
#define LOG_(message) _LOG(QString("MPC: ") + message)
#define LOG_TX(message) _LOG(QString("MPC TX: ") + message)
#define LOG_RX(message) _LOG(QString("MPC RX: ") + message)

#endif

class Mpc : public MythScreenType
{
    Q_OBJECT

    public:
        Mpc(MythScreenStack *parent);
        ~Mpc();
        bool create(void);
        static QString whatToString(ChangedStatusType);
        InfoMap stateInfo;
        void updateSongInfo();
        void updatePlaybackState();
        void updateInfo(QString key, QString value);

        public slots:
            void newClicked(void);
            void itemClicked(MythUIButtonListItem *);
            void onEditCompleted(bool started);

        private slots:
            void stop();
            void togglePlay();
            void next();
            void prev();
            void volUp();
            void volDown();
            void readFromMpd();
            void displayMpdConnError(QAbstractSocket::SocketError socketError);
            void poll();
            void openConfigWindow();

    private:
        void changeVolume(int volChange);
        void updateUi();

        MythUIButton     *m_Stop;
        MythUIButton     *m_PlayPause;
        MythUIButton     *m_Next;
        MythUIButton     *m_Prev;
        MythUIButton     *m_Config;
        MythUIText       *m_InfoText;
        MythUIText       *m_TitleText;
        MythUIButtonList *m_ButtonList;
        MpdObj           *m_Mpc;
        QTimer           *m_PollTimer;
        int               m_PollTimeout;
        int               m_VolUpDownStep;
        MythUIButton *m_VolUp;
        MythUIButton *m_VolDown;
};

#endif /* MYTHTEATIME_UI_H*/
