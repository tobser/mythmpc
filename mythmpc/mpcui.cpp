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



#include <QSignalMapper>
// MythMpc headers
#include "mpcui.h"
#include "mpcconf.h"

Mpc *gMythMpc = NULL;

Mpc::Mpc(MythScreenStack *parent):
    MythScreenType(parent, "mpc"),
    m_Stop(NULL),
    m_PlayPause(NULL),
    m_Next(NULL),
    m_Prev(NULL),
    m_Config(NULL),
    m_InfoText(NULL),
    m_TitleText(NULL),
    m_ButtonList(NULL),
    m_Mpc(NULL),
    m_PollTimer(NULL),
    m_PollTimeout(50),
    m_VolUpDownStep(5),
    m_VolUp(NULL),
    m_VolDown(NULL)
{
    gMythMpc = this;
}
Mpc::~Mpc()
{
    if (m_Mpc){
        mpd_free(m_Mpc);
        m_Mpc = NULL;
    }
    gMythMpc = NULL;
}
void mpc_error_callback(MpdObj * /* mi */,int errorid, char *msg, void * /*userdata*/)
{
   LOG_RX(QString ("ERROR '%1': %2").arg(errorid).arg(msg));
}

QString Mpc::whatToString(ChangedStatusType w){
    QString r = QString("Changed (0x%1): ").arg(w, 0, 16);;
	r += (w & MPD_CST_PLAYLIST           ? "|PLAYLIST" : "");
	r += (w & MPD_CST_SONGPOS            ? "|SONGPOS" : "");
	r += (w & MPD_CST_SONGID             ? "|SONGID" : "");
	r += (w & MPD_CST_DATABASE           ? "|DATABASE" : "");
	r += (w & MPD_CST_UPDATING           ? "|UPDATING" : "");
	r += (w & MPD_CST_VOLUME             ? "|VOLUME" : "");
	r += (w & MPD_CST_TOTAL_TIME         ? "|TOTAL_TIME" : "");
	r += (w & MPD_CST_ELAPSED_TIME       ? "|ELAPSED_TIME" : "");
	r += (w & MPD_CST_CROSSFADE          ? "|CROSSFADE" : "");
	r += (w & MPD_CST_RANDOM             ? "|RANDOM" : "");
	r += (w & MPD_CST_REPEAT             ? "|REPEAT" : "");
	r += (w & MPD_CST_AUDIO              ? "|AUDIO" : "");
	r += (w & MPD_CST_STATE              ? "|STATE" : "");
	r += (w & MPD_CST_PERMISSION         ? "|PERMISSION" : "");
	r += (w & MPD_CST_BITRATE            ? "|BITRATE" : "");
	r += (w & MPD_CST_AUDIOFORMAT        ? "|AUDIOFORMAT" : "");
	r += (w & MPD_CST_STORED_PLAYLIST	 ? "|STORED_PLAYLIST" : "");
    r += (w & MPD_CST_SERVER_ERROR       ? "|SERVER_ERROR" : "");
    r += (w & MPD_CST_OUTPUT             ? "|OUTPUT" : "");
    r += (w & MPD_CST_STICKER            ? "|STICKER" : "");
    r += (w & MPD_CST_NEXTSONG           ? "|NEXTSONG" : "");
    r += (w & MPD_CST_SINGLE_MODE        ? "|SINGLE_MODE" : "");
    r += (w & MPD_CST_CONSUME_MODE       ? "|CONSUME_MODE" : "");
    r += (w & MPD_CST_REPLAYGAIN         ? "|REPLAYGAIN" : "");
    return r + "|";
}

void Mpc::updateSongInfo(){
        mpd_Song *song = mpd_playlist_get_current_song(m_Mpc);
        if(!song)
            return;

        QString playing(song->artist);
        updateInfo("file",  song->file);
        updateInfo("artist",  song->artist);
        updateInfo("title",  song->title);
        updateInfo("album",  song->album);
        updateInfo("track",  song->track);
        updateInfo("name",  song->name);
        updateInfo("date",  song->date);
        updateInfo("genre",  song->genre);
        updateInfo("composer",  song->composer);
        updateInfo("performer",  song->performer);
        updateInfo("disc", song->disc);
        updateInfo("comment",  song->comment);
        updateInfo("albumartist",  song->albumartist);
        updateInfo("time",QString::number(song->time));
        updateInfo("pos", QString::number(song->pos));
        updateInfo("id",  QString::number(song->id));
}

void Mpc::updatePlaybackState(){
    switch(mpd_player_get_state(m_Mpc))
    {
        case MPD_PLAYER_PLAY:
            updateInfo("playback", "Playing");
            break;
        case MPD_PLAYER_PAUSE:
            updateInfo("playback", "Paused");
            break;
        case MPD_PLAYER_STOP:
            updateInfo("playback", "Stopped");
            break;
        default:
            break;
    }
}

void Mpc::updateInfo(QString key, QString value){
    if (stateInfo[key] == value)
        return;

    LOG_RX(key + ": '" + stateInfo[key] + "' => '" + value + "'");
    stateInfo[key] = value;
    updateUi();
}

void Mpc::updateUi(){
    if (stateInfo["playback"] == "Stopped" || stateInfo["playback"] == "Paused")
        m_PlayPause->SetText(tr("Play"));
    else
        m_PlayPause->SetText(tr("Pause"));

    if(m_TitleText)
        m_TitleText->SetTextFromMap(stateInfo);
    if (m_InfoText)
        m_InfoText->SetTextFromMap(stateInfo);

//    LOG_("UI Update");
//    QList<MythUIText *> allText = findChildren<MythUIText *>();
//    foreach(MythUIText* t, allText){
//        //if (t != 0)
//            //LOG_TX(t->objectName() + " (" + t->metaObject()->className() +")");
//    }
//    LOG_("UI Update ende \n");
}

void mpc_status_changed(MpdObj *mi, ChangedStatusType what)
{
    if (gMythMpc == NULL)
        return;

    if(what&MPD_CST_SONGID)
        gMythMpc->updateSongInfo();

    if(what&MPD_CST_STATE)
        gMythMpc->updatePlaybackState();

    if(what&MPD_CST_REPEAT)
        gMythMpc->updateInfo("repeat", (mpd_player_get_repeat(mi)? "On":"Off"));

    if(what&MPD_CST_RANDOM)
        gMythMpc->updateInfo("random", (mpd_player_get_random(mi)? "On":"Off"));

    if(what&MPD_CST_VOLUME)
        gMythMpc->updateInfo("volume", QString::number((mpd_status_get_volume(mi))));

    if(what&MPD_CST_CROSSFADE)
        gMythMpc->updateInfo ("x-fade", QString::number(mpd_status_get_crossfade(mi)));

    if(what&MPD_CST_TOTAL_TIME)
    {
        int total = mpd_status_get_total_song_time(mi);
        gMythMpc->updateInfo("song_total", QString::number(total));
        gMythMpc->updateInfo("song_total_min", QString("%1").arg(total/60, 2, 10, QChar('0')));
        gMythMpc->updateInfo("song_total_sec", QString("%1").arg(total%60, 2, 10, QChar('0')));
    }

    if(what&MPD_CST_ELAPSED_TIME){
        int elapsed = mpd_status_get_elapsed_song_time(mi);
        gMythMpc->updateInfo("song_elapsed", QString::number(elapsed));
        gMythMpc->updateInfo("song_elapsed_min", QString("%1").arg(elapsed/60, 2, 10, QChar('0')));
        gMythMpc->updateInfo("song_elapsed_sec", QString("%1").arg(elapsed%60, 2, 10, QChar('0')));
    }

    if(what&MPD_CST_BITRATE){
        gMythMpc->updateInfo("bits", QString::number(
                    mpd_status_get_bits(mi)));
    }

    if(what&MPD_CST_UPDATING)
    {
        if(mpd_status_db_is_updating(mi))
            LOG_RX("Started updating DB");
        else
            LOG_RX("Updating DB finished");
    }
    if(what&MPD_CST_DATABASE)
        LOG_RX("Databased changed");

    if(what&MPD_CST_PLAYLIST)
    {
        LOG_RX("Playlist changed");
        gMythMpc->updateSongInfo();
    }

    /* not yet implemented signals */
    if(what&MPD_CST_AUDIO)
        LOG_RX("Audio Changed");

    if(what&MPD_CST_PERMISSION)
        LOG_RX("Permission: Changed");
    if(what&MPD_CST_ELAPSED_TIME){
        /*LOG_TX("Time elapsed changed:"RESET" %02i:%02i\n",
          mpd_status_get_elapsed_song_time(mi)/60,
          mpd_status_get_elapsed_song_time(mi)%60); */
    }
}

bool Mpc::create(void)
{
    // Load the theme for this screen
    bool foundtheme = false;
    foundtheme = LoadWindowFromXML("mpcui.xml", "mpc", this);
    if (!foundtheme)
    {
        LOG_("window mpc in mpcui.xml is missing."); 
        return  false;
    }

    UIUtilW::Assign(this, m_InfoText, "infotext");
    if (m_InfoText)
        m_InfoText->SetText(tr("useless text"));

    UIUtilW::Assign(this, m_TitleText, "title");
    if (m_TitleText)
        m_TitleText->SetText(tr("a title"));

    bool err = false;

    UIUtilE::Assign(this, m_Stop, "stop", &err);
    UIUtilE::Assign(this, m_PlayPause, "play_pause", &err);
    UIUtilE::Assign(this, m_Next, "next", &err);
    UIUtilE::Assign(this, m_Prev, "previous", &err);
    UIUtilE::Assign(this, m_VolUp, "volume_up", &err);
    UIUtilE::Assign(this, m_VolDown, "volume_down", &err);
    UIUtilE::Assign(this, m_Config, "config", &err);
    if (err)
    {
        LOG_("Theme is missing required elements.");
        return  false;
    }

    connect(m_ButtonList, SIGNAL(itemClicked(MythUIButtonListItem *)),
            this, SLOT(itemClicked(MythUIButtonListItem *)));

    connect(m_Stop,      SIGNAL(Clicked()), this, SLOT(stop()));
    connect(m_PlayPause, SIGNAL(Clicked()), this, SLOT(togglePlay()));
    connect(m_Next,      SIGNAL(Clicked()), this, SLOT(next()));
    connect(m_Prev,      SIGNAL(Clicked()), this, SLOT(prev()));
    connect(m_VolUp,     SIGNAL(Clicked()), this, SLOT(volUp()));
    connect(m_VolDown,   SIGNAL(Clicked()), this, SLOT(volDown()));
    connect(m_Config,    SIGNAL(Clicked()), this, SLOT(openConfigWindow()));


    BuildFocusList();
    SetFocusWidget(m_PlayPause);

    QString host = gCoreContext->GetSetting("mpd-host", "localhost");
    int port = gCoreContext->GetSetting("mpd-port", "6600").toInt();
    QString pass = gCoreContext->GetSetting("mpd-pass", "");


    m_Mpc = mpd_new((char*) host.toLatin1().data(),port, (char*)pass.toLatin1().data());
    mpd_signal_connect_error(m_Mpc,(ErrorCallback) mpc_error_callback , NULL);
    mpd_signal_connect_status_changed(m_Mpc, (StatusChangedCallback) mpc_status_changed, NULL);
    mpd_set_connection_timeout(m_Mpc, 10);
    mpd_connect(m_Mpc);

    m_PollTimer = new QTimer(this);
    connect(m_PollTimer, SIGNAL(timeout()), this, SLOT(poll()));
    m_PollTimer->start(m_PollTimeout);

    LOG_("created...");
    return true;
}

void Mpc::poll(){
   mpd_status_update(m_Mpc);
   m_PollTimer->start(m_PollTimeout);
}
void Mpc::stop(){
    LOG_TX("stop");
    mpd_player_stop(m_Mpc);
    poll();
}

void Mpc::togglePlay(){
    switch(mpd_player_get_state(m_Mpc))
    {
        case MPD_PLAYER_PLAY:
            LOG_TX("play => pause");
            mpd_player_pause(m_Mpc);
            break;
        case MPD_PLAYER_PAUSE:
            LOG_TX("pause => play");
            mpd_player_pause(m_Mpc);
            break;
        case MPD_PLAYER_STOP:
            LOG_TX("stopped => play");
            mpd_player_play(m_Mpc);
            break;
        default:
            break;
    }
    poll();
}
void Mpc::next(){
    LOG_TX("next");
    mpd_player_next(m_Mpc);
    poll();
};
void Mpc::prev(){
    LOG_TX("prev");
    mpd_player_prev(m_Mpc);
    poll();
}
void Mpc::volUp(){
    changeVolume(+ m_VolUpDownStep);
}
void Mpc::volDown() {
    changeVolume( - m_VolUpDownStep);
}
void Mpc::changeVolume(int vol){
    int current = mpd_status_get_volume(m_Mpc);
    if (current < 0) return;

    int newVol  = current + vol;
    if (newVol > 100) newVol = 100;
    if (newVol < 0) newVol = 0;
    if (current == newVol)
        return;

    LOG_TX(QString("Volume %1%% => %2%%").arg(current).arg(newVol));
    mpd_status_set_volume(m_Mpc, newVol);
    poll();
}

void Mpc::openConfigWindow(){
    MythScreenStack *st = GetMythMainWindow()->GetMainStack();
    if (!st)
    {
        LOG_("Could not get main stack.");
        return;
    }

    MpcConf* conf = new MpcConf(st);
    if (!conf->create())
    {
        LOG_("Could not create MPC UI.");
        delete conf;
        conf = NULL;
        return;
    }
    st->AddScreen(conf);
}
