/* This file is part of GBerry.
 *
 * Copyright 2015 Tero Vuorela
 *
 * GBerry is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * GBerry is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with GBerry. If not, see <http://www.gnu.org/licenses/>.
 */
 
 #include "connectionmanager.h"

#include "commtcpserver.h"
#include "serverchannelmanager.h"
#include "serversidecontrolchannel.h"
#include "connectiongatekeeper.h"
#include "applicationregistry.h"

#define LOG_AREA "ConnectionManager"
#include "log/log.h"

using namespace GBerry::Console::Server;

ConnectionManager::ConnectionManager(CommTcpServer* tcpServer,
                                     ApplicationRegistry* appRegistry,
                                     ServerChannelManager* channelManager,
                                     QObject *parent) :
    QObject(parent),
    _tcpServer(tcpServer),
    _applicationRegistry(appRegistry),
    _channelManager(channelManager)
{
}

ConnectionManager::~ConnectionManager()
{
}

bool ConnectionManager::activeConnection()
{
    return _channelManager->hasActiveConnection();
}


void ConnectionManager::applicationConnected(int connectionId)
{
    DEBUG("222 New application connection: connectionId=" << connectionId);

    ConnectionGateKeeper* gatekeeper = new ConnectionGateKeeper(connectionId, _applicationRegistry, this);
    connect(gatekeeper, &ConnectionGateKeeper::connectionValidated, this, &ConnectionManager::onConnectionValidationOK);
    connect(gatekeeper, &ConnectionGateKeeper::connectionDiscarded, this, &ConnectionManager::onConnectionValidationFailed);
    connect(gatekeeper, &ConnectionGateKeeper::outgoingMessage, this, &ConnectionManager::onOutgoingMessageFromGateKeeper);

    _gatekeepers[connectionId] = gatekeeper;
    gatekeeper->validate();
}

void ConnectionManager::applicationDisconnected(int connectionId)
{
    DEBUG("Application disconnected: connectionId=" << connectionId);

    _tcpServer->closeConnection(connectionId);
    _channelManager->applicationClosed(connectionId);
    _applicationRegistry->removeLink(connectionId);
}

void ConnectionManager::incomingMessage(int connectionId, int channelId, const QByteArray msg)
{
    DEBUG("incomingMessage(): cid=" << channelId);
    if (_gatekeepers.contains(connectionId)) {
        _gatekeepers[connectionId]->incomingMessage(msg);
    } else {
        _channelManager->processMessageFromSouth(connectionId, channelId, msg);
    }
}

void ConnectionManager::outgoingMessageFromChannel(int connectionId, int channelId, const QByteArray msg)
{
    _tcpServer->write(connectionId, channelId, msg);
}

void ConnectionManager::onOutgoingMessageFromGateKeeper(int connectionId, const QByteArray &msg)
{
    _tcpServer->write(connectionId, ServerSideControlChannel::CHANNEL_ID, msg);
}

void ConnectionManager::onConnectionValidationOK(int connectionId)
{
    DEBUG("Connection validation OK: connectionId=" << connectionId);

    if (!_gatekeepers.contains(connectionId)) {
        ERROR("Got signal from ConnectionGatekeeper that is not registered. Ignoring it.");
        return;
    }
    ConnectionGateKeeper* gatekeeper = _gatekeepers[connectionId];
    _gatekeepers.remove(connectionId);

    _applicationRegistry->createLink(connectionId, gatekeeper->validatedApplicationId());

    _channelManager->openControlChannel(connectionId);

    // latest will always be active
    _channelManager->activateConnection(connectionId);

    // TODO: there is state between app deactivate and active -> players should have waiting state

    // if no applicationId then empty string

    emit applicationConnectionValidated(gatekeeper->validatedApplicationId());

    // guard is no longer needed
    gatekeeper->deleteLater();
}


void ConnectionManager::onConnectionValidationFailed(int connectionId)
{
    DEBUG("Connection validation failed: connectionId=" << connectionId);

    if (!_gatekeepers.contains(connectionId)) {
        ERROR("Got signal from ConnectionGatekeeper that is not registered. Ignoring it.");
        return;
    }
    ConnectionGateKeeper* gatekeeper = _gatekeepers[connectionId];
    _gatekeepers.remove(connectionId);

    _tcpServer->closeConnection(connectionId); // note that we will get disconnect signal from this

    // no need to close ChannelManagers as we haven't opened them yet

    gatekeeper->deleteLater();
}
