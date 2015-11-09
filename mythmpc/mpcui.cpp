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
    m_Mpc(NULL), m_PollTimer(NULL), m_PollTimeout(50), m_VolUpDownStep(5),
    m_VolumeBeforeMute(-5), m_IsMute(true), m_StopBtn(NULL), m_PlayBtn(NULL),
    m_PauseBtn(NULL), m_NextBtn(NULL), m_PrevBtn(NULL), m_RewBtn(NULL), m_FfBtn(NULL),
    m_ConfigBtn(NULL), m_TitleText(NULL), m_TimeText(NULL), m_InfoText(NULL),
    m_TrackProgress(NULL), m_TrackProgressText(NULL), m_TrackState(NULL),
    m_MuteState(NULL), m_RatingState(NULL), m_VolumeText(NULL), m_CoverartImage(NULL),
    m_VolumeDialog(NULL), m_Playlist(NULL){
    gMythMpc = this;
}

Mpc::~Mpc(){
    if (m_Mpc){
        mpd_free(m_Mpc);
        m_Mpc = NULL;
    }
    gMythMpc = NULL;
}

void mpc_error_callback(MpdObj * /* mi */,int errorid, char *msg, void * /*userdata*/){
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
    m_CurrentSong = mpd_playlist_get_current_song(m_Mpc);
    if(!m_CurrentSong)
        return;

    updateInfo("file",  m_CurrentSong->file);
    updateInfo("artist",  m_CurrentSong->artist);
    updateInfo("title",  m_CurrentSong->title);
    updateInfo("album",  m_CurrentSong->album);
    updateInfo("track",  m_CurrentSong->track);
    updateInfo("name",  m_CurrentSong->name);
    updateInfo("date",  m_CurrentSong->date);
    updateInfo("genre",  m_CurrentSong->genre);
    updateInfo("composer",  m_CurrentSong->composer);
    updateInfo("performer",  m_CurrentSong->performer);
    updateInfo("disc", m_CurrentSong->disc);
    updateInfo("comment",  m_CurrentSong->comment);
    updateInfo("albumartist",  m_CurrentSong->albumartist);
    updateInfo("pos", QString::number(m_CurrentSong->pos));
    updateInfo("id",  QString::number(m_CurrentSong->id));

    int t  = m_CurrentSong->time;
    if (t == MPD_SONG_NO_TIME)
        updateInfo("song_total", "");
    else
        updateInfo("song_total", toMinSecString(t));
}

void Mpc::updatePlaylist(){
    //long long listId = mpd_playlist_get_playlist_id (m_Mpc);
    m_Playlist->Reset();
    int pivot = 0;
    if (m_CurrentSong)
        pivot = m_CurrentSong->pos;

    int cntElementsBeforAndAfter= 20;
    int start = pivot - cntElementsBeforAndAfter;
    if (start < 0)
        start = 0;
    int  stop = pivot +cntElementsBeforAndAfter ;

    MpdData* d = mpd_playlist_get_song_from_pos_range(m_Mpc, start, stop);
    d = mpd_data_get_first(d);

    while(d != NULL){
        if(d->type == MPD_DATA_TYPE_SONG)
        {
            InfoMap mdata;
            if (d->song->id)
                mdata["id"] = d->song->id;
            if (d->song->artist)
                mdata["artist"] = d->song->artist;
            if (d->song->name)
                mdata["name"] = d->song->name;
            if (d->song->title)
                mdata["title"] = d->song->title;
            if (d->song->album)
                mdata["album"] = d->song->album;

            MythUIButtonListItem *item =
                new MythUIButtonListItem(m_Playlist, " ", qVariantFromValue(d->song->id));

            item->SetTextFromMap(mdata);
        }
        d = mpd_data_get_next(d);
    }

    updatePlaylistSongStates();

}

void Mpc::updatePlaylistSongStates(){
    for (int i = 0; i < m_Playlist->GetCount(); i++){
        MythUIButtonListItem *item = m_Playlist->GetItemAt(i);
        item->SetFontState("normal");
        item->DisplayState("default", "playstate");
        // if this is the current track update its play state to match the player
        if (m_CurrentSong && m_CurrentSong->id == item->GetData())
        {
            if (isPlaying())
            {
                item->SetFontState("running");
                item->DisplayState("playing", "playstate");
            }
            else if (isPaused())
            {
                item->SetFontState("idle");
                item->DisplayState("paused", "playstate");
            }
            else
            {
                item->SetFontState("normal");
                item->DisplayState("stopped", "playstate");
            }

            m_Playlist->SetItemCurrent(item);
        }
    }
}

void Mpc::updatePlaybackState(){
    switch(mpd_player_get_state(m_Mpc))
    {
        case MPD_PLAYER_PLAY:
            updateInfo("trackstate", "playing");
            break;
        case MPD_PLAYER_PAUSE:
            updateInfo("trackstate", "paused");
            break;
        case MPD_PLAYER_STOP:
            updateInfo("trackstate", "stopped");
            break;
        default:
            break;
    }
}

void Mpc::updateInfo(QString key, QString value){
    if (stateInfo[key] == value)
        return;

    if (key != "song_elapsed" && key != "progress")
        LOG_RX(key + ": '" + stateInfo[key] + "' => '" + value + "'");

    QString prev = stateInfo[key];
    stateInfo[key] = value;

    if (key == "song_elapsed" || key == "song_total"){
        stateInfo["time"] =  stateInfo["song_elapsed"];
        if (stateInfo["time"] != "" && stateInfo["song_total"] != "")
            stateInfo["time"] += "/";

        stateInfo["time"] += stateInfo["song_total"];

        m_TimeText->SetTextFromMap(stateInfo);
        return; 
    }

    if (key == "progress"){
        m_TrackProgress->SetTotal(100);
        m_TrackProgress->SetUsed(value.toInt());
    }


    if(key == "title" && m_TitleText){
        m_TitleText->SetTextFromMap(stateInfo);
        return;
    }

    if (key == "info" && m_InfoText){
        m_InfoText->SetTextFromMap(stateInfo);
        return;
    }

    if (key == "artist" && m_ArtistText){
        m_ArtistText->SetTextFromMap(stateInfo);
        return;
    }

    if(key == "trackstate"){
        if (value == "playing"){
            m_PlayBtn->SetEnabled(false);
            m_PlayBtn->SetLocked(true);

            m_PauseBtn->SetEnabled(true);
            m_PauseBtn->SetLocked(false);

            m_StopBtn->SetEnabled(true);
            m_StopBtn->SetLocked(false);
        }
        else if (value == "stopped"){
            m_PlayBtn->SetEnabled(true);
            m_PlayBtn->SetLocked(false);

            m_PauseBtn->SetEnabled(false);
            m_PauseBtn->SetLocked(false);

            m_StopBtn->SetEnabled(false);
            m_StopBtn->SetLocked(true);
        }
        else if (value == "paused"){
            m_PlayBtn->SetEnabled(true);
            m_PlayBtn->SetLocked(false);

            m_PauseBtn->SetEnabled(true);
            m_PauseBtn->SetLocked(true);

            m_StopBtn->SetEnabled(true);
            m_StopBtn->SetLocked(false);
        }

        updatePlaylistSongStates();
        return;
    }

    if (key == "volumepercent"){
        if (!value.startsWith("-"))
            m_VolumeText->SetTextFromMap(stateInfo);

        if(prev != "" && !value.startsWith("-"))
            showVolume();

        return;
    }

    if (key == "mutestate"){
        m_MuteState->DisplayState(value);
        return;
    }
}

void mpc_status_changed(MpdObj *mi, ChangedStatusType what){
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
        gMythMpc->updateInfo("volumepercent", QString::number((mpd_status_get_volume(mi))));

    if(what&MPD_CST_CROSSFADE)
        gMythMpc->updateInfo ("x-fade", QString::number(mpd_status_get_crossfade(mi)));

    if(what&MPD_CST_TOTAL_TIME){
        int total = mpd_status_get_total_song_time(mi);
        gMythMpc->updateInfo("song_total", gMythMpc->toMinSecString(total));
    }

    if(what&MPD_CST_ELAPSED_TIME){
        int elapsed = mpd_status_get_elapsed_song_time(mi);
        gMythMpc->updateInfo("song_elapsed", gMythMpc->toMinSecString(elapsed));

        int total = mpd_status_get_total_song_time(mi);
        int progress = 0;
        if (total <= 0)
            progress = 33;
        else if (elapsed > total)
            progress = 100;
        else if (elapsed <= 0)
            progress = 0;
        else
            progress = ((float)elapsed/(float)total)*100;

        gMythMpc->updateInfo("progress", QString("%1").arg(progress));
    }

    if(what&MPD_CST_BITRATE){
        gMythMpc->updateInfo("bits", QString::number(
                    mpd_status_get_bits(mi)));
    }

    if(what&MPD_CST_UPDATING){
        if(mpd_status_db_is_updating(mi))
            LOG_RX("Started updating DB");
        else
            LOG_RX("Updating DB finished");
    }

    if(what&MPD_CST_DATABASE)
        LOG_RX("Databased changed");

    if(what&MPD_CST_PLAYLIST){
        LOG_RX("Playlist changed");
        gMythMpc->updateSongInfo();
        gMythMpc->updatePlaylist();
    }

    if(what&MPD_CST_AUDIO)
        LOG_RX("Audio Changed");

    if(what&MPD_CST_PERMISSION)
        LOG_RX("Permission: Changed");
}

bool Mpc::create(void){

    bool foundtheme = false;
    foundtheme = LoadWindowFromXML("mpcui.xml", "mpc", this);
    if (!foundtheme)
    {
        LOG_("window mpc in mpcui.xml is missing."); 
        return  false;
    }

    bool err = false;

    UIUtilE::Assign(this, m_TitleText, "title");
    UIUtilE::Assign(this, m_TimeText,      "time", &err);
    UIUtilE::Assign(this, m_InfoText,      "info");
    UIUtilE::Assign(this, m_ArtistText,      "artist");
    UIUtilE::Assign(this, m_RatingState,       "ratingstate", &err);
    UIUtilE::Assign(this, m_TrackProgress,     "progress", &err);
    UIUtilE::Assign(this, m_TrackProgressText, "trackprogress");
    UIUtilE::Assign(this, m_TrackState, "trackstate", &err);
    UIUtilE::Assign(this, m_VolumeText,        "volume", &err);
    UIUtilE::Assign(this, m_MuteState,         "mutestate", &err);
    UIUtilE::Assign(this, m_PrevBtn,    "prev", &err);
    UIUtilE::Assign(this, m_RewBtn,     "rew", &err);
    UIUtilE::Assign(this, m_PlayBtn,    "play", &err);
    UIUtilE::Assign(this, m_PauseBtn,    "pause", &err);
    UIUtilE::Assign(this, m_StopBtn,    "stop", &err);
    UIUtilE::Assign(this, m_FfBtn,      "ff", &err);
    UIUtilE::Assign(this, m_NextBtn,    "next", &err);
    UIUtilE::Assign(this, m_CoverartImage, "coverart", &err);
    UIUtilE::Assign(this, m_Playlist,  "currentplaylist", &err);
    m_Playlist->SetEnabled(false);

    connect(m_PrevBtn, SIGNAL(Clicked()), this, SLOT(prev()));
    connect(m_NextBtn, SIGNAL(Clicked()), this, SLOT(next()));
    connect(m_PlayBtn, SIGNAL(Clicked()), this, SLOT(play()));
    m_PlayBtn->SetLockable(true);
    connect(m_PauseBtn, SIGNAL(Clicked()), this, SLOT(pause()));
    m_PauseBtn->SetLockable(true);
    connect(m_StopBtn, SIGNAL(Clicked()), this, SLOT(stop()));
    m_StopBtn->SetLockable(true);

    m_RatingState->DisplayState("0");
    m_CoverartImage->Reset();
    m_MuteState->DisplayState("off");
    // disable ff and rewind..
    if (m_FfBtn){
        m_FfBtn->SetEnabled(false);
        m_FfBtn->SetVisible(false);
    }
    if(m_RewBtn){
        m_RewBtn->SetEnabled(false);
        m_RewBtn->SetVisible(false);
    }

    BuildFocusList();

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

    m_VolumeBeforeMute = mpd_status_get_volume(m_Mpc);
    if (m_VolumeBeforeMute > 0)
        m_IsMute = false;

    updatePlaylist();

    switch(mpd_player_get_state(m_Mpc))
    {
        case MPD_PLAYER_PLAY:
            SetFocusWidget(m_PauseBtn);
            break;
        case MPD_PLAYER_PAUSE:
        case MPD_PLAYER_STOP:
            SetFocusWidget(m_PlayBtn);
            break;
        default:
            break;
    }
    LOG_("created...");
    return true;
}

bool Mpc::keyPressEvent(QKeyEvent *e)
{

    if (GetMythMainWindow()->IsExitingToMain())
        return false;

    QStringList actions;

    bool handledElsewhere = GetMythMainWindow()->TranslateKeyPress("Music", e, actions, true);
    if (handledElsewhere)
        return true;


    QStringList::const_iterator it;
    for (it = actions.constBegin(); it != actions.constEnd(); ++it) {

        LOG_TX(QString("kPE: ") + *it);
        if (*it == "VOLUMEUP") {
            volUp();
            return true;
        }

        if (*it == "VOLUMEDOWN") {
            volDown();
            return true;
        }

        if (*it == "MENU") {
            openConfigWindow();
            return true;
        }

        if (*it == "PLAY" || *it == "PAUSE") {
            togglePlay();
            return true;
        }

        if (*it == "MUTE") {
            toggleMute();
            return true;
        }

        if (*it == "PREVIOUS") {
            prev();
            return true;
        }

        if (*it == "NEXT") {
            next();
            return true;
        }
    }

    return MythScreenType::keyPressEvent(e);
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
            LOG_("play => pause");
            pause();
            break;
        case MPD_PLAYER_PAUSE:
            LOG_("pause => play");
            play();
            break;
        case MPD_PLAYER_STOP:
            LOG_("stopped => play");
            mpd_player_play(m_Mpc);
            break;
        default:
            break;
    }
    poll();
}

void Mpc::play(){
    LOG_TX("play");
    mpd_player_play(m_Mpc);
    poll();
}

void Mpc::pause(){
    LOG_TX("pause");
    mpd_player_pause(m_Mpc);
    poll();
}

void Mpc::next(){
    LOG_TX("next");
    mpd_player_next(m_Mpc);
    poll();
}

void Mpc::prev(){
    LOG_TX("prev");
    mpd_player_prev(m_Mpc);
    poll();
}

void Mpc::volUp(){
    changeVolume(+m_VolUpDownStep);
}

void Mpc::volDown(){
    changeVolume(-m_VolUpDownStep);
}

void Mpc::toggleMute(){
    if(m_IsMute){
        LOG_("Unmute");
        changeVolume(m_VolumeBeforeMute);
        setMute(false);
    }
    else {
        LOG_("Muting");
        m_VolumeBeforeMute = mpd_status_get_volume(m_Mpc);
        changeVolume(0);
        setMute(true);
    }
    showVolume();
}

void Mpc::setMute(bool isMute){
    m_IsMute = isMute;
    if (isMute)
        updateInfo("mutestate", "on");
    else
        updateInfo("mutestate", "off");
}

bool Mpc::isMuted(){
    return m_IsMute;
}

int Mpc::getVolume(){
    if(isMuted())
        return m_VolumeBeforeMute;

    return mpd_status_get_volume(m_Mpc);
}

void Mpc::changeVolume(int vol){
    int current = mpd_status_get_volume(m_Mpc);
    if (current < 0) return;

    int newVol = 0;
    if (vol != 0)
        newVol = current + vol;

    if (newVol > 100) newVol = 100;
    if (newVol < 0) newVol = 0;
    if (current == newVol)
        return;

    LOG_TX(QString("Volume %1%% => %2%%").arg(current).arg(newVol));
    mpd_status_set_volume(m_Mpc, newVol);
    setMute(false);
    poll();
    showVolume();
}

void Mpc::showVolume()
{
    if (m_VolumeDialog)
    {
        m_VolumeDialog->updateDisplay();
        return;
    }

    MythScreenStack *popupStack = GetMythMainWindow()->GetStack("popup stack");
    m_VolumeDialog = new MpcVolumeDialog(popupStack, "mpcvolumepopup", this);
    if (m_VolumeDialog->Create() == false)
    {
        delete m_VolumeDialog;
        return;
    }

    popupStack->AddScreen(m_VolumeDialog);
}

void Mpc::openConfigWindow(){
    MythScreenStack *st = GetMythMainWindow()->GetMainStack();
    if (!st) {
        LOG_("Could not get main stack.");
        return;
    }

    MpcConf* conf = new MpcConf(st);
    if (!conf->create()) {
        LOG_("Could not create MPC UI.");
        delete conf;
        conf = NULL;
        return;
    }

    st->AddScreen(conf);
}

QString Mpc::toMinSecString(int seconds){
    return QString("%1:%2")
        .arg(seconds/60, 2, 10, QChar('0'))
        .arg(seconds%60, 2, 10, QChar('0'));
}

#define MPD_VOLUME_POPUP_TIME 4 * 1000

MpcVolumeDialog::MpcVolumeDialog(MythScreenStack *parent, const char *name, Mpc *mpc)
: MythScreenType(parent, name, false),
    m_displayTimer(NULL), m_messageText(NULL), m_volText(NULL), m_muteState(NULL),
    m_volProgress(NULL), m_Mpc(mpc) {
}

MpcVolumeDialog::~MpcVolumeDialog(void) {
    if (m_displayTimer) {
        m_displayTimer->stop();
        delete m_displayTimer;
        m_displayTimer = NULL;
    }

    if (m_Mpc) {
        m_Mpc->m_VolumeDialog = NULL;
        m_Mpc = NULL;
    }
}

bool MpcVolumeDialog::Create(void)
{
    if (!LoadWindowFromXML("music-ui.xml", "volume_popup", this))
        return false;

    UIUtilW::Assign(this, m_volText,     "volume");
    UIUtilW::Assign(this, m_volProgress, "volumeprogress");
    UIUtilW::Assign(this, m_muteState,   "mutestate");

    if (m_volProgress)
        m_volProgress->SetTotal(100);

    updateDisplay();

    m_displayTimer = new QTimer(this);
    connect(m_displayTimer, SIGNAL(timeout()), this, SLOT(Close()));
    m_displayTimer->setSingleShot(true);
    m_displayTimer->start(MPD_VOLUME_POPUP_TIME);

    return true;
}

bool MpcVolumeDialog::keyPressEvent(QKeyEvent *event)
{
    QStringList actions;
    bool handledElsewhere = GetMythMainWindow()->TranslateKeyPress("Music", event, actions, false);
    if (handledElsewhere)
        return true;

    bool handled = false;
    for (int i = 0; i < actions.size() && !handled; i++) {
        QString action = actions[i];
        handled = true;

        if (action == "UP" || action == "VOLUMEUP")
            m_Mpc->volUp();
        else if (action == "DOWN" || action == "VOLUMEDOWN")
            m_Mpc->volDown();
        else if (action == "MUTE" || action == "SELECT")
            m_Mpc->toggleMute();
        else
            handled = false;
    }

    if (!handled && MythScreenType::keyPressEvent(event))
        handled = true;

    // Restart the display timer only if we handled this keypress, if nothing
    // has changed there's no need to keep the volume on-screen
    if (handled)
        m_displayTimer->start(MPD_VOLUME_POPUP_TIME);

    return handled;
}

void MpcVolumeDialog::updateDisplay() {
    if (m_muteState)
        m_muteState->DisplayState(m_Mpc->isMuted() ? "on" : "off");

    if (m_volProgress)
        m_volProgress->SetUsed(m_Mpc->getVolume());

    if (m_volText)
        m_volText->SetTextFromMap(m_Mpc->stateInfo);
}

