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
 
#include <testutils/qtgtest.h>
#include <testutils/waiter.h>

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "commands/querydownloadableapplicationscommand.h"
#include "headserverconnection.h"
#include "invocationfactoryimpl.h"
#include "downloadableapplicationcache.h"
#include <server/serversidecontrolchannel.h>
#include <server/playersessionmanager.h>
#include "realsystemservices.h"

#define LOG_AREA "IntegrationQueryDownloadableApplicationsCommand"
#include "log/log.h"

#include "testobjects/stub_channelsouthpartner.h"

using namespace GBerry;
using namespace GBerryConsole::Test;


// this is integration test with real local server.
TEST(IntegrationQueryDownloadableApplicationsCommand, OK)
{
    RealSystemServices systemServices;
    systemServices.registerItself();

    DownloadableApplicationCache applicationCache; // so simple that no mock need ...

    InvocationFactoryImpl invocationFactory;
    invocationFactory.setProperty(InvocationFactory::URL_PREFIX_PROP, "http://localhost/gberryrest/v1");

    HeadServerConnection headServerConnection(&invocationFactory); // TODO: some interface for this

    StubChannelSouthPartner* stubChannelSouthPartner = new StubChannelSouthPartner; // channel takes ownership

    ServerSideControlChannel controlChannel; // TODO: some interface for message sending
    controlChannel.attachSouthPartner(stubChannelSouthPartner);

    PlayerSessionManager playerSessions;

    QueryDownloadableApplicationsCommand* cmd =
            new QueryDownloadableApplicationsCommand(&headServerConnection, &controlChannel, &applicationCache, &playerSessions);
    controlChannel.registerCommand(cmd); // channel will take ownership

// -- test
    QJsonObject json;
    json["command"]  = "QueryDownloadableApplications";

    controlChannel.receiveMessageFromSouth(QJsonDocument(json).toJson());

    WAIT_WITH_TIMEOUT(stubChannelSouthPartner->channelSendMessageToSouthCallCount > 0, 30000); // 30s

    ASSERT_EQ(1, stubChannelSouthPartner->channelSendMessageToSouthCallCount);

    ASSERT_TRUE(stubChannelSouthPartner->lastSentMessage.size() > 0);

    QJsonObject answerJson = QJsonDocument::fromJson(stubChannelSouthPartner->lastSentMessage).object();
    EXPECT_TRUE(answerJson["command"].toString() == "QueryDownloadableApplicationsReply");
    EXPECT_TRUE(answerJson["result"].toString() == "ok");
    EXPECT_TRUE(answerJson.contains("applications"));

    // not really interested what apps are there if there are
    // (message syntax is tested elsewhere)
    EXPECT_TRUE(answerJson["applications"].toArray().size() > 0);

    foreach(QJsonValue value, answerJson["applications"].toArray()) {
        QJsonObject appJson = value.toObject();
        QString appFullId = appJson["id"].toString();
        EXPECT_TRUE(appFullId.size() > 0);
        DEBUG("ApplicationFullId:" << appFullId);
    }

}
