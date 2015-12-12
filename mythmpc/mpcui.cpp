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
    m_MasterBackend("localhost"),
    m_MasterBackendPort(6544),
    m_SqlQuery(MSqlQuery::InitCon()),
    m_SelectCurrent(true),

    //ui
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
    m_MuteState(NULL),
    m_RatingState(NULL),
    m_VolumeText(NULL),
    m_CoverartImage(NULL),
    m_VolumeDialog(NULL),
    m_Playlist(NULL),
    m_Visualizerborder(NULL)
{
}

Mpc::~Mpc(){
    if (!m_Mpc)
        return;

    mpd_connection_free(m_Mpc);
    m_Mpc = NULL;
    m_PlayBtn->Reset();
}

void Mpc::createOrUpdateQueueItem(int pos, InfoMap meta){
    MythUIButtonListItem* item = m_Playlist->GetItemAt(pos);
    QString m = QString("Btn %1").arg (pos);
    if (item){
        m += QString(" updating %1 to %2 ")
            .arg(item->GetData().value<InfoMap>()["title"])
            .arg(meta["title"]);
        item->SetData(qVariantFromValue(meta));
    }
    else{
        item = new MythUIButtonListItem(m_Playlist, " ", qVariantFromValue(meta),pos);
        m += QString(" created for %1").arg(meta["title"]);
    }
    item->SetTextFromMap(meta);

    item->SetText(meta["title"] + meta["artist"] + meta["album"] , "**search**");
    item->SetImage("");
    item->SetImage("","coverart");
    LOG_(m);
}

void Mpc::songMetaToInfoMap(mpd_song* s, InfoMap* m){
    unsigned pos = mpd_song_get_pos(s);
    m->insert("uri",QString(mpd_song_get_uri(s)));
    m->insert("pos"  , QString("%1").arg(pos));
    m->insert("artist", getTag(MPD_TAG_ARTIST, s));
    m->insert("name" , getTag( MPD_TAG_NAME,  s));
    m->insert("title", getTag( MPD_TAG_TITLE, s));
    m->insert("album", getTag( MPD_TAG_ALBUM, s));
    m->insert("track", getTag( MPD_TAG_TRACK, s));
    m->insert("date" , getTag( MPD_TAG_DATE,  s));
    m->insert("genre",getTag(MPD_TAG_GENRE,s));

    if (m->value("artist").isEmpty()){
        m->insert("artist", m->value("name"));
    }
}

void Mpc::updatePlaylistSongStates(){
    for (int i = 0; i < m_Playlist->GetCount(); i++) {
        MythUIButtonListItem *item = m_Playlist->GetItemAt(i);
        item->SetFontState("normal");
        item->DisplayState("default", "playstate");
        InfoMap mdata = item->GetData().value<InfoMap>();
        unsigned song_pos = mdata["pos"].toInt();
        if (m_CurrentSongPos == song_pos) {

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

        }
    }
}

void Mpc::selectCurrentSong(){
    LOG_(QString("Select song at position %1").arg(m_CurrentSongPos));
    MythUIButtonListItem *item = m_Playlist->GetItemAt(m_CurrentSongPos);
    if (item)
        m_Playlist->SetItemCurrent(item);
}


void Mpc::updateInfo(QString key, QString value){
    if (stateInfo[key] == value)
        return;

    //  if (key != "song_elapsed" && key != "progress")
    //      LOG_(key + ": '" + stateInfo[key] + "' => '" + value + "'");

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

    if (key.startsWith("next") && m_InfoText){
        m_InfoText->SetTextFromMap(stateInfo);
        return;
    }

    if (key == "artist" && m_ArtistText){
        m_ArtistText->SetTextFromMap(stateInfo);
        return;
    }

    if(key == "trackstate"){
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
        stateInfo["volume"] = stateInfo["volumepercent"];
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

    if (key == "rating"){
        m_RatingState->DisplayState(value);
    }

}


bool Mpc::create(void){
    m_MasterBackend     = gCoreContext->GetMasterServerIP();
    m_MasterBackendPort = gCoreContext->GetMasterServerStatusPort();
    LOG_(QString("master: %1:%2 ").arg(m_MasterBackend).arg(m_MasterBackendPort));

    m_SqlQuery = MSqlQuery(MSqlQuery::InitCon());
    m_SqlQuery.prepare("SELECT song_id,rating from `music_songs` WHERE `filename` = :FILENAME AND `name` = :NAME");


    bool foundtheme = false;
    foundtheme = LoadWindowFromXML("mpcui.xml", "mpc", this);
    if (!foundtheme) {
        LOG_("window mpc in mpcui.xml is missing."); 
        return  false;
    }

    bool err = false;

    UIUtilE::Assign(this, m_TitleText, "title");
    UIUtilE::Assign(this, m_TimeText, "time", &err);
    UIUtilW::Assign(this, m_InfoText, "nexttitle");
    UIUtilE::Assign(this, m_ArtistText, "artist");
    UIUtilE::Assign(this, m_RatingState, "ratingstate", &err);
    UIUtilE::Assign(this, m_TrackProgress, "progress", &err);
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
    UIUtilW::Assign(this, m_Visualizerborder, "visualizer_border", &err);
    if (m_Visualizerborder){
        m_Visualizerborder->SetVisible(false);
    }

    connect(m_Playlist, SIGNAL(itemClicked(MythUIButtonListItem *)),
            this, SLOT(onQueueItemClicked(MythUIButtonListItem *)));
    connect(m_Playlist, SIGNAL(itemVisible(MythUIButtonListItem*)),
            this, SLOT(onQueueItemVisible(MythUIButtonListItem*)));
    m_Playlist->SetSearchFields("**search**");


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
    connectToMpd();

    LOG_("created...");
    return true;
}

void Mpc::onQueueItemVisible(MythUIButtonListItem* item){
    if (!m_Mpc) {
        LOG_("Not connected to MPD");
        return;
    }
    InfoMap mdata = item->GetData().value<InfoMap>();
    mythmusicMetaToInfoMap(mdata["uri"],  mdata["title"], &mdata);
    item->SetData(qVariantFromValue(mdata));

    item->SetTextFromMap(mdata);
    QString art_url = mdata["coverart"];
    if (   art_url.isEmpty() == false
            && item->GetImageFilename("coverart").isEmpty()) {
        item->SetImage(art_url);
        item->SetImage(art_url, "coverart");
    }

    item->DisplayState(mdata["rating"], "ratingstate");

    //    LOG_(QString("%1 - %2 - %3 - %4")
    //            .arg(mdata["pos"],2)
    //            .arg(mdata["artist"])
    //            .arg(mdata["title"])
    //            .arg(mdata["coverart"])
    //        );
}

void Mpc::onQueueItemClicked(MythUIButtonListItem* item){
    if (!m_Mpc) {
        LOG_("Not connected to MPD");
        return;
    }
    InfoMap mdata = item->GetData().value<InfoMap>();
    unsigned song_pos = mdata["pos"].toInt();
    mpd_run_play_pos(m_Mpc,  song_pos);
    poll();
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

        if (*it == "STOP" ){
            stop();
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
    if (!m_Mpc) {
        LOG_("Not connected to MPD");
        return;
    }
    QElapsedTimer t;
    t.start();
    m_PollTimer->stop();
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

        int total = mpd_status_get_total_time(s);
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
            LOG_(QString("queue version changed %1 => %2")
                    .arg(m_knownQueueVersion)
                    .arg(currentQueueVers));

            mpd_send_queue_changes_meta(m_Mpc, m_knownQueueVersion);
            m_knownQueueVersion = currentQueueVers;

            unsigned newSize = mpd_status_get_queue_length(s);
            unsigned currentSize = m_Playlist->GetCount();
            for (unsigned i= currentSize; i> newSize; i--){
                MythUIButtonListItem *item = m_Playlist->GetItemAt(i-1);
                m_Playlist->RemoveItem(item);
            }

            mpd_song *song;
            while ((song = mpd_recv_song(m_Mpc)) != NULL) {
                InfoMap d;
                songMetaToInfoMap(song, &d);
                LOG_(QString("%1 is now %2").arg(d["pos"]).arg(d["title"]));
                createOrUpdateQueueItem(d["pos"].toInt(), d);
                mpd_song_free(song);
            }

        }

        mpd_status_free(s);
    }

    struct mpd_song *song;
    song = mpd_run_current_song(m_Mpc);
    if (song != NULL) {
        InfoMap m;
        songMetaToInfoMap(song, &m); // Always have to read title to update radio stream titles

        unsigned pos = mpd_song_get_pos(song);
        if(pos != m_CurrentSongPos){
            m_CurrentSongPos = pos;
            mythmusicMetaToInfoMap(m["uri"],  m["title"], &m);

            m_CoverartImage->Reset();
            QString url = m["coverart"];
            if (!url.isEmpty()){
                m_CoverartImage->SetFilename(url);
                m_CoverartImage->Load();
            }
            updatePlaylistSongStates();
            MythUIButtonListItem *item = m_Playlist->GetItemAt(m_CurrentSongPos +1);
            if (item) {
                InfoMap next = item->GetData().value<InfoMap>();
                updateInfo("nexttitle", next["title"]);
                updateInfo("nextartist", next["artist"]);
            }
        }

        QHash<QString,QString>::iterator i;
        for (i = m.begin(); i != m.end(); i++)
            updateInfo(i.key(), i.value());

        mpd_song_free(song);

        if (m_SelectCurrent)
        {
            m_SelectCurrent = false;
            selectCurrentSong();
        }
    }


    int elapsed = t.elapsed();
    if (elapsed > 5)
        LOG_(QString("Poll took %1").arg(elapsed));
    m_PollTimer->start(m_PollTimeout);
}

QString Mpc::getTag(mpd_tag_type tag, mpd_song* song){
    return QString("%1").arg(mpd_song_get_tag(song, tag, 0));
}

void Mpc::mythmusicMetaToInfoMap(QString uri, QString title, InfoMap *m){
    if(m->value("coverart").isEmpty() == false
            && m->value("").isEmpty() == false)
        return;

    int idx = uri.lastIndexOf("/");
    QString fname;
    if (idx <= 0)
        fname = uri;
    else
        fname = uri.mid(idx+1,uri.length());

    m_SqlQuery.bindValue(":FILENAME", fname );
    m_SqlQuery.bindValue(":NAME", title);

    if (!m_SqlQuery.exec()) {
        LOG_("Could not read songinfo from DB.");
        return;
    }

    while(m_SqlQuery.next()) {
        int songId = m_SqlQuery.value(0).toInt();
        QString rating = m_SqlQuery.value(1).toString();
        QString url =  QString("http://%1:%2/Content/GetAlbumArt?Id=%3")
            .arg(m_MasterBackend)
            .arg(m_MasterBackendPort)
            .arg(songId);
        //    LOG_(QString("rating %1 url %2 ").arg(rating).arg(url));
        m->insert("coverart",url);
        m->insert("rating",rating);
        return;
    }
}

void Mpc::stop(){
    LOG_("stop");
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
    LOG_("play");
    if (!m_Mpc) {
        LOG_("Not connected to MPD");
        return;
    }
    mpd_run_play(m_Mpc);
    poll();
}

void Mpc::pause(){
    LOG_("pause");
    if (!m_Mpc) {
        LOG_("Not connected to MPD");
        return;
    }
    mpd_run_pause(m_Mpc, true); // no clue what mode exactly should be..
    poll();
}

void Mpc::next(){
    LOG_("next");
    if (!m_Mpc) {
        LOG_("Not connected to MPD");
        return;
    }
    mpd_run_next(m_Mpc);
    poll();
}

void Mpc::prev(){
    LOG_("prev");
    if (!m_Mpc) {
        LOG_("Not connected to MPD");
        return;
    }
    mpd_run_previous(m_Mpc);
    poll();
}

void Mpc::volUp(){
    if (m_IsMute)
        toggleMute();
    changeVolume(m_Volume + m_VolUpDownStep);
}

void Mpc::volDown(){
    if (m_IsMute)
        toggleMute();
    changeVolume(m_Volume - m_VolUpDownStep);
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
    if (!m_Mpc) {
        LOG_("Not connected to MPD");
        return;
    }
    if (vol > 100) vol = 100;
    if (vol < 0) vol = 0;
    if (m_Volume == vol)
        return;

    LOG_(QString("Volume %1%% => %2%%").arg(m_Volume).arg(vol));
    mpd_run_set_volume(m_Mpc, vol);
    updateInfo("volumepercent", QString("%1").arg(vol));
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

#define MPD_VOLUME_POPUP_TIME 2 * 1000

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
