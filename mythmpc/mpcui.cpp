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
#include "mpcui.h"
#include "mpcconf.h"


Mpc::Mpc(MythScreenStack *parent): MythScreenType(parent, "mpc"),
    m_Mpc(NULL),
    m_PollTimer(NULL),
    m_PollTimeout(50),
    m_VolUpDownStep(1),
    m_VolumeBeforeMute(-5),
    m_Volume(-1),
    m_IsMute(false),
    m_CurrentSong(NULL),
    m_CurrentSongPos(-1),
    m_knownQueueVersion(0),
    m_StopBtn(NULL),
    m_PlayBtn(NULL),
    m_PauseBtn(NULL),
    m_NextBtn(NULL),
    m_PrevBtn(NULL),
    m_RewBtn(NULL),
    m_FfBtn(NULL),
    m_ConfigBtn(NULL),
    m_TitleText(NULL),
    m_TimeText(NULL),
    m_InfoText(NULL),
    m_TrackProgress(NULL),
    m_TrackProgressText(NULL),
    m_TrackState(NULL),
    m_MuteState(NULL),
    m_RatingState(NULL),
    m_VolumeText(NULL),
    m_CoverartImage(NULL),
    m_VolumeDialog(NULL),
    m_Playlist(NULL)
{
}

Mpc::~Mpc(){
    if (!m_Mpc)
        return;

    mpd_connection_free(m_Mpc);
    m_Mpc = NULL;
}

void Mpc::updatePlaylist(){
    int pivot = m_CurrentSongPos;
    int cntElementsBeforAndAfter= 20;
    int start = pivot - cntElementsBeforAndAfter;
    if (start < 0)
        start = 0;
    int  stop = pivot +cntElementsBeforAndAfter ;

    if(!mpd_send_list_queue_range_meta(m_Mpc, start, stop))
        return;

    m_Playlist->Reset();
    mpd_song *song;
    while ((song = mpd_recv_song(m_Mpc)) != NULL) {
        InfoMap mdata;
        unsigned pos = mpd_song_get_pos(song);
        mdata["uri"]    = mpd_song_get_uri(song);
        mdata["pos"]    = pos;;
        mdata["artist"] = getTag(MPD_TAG_ARTIST, song);
        mdata["name"]   = getTag( MPD_TAG_NAME,  song);
        mdata["title"]  = getTag( MPD_TAG_TITLE, song);
        mdata["album"]  = getTag( MPD_TAG_ALBUM, song);
        mdata["track"]  = getTag( MPD_TAG_TRACK, song);
        mdata["date"]   = getTag( MPD_TAG_DATE,  song);

        MythUIButtonListItem *item;
        item = new MythUIButtonListItem(m_Playlist, " ", qVariantFromValue(pos));
        item->SetTextFromMap(mdata);
        mpd_song_free(song);
    }
    updatePlaylistSongStates();
}

void Mpc::updatePlaylistSongStates(){
    for (int i = 0; i < m_Playlist->GetCount(); i++) {
        MythUIButtonListItem *item = m_Playlist->GetItemAt(i);
        item->SetFontState("normal");
        item->DisplayState("default", "playstate");
        if (m_CurrentSongPos > 0 && m_CurrentSongPos == item->GetData()) {

            if (isPlaying()) {
                item->SetFontState("running");
                item->DisplayState("playing", "playstate");
            }
            else if (isPaused()) {
                item->SetFontState("idle");
                item->DisplayState("paused", "playstate");
            }
            else {
                item->SetFontState("normal");
                item->DisplayState("stopped", "playstate");
            }

            m_Playlist->SetItemCurrent(item);
        }
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
        LOG_("Setting lockstate...");
        if (value == "playing"){
            m_PlayBtn->SetLocked(true);
            m_PauseBtn->SetLocked(false);
            m_StopBtn->SetLocked(false);
        }
        else if (value == "stopped"){
            m_PlayBtn->SetLocked(false);
            m_PauseBtn->SetLocked(false);
            m_StopBtn->SetLocked(true);
        }
        else if (value == "paused"){
            m_PlayBtn->SetLocked(false);
            m_PauseBtn->SetLocked(true);
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


bool Mpc::create(void){

    bool foundtheme = false;
    foundtheme = LoadWindowFromXML("mpcui.xml", "mpc", this);
    if (!foundtheme) {
        LOG_("window mpc in mpcui.xml is missing."); 
        return  false;
    }

    bool err = false;

    UIUtilE::Assign(this, m_TitleText, "title");
    UIUtilE::Assign(this, m_TimeText, "time", &err);
    UIUtilE::Assign(this, m_InfoText, "info");
    UIUtilE::Assign(this, m_ArtistText, "artist");
    UIUtilE::Assign(this, m_RatingState, "ratingstate", &err);
    UIUtilE::Assign(this, m_TrackProgress, "progress", &err);
    UIUtilE::Assign(this, m_TrackProgressText, "trackprogress");
    UIUtilE::Assign(this, m_TrackState, "trackstate", &err);
    UIUtilE::Assign(this, m_VolumeText, "volume", &err);
    UIUtilE::Assign(this, m_MuteState, "mutestate", &err);
    UIUtilE::Assign(this, m_PrevBtn, "prev", &err);
    UIUtilE::Assign(this, m_RewBtn, "rew", &err);
    UIUtilE::Assign(this, m_PlayBtn, "play", &err);
    UIUtilE::Assign(this, m_PauseBtn, "pause", &err);
    UIUtilE::Assign(this, m_StopBtn, "stop", &err);
    UIUtilE::Assign(this, m_FfBtn, "ff", &err);
    UIUtilE::Assign(this, m_NextBtn, "next", &err);
    UIUtilE::Assign(this, m_CoverartImage, "coverart", &err);
    UIUtilE::Assign(this, m_Playlist, "currentplaylist", &err);
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

    m_PollTimer = new QTimer(this);
    connect(m_PollTimer, SIGNAL(timeout()), this, SLOT(poll()));

    if (connectToMpd())
        updatePlaylist();

    LOG_("created...");
    return true;
}

bool Mpc::connectToMpd(){
    if(m_PollTimer)
        m_PollTimer->stop();

    if(m_Mpc){
        mpd_connection_free(m_Mpc);
        m_Mpc = NULL;
    }

    QString host = gCoreContext->GetSetting("mpd-host", "localhost");
    unsigned port = gCoreContext->GetSetting("mpd-port", "6600").toInt();
    QString pass = gCoreContext->GetSetting("mpd-pass", "");

    if (pass.isEmpty() == false)
        host = pass + "@" + host;

    m_Mpc = mpd_connection_new((char*) host.toLatin1().data(), port, 30000);
    if (!m_Mpc)
    {
        LOG_(QString("Could not create connection to MPD"));
        return false;
    }

    if (mpd_connection_get_error(m_Mpc) != MPD_ERROR_SUCCESS) {
        LOG_(QString("%1").arg(mpd_connection_get_error_message(m_Mpc)));
        mpd_connection_free(m_Mpc);
        m_Mpc = NULL;
        return false;
    }

    if(m_PollTimer)
        m_PollTimer->start(m_PollTimeout);

    return true;
}

bool Mpc::keyPressEvent(QKeyEvent *e)
{
    if (GetMythMainWindow()->IsExitingToMain())
        return false;

    QStringList actions;

    bool handledElsewhere = GetMythMainWindow()->TranslateKeyPress("Music", e, actions, true);
    if (handledElsewhere){
        LOG_(QString("Already handledElsewhere"));
        return true;
    }

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
    m_PollTimer->stop();
    bool reloadPlaylist = false;
    struct mpd_status *s;

    s = mpd_run_status(m_Mpc);

    if(s){
        int vol = mpd_status_get_volume(s);
        m_Volume = vol;
        updateInfo("volumepercent", QString("%1").arg(vol));

        switch (mpd_status_get_state(s)) {
            case MPD_STATE_PLAY:
                updateInfo("trackstate", "playing");
                break;
            case MPD_STATE_PAUSE:
                updateInfo("trackstate", "paused");
                break;
            case MPD_STATE_STOP:
            default:
                updateInfo("trackstate", "stopped");
                break;
        }

        updateInfo("song_pos", QString("%1").arg(mpd_status_get_song_pos(s)));

        int elapsed = mpd_status_get_elapsed_time(s);
        updateInfo("song_elapsed", toMinSecString(elapsed));

        int total =mpd_status_get_total_time(s);
        updateInfo("song_total", toMinSecString(total));
        int progress = 0;
        if (total <= 0)
            progress = 33;
        else if (elapsed > total)
            progress = 100;
        else if (elapsed <= 0)
            progress = 0;
        else
            progress = ((float)elapsed/(float)total)*100;

        updateInfo("progress", QString("%1").arg(progress));

        unsigned currentQueueVers = mpd_status_get_queue_version(s);

        if (m_knownQueueVersion != currentQueueVers){
            reloadPlaylist = true;
            m_knownQueueVersion = currentQueueVers;
        }

        mpd_status_free(s);
    }

    struct mpd_song *song;
    song = mpd_run_current_song(m_Mpc);
    if (song != NULL) {
        updateInfo("artist", getTag(MPD_TAG_ARTIST, song));
        updateInfo("album", getTag( MPD_TAG_ALBUM, song));
        updateInfo("title", getTag( MPD_TAG_TITLE, song));
        updateInfo("track", getTag( MPD_TAG_TRACK, song));
        updateInfo("name", getTag( MPD_TAG_NAME, song));
        updateInfo("date", getTag( MPD_TAG_DATE, song));
        unsigned pos = mpd_song_get_pos(song);
        if(pos != m_CurrentSongPos){
            reloadPlaylist = true;
            m_CurrentSongPos = pos;
        }
        mpd_song_free(song);
    }

    if (reloadPlaylist)
        updatePlaylist();

    m_PollTimer->start(m_PollTimeout);
}

QString Mpc::getTag(mpd_tag_type tag, mpd_song* song){
    return QString("%1").arg(mpd_song_get_tag(song, tag, 0));
}

void Mpc::stop(){
    LOG_TX("stop");
    mpd_run_stop(m_Mpc);
    poll();
}

void Mpc::togglePlay(){
    QString ts = stateInfo["trackstate"];

    if ( ts == "playing") {
        LOG_("play => pause");
        pause();
    }
    else if ( ts == "paused") {
        LOG_("pause => play");
        play();
    }
    else if ( ts == "stopped") {

        LOG_("stopped => play");
        play();
    }
    else {
        LOG_("unknown => play");
        play();
    }

    poll();
}

void Mpc::play(){
    LOG_TX("play");
    mpd_run_play(m_Mpc);
    poll();
}

void Mpc::pause(){
    LOG_TX("pause");
    mpd_run_pause(m_Mpc, true); // no clue what mode exactly should be..
    poll();
}

void Mpc::next(){
    LOG_TX("next");
    mpd_run_next(m_Mpc);
    poll();
}

void Mpc::prev(){
    LOG_TX("prev");
    mpd_run_previous(m_Mpc);
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
        m_VolumeBeforeMute = m_Volume;
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

    return m_Volume;
}

void Mpc::changeVolume(int vol){
    int current = getVolume();
    if (current < 0) return;

    int newVol = 0;
    if (vol != 0)
        newVol = current + vol;

    if (newVol > 100) newVol = 100;
    if (newVol < 0) newVol = 0;
    if (current == newVol)
        return;

    LOG_TX(QString("Volume %1%% => %2%%").arg(current).arg(newVol));
    mpd_run_set_volume(m_Mpc, newVol);
    updateInfo("volumepercent", QString("%1").arg(newVol));
    setMute(false);
    poll();
    showVolume();
}

void Mpc::showVolume() {
    if (m_VolumeDialog) {
        m_VolumeDialog->updateDisplay();
        return;
    }

    MythScreenStack *popupStack = GetMythMainWindow()->GetStack("popup stack");
    m_VolumeDialog = new MpcVolumeDialog(popupStack, "mpcvolumepopup", this);
    if (m_VolumeDialog->Create() == false) {
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

    MpcConf* conf = new MpcConf(st, this);
    if (!conf->create()) {
        LOG_("Could not create MPC UI.");
        delete conf;
        conf = NULL;
        return;
    }

    st->AddScreen(conf);
}

QString Mpc::toMinSecString(int seconds){
    if (seconds <= 0)
        return "";

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

bool MpcVolumeDialog::Create(void) {

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

bool MpcVolumeDialog::keyPressEvent(QKeyEvent *event) {

    QStringList actions;
    bool handledElsewhere = GetMythMainWindow()->TranslateKeyPress("Music", event, actions, false);
    if (handledElsewhere)
        return true;

    bool handled = false;
    for (int i = 0; i < actions.size() && !handled; i++) {
        QString action = actions[i];

        if (action == "VOLUMEUP" || action == "UP" || action == "RIGHT"){
            m_Mpc->volUp();
            handled = true;
        }

        if (action == "VOLUMEDOWN" || action == "DOWN" || action == "LEFT"){
            m_Mpc->volDown();
            handled = true;
        }

        if (action == "MUTE" || action == "SELECT"){
            m_Mpc->toggleMute();
            handled = true;
        }
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
