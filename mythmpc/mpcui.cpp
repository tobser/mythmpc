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

Mpc::Mpc(MythScreenStack *parent):
    MythScreenType(parent, "mpc"),
    m_Stop(NULL),
    m_PlayPause(NULL),
    m_Next(NULL),
    m_Prev(NULL),
    m_InfoText(NULL),
    m_TitleText(NULL),
    m_ButtonList(NULL),
    m_MpcSocket(NULL),
    m_PingTimer(NULL),
    m_PingTimeout(45000),
    m_VolUp(NULL),
    m_VolDown(NULL)
{
}

bool Mpc::create(void)
{
    // Load the theme for this screen
    bool foundtheme = false;
    foundtheme = LoadWindowFromXML("mpcui.xml", "mpc", this);
    if (!foundtheme)
    {
        LOG_M(LOG_WARNING, "window mpc in mpcui.xml is missing."); 
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
    if (err)
    {
        LOG_M(LOG_WARNING, "Theme is missing required elements.");
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

    BuildFocusList();
    SetFocusWidget(m_PlayPause);

    m_MpcSocket = new QTcpSocket(this);
    connect(m_MpcSocket, SIGNAL(readyRead()), this, SLOT(readFromMpd()));
    connect(m_MpcSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayMpdConnError(QAbstractSocket::SocketError)));

    QString host =
        gCoreContext->GetSetting("mpd-host", "localhost");
    int port =
        gCoreContext->GetSetting("mpd-port", "6600").toInt();
    m_MpcSocket->connectToHost(host, port);

    m_PingTimer = new QTimer(this);
    connect(m_PingTimer, SIGNAL(timeout()), this, SLOT(sendPing()));
    m_PingTimer->start(m_PingTimeout);

    poll();
    return true;
}

void Mpc::poll(){
    sendCommand("currentsong");
    sendCommand("status");
}
void Mpc::sendPing(){
    sendCommand("ping");
}
void Mpc::sendCommand(QString cmd){
    LOG_M(LOG_WARNING, "sending '" + cmd + "'");
    cmd = cmd + "\n";
    m_MpcSocket->write(cmd.toLatin1(), (quint16) cmd.length());
    m_MpcSocket->flush();
    m_PingTimer->start(m_PingTimeout);
}
void Mpc::readFromMpd()
{
    while(m_MpcSocket->canReadLine())
    {
        QString incomingData = m_MpcSocket->readLine();
        incomingData = incomingData.trimmed();
        LOG_M(LOG_WARNING, incomingData);
        if (incomingData.startsWith("Title: ")){
            QString t = incomingData.mid(7,incomingData.length());
            LOG_M(LOG_WARNING, "setting titel to "+ t);
            m_TitleText->SetText(t);
        }
    }
}
void Mpc::displayMpdConnError(QAbstractSocket::SocketError socketError){
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        LOG_M(LOG_WARNING, "Remote Host closed");
        break;
    case QAbstractSocket::HostNotFoundError:
       LOG_M(LOG_WARNING, tr("The host was not found. Please check the "
                                    "host name and port settings."));
        break;
    case QAbstractSocket::ConnectionRefusedError:
       LOG_M(LOG_WARNING, tr("The connection was refused by the peer. "
                                    "Make sure the fortune server is running, "
                                    "and check that the host name and port "
                                    "settings are correct."));
        break;
    default:
        LOG_M(LOG_WARNING, tr("The following error occurred: %1.")
                                 .arg(m_MpcSocket->errorString()));
    }
}

void Mpc::newClicked(void)
{
    openEditScreen();
}

void Mpc::itemClicked(MythUIButtonListItem * item)
{
    //TimerData val = qVariantValue<TimerData>(item->GetData());
}
