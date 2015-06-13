#include "clientsidecontrolchannel.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

#include "clientsidechannelpartners.h"
#include "messagefactory.h"
using namespace GBerry::Console;

#define LOG_AREA "ClientSideControlChannel"
#include "log/log.h"


const int ClientSideControlChannel::CHANNEL_ID(0);

ClientSideControlChannel::ClientSideControlChannel() :
    _id(0),
    _partner(nullptr)
{
}

ClientSideControlChannel::~ClientSideControlChannel()
{
    TRACE("~ClientSideControlChannel");
}

void ClientSideControlChannel::requestApplicationLaunch(QString appID)
{
    if (!_partner) return;

    QJsonObject json;
    json["command"] = "LaunchApplication";
    json["app_id"] = appID;
    QJsonDocument jsonDoc(json);
    _partner->sendMessage(jsonDoc.toJson());
}

void ClientSideControlChannel::requestApplicationExit()
{
    if (!_partner) return;

    QJsonObject json;
    json["command"] = "ExitApplication";
    QJsonDocument jsonDoc(json);
    _partner->sendMessage(jsonDoc.toJson());
}

void ClientSideControlChannel::setApplicationIdCode(const QString& code)
{
    _applicationIdCode = code;
}


void ClientSideControlChannel::ping()
{
    if (!_partner) return;
    _partner->sendMessage(MessageFactory::createPingCommand(_applicationIdCode));
}

bool ClientSideControlChannel::receiveMessage(const QByteArray& msg)
{
    QJsonParseError errors;

    QJsonDocument doc(QJsonDocument::fromJson(msg, &errors));
    if (errors.error != QJsonParseError::NoError) {
        WARN("Failed to parse json message: error =" << errors.errorString() << ", offset =" << errors.offset << ", message =" << msg);
        return false;
    }

    QJsonObject json(doc.object());

    if (json.contains("command") && json["command"] == "Ping")
    {
        emit pingReceived();
        // TODO: we should put our code here
        if (_partner) _partner->sendMessage(MessageFactory::createPingReply(_applicationIdCode));
        return true;
    }
    else if (json.contains("command") && json["command"] == "PingReply")
    {
        emit pingReceived();
        // but no reply to reply ...
        return true;
    }

    // not known by us
    return false;
}


void ClientSideControlChannel::attachChannelPartner(ClientSideChannelPartner* partner)
{
    _partner = partner;
}

void ClientSideControlChannel::detachChannelPartner()
{
    _partner = NULL;
}

int ClientSideControlChannel::channelId()
{
    return _id;
}
