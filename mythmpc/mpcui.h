#ifndef MYTHMPC_UI_H
#define MYTHMPC_UI_H

// MythTV headers
#include <mythcontext.h>
#include <mythtv/libmythui/mythscreentype.h>
#include <mythtv/libmythui/mythuibutton.h>
#include <mythtv/libmythui/mythuispinbox.h>
#include <mythtv/libmythui/mythuitext.h>
#include <mythtv/libmythui/mythuistatetype.h>
#include <mythtv/libmythui/mythuiimage.h>
#include <mythtv/libmythui/mythuiprogressbar.h>
#include <QTimer>
#include <libmpd-1.0/libmpd/libmpd.h>
#include <libmpd-1.0/libmpd/debug_printf.h>

#ifndef _LOG

#define _LOG(message) LOG(VB_GENERAL, LOG_WARNING, message)
#define LOG_(message) _LOG(QString("MPC: ") + message)
#define LOG_TX(message) _LOG(QString("MPC TX: ") + message)
#define LOG_RX(message) _LOG(QString("MPC RX: ") + message)

#endif
class MpcVolumeDialog;

class Mpc : public MythScreenType
{
    Q_OBJECT

    friend class MpcVolumeDialog;

    public:
        Mpc(MythScreenStack *parent);
        ~Mpc();
        bool create(void);
        static QString whatToString(ChangedStatusType);
        InfoMap stateInfo;
        void updateSongInfo();
        void updatePlaylist();
        void updatePlaybackState();
        void updateInfo(QString key, QString value);
        void updateTotalSongTime();
        QString toMinSecString(int seconds);

    public slots:
        void newClicked(void);
        void itemClicked(MythUIButtonListItem *);
        void onEditCompleted(bool started);

    protected:
        bool keyPressEvent(QKeyEvent *e);

    private slots:
        void stop();
        void play();
        void pause();
        void togglePlay();
        void next();
        void prev();
        void volUp();
        void volDown();
        void toggleMute();
        void readFromMpd();
        void displayMpdConnError(QAbstractSocket::SocketError socketError);
        void poll();
        void openConfigWindow();

    private:
        void changeVolume(int volChange);
        void updateUi();
        void showVolume();
        int getVolume();
        void setMute(bool isMute);
        bool isMuted();
        bool isPlaying(void){ return stateInfo["trackstate"] == "playing"; }
        bool isStopped(void){ return stateInfo["trackstate"] == "stopped"; }
        bool isPaused(void){ return stateInfo["trackstate"] == "paused"; }
        void updatePlaylistSongStates();

        MpdObj           *m_Mpc;
        QTimer           *m_PollTimer;
        int               m_PollTimeout;
        int               m_VolUpDownStep;
        int               m_VolumeBeforeMute;
        int               m_IsMute;
        mpd_Song         *m_CurrentSong;

        // ui
        MythUIButton     *m_StopBtn;
        MythUIButton     *m_PlayBtn;
        MythUIButton     *m_PauseBtn;
        MythUIButton     *m_NextBtn;
        MythUIButton     *m_PrevBtn;
        MythUIButton     *m_RewBtn;
        MythUIButton     *m_FfBtn;
        MythUIButton     *m_ConfigBtn;
        MythUIText       *m_TitleText;
        MythUIText       *m_TimeText;
        MythUIText       *m_InfoText;
        MythUIText       *m_ArtistText;
        MythUIProgressBar     *m_TrackProgress;
        MythUIText            *m_TrackProgressText;
        MythUIStateType       *m_TrackState;
        MythUIStateType       *m_MuteState;
        MythUIStateType       *m_RatingState;
        MythUIText            *m_VolumeText;
        MythUIImage           *m_CoverartImage;
        MpcVolumeDialog       *m_VolumeDialog;
        MythUIButtonList      *m_Playlist;

};

class MPUBLIC MpcVolumeDialog : public MythScreenType
{
    Q_OBJECT
    public:
        MpcVolumeDialog(MythScreenStack *parent, const char *name, Mpc *mpc);
        ~MpcVolumeDialog(void);

        bool Create(void);
        bool keyPressEvent(QKeyEvent *event);
        void updateDisplay();

    protected:
        void increaseVolume(void);
        void decreaseVolume(void);
        void toggleMute(void);

        QTimer            *m_displayTimer;
        MythUIText        *m_messageText;
        MythUIText        *m_volText;
        MythUIStateType   *m_muteState;
        MythUIProgressBar *m_volProgress;
        Mpc               *m_Mpc;
};

#endif /* MYTHTEATIME_UI_H*/
