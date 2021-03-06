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
 
 #include <gtest/gtest.h>

#include <QObject>

#include <testutils/signalrecorder.h>
#include <testutils/waiter.h>
#include <testutils/qtgtest.h>

#include "server/serverchannelmanager.h"
#include "server/serversideplayerchannel.h"
#include "server/commtcpserver.h"
#include "server/serversidecontrolchannel.h"
#include "server/serversetup.h"
#include "server/channelfactory.h"
#include "client/clientchannelmanager.h"
#include "client/clientsidecontrolchannel.h"
#include "client/commtcpclient.h"
#include "client/clientsetup.h"
#include <client/qml/qmlapplication.h>

#include "utils/testchannelfactory.h"

#include "utils/stubconsoleapplication.h"
#include "testobjects/stubinvocationfactory.h"

// TEST
#include <invocationfactoryimpl.h>
#include <server/consolerestserver.h>


#define LOG_AREA "test_communication_integration"
#include "log/log.h"

TEST(CommunicationIntegration, SetupServerAndClientPingBothWaysWithManualSetup)
{
    // TESTING: basic message flow without ServerSetup or ClientSetup
    //  - we are setting up objects manually

    CommTcpServer tcpServer(7777); // server is comms
    TestChannelFactory testChannelFactory;
    ServerChannelManager serverChannelManager(&testChannelFactory);
    ApplicationRegistry applicationRegistry;
    ConnectionManager connectionManager(&tcpServer, &applicationRegistry, &serverChannelManager);


    QObject::connect(&serverChannelManager, &ServerChannelManager::outgoingMessageToSouth,
                     &connectionManager, &ConnectionManager::outgoingMessageFromChannel);

    QObject::connect(&tcpServer, &CommTcpServer::received,
                     &connectionManager, &ConnectionManager::incomingMessage);

    // TODO: these could be moved to ConnectionManager
    QObject::connect(&tcpServer, &CommTcpServer::connected,
                     &connectionManager, &ConnectionManager::applicationConnected);

    QObject::connect(&tcpServer, &CommTcpServer::disconnected,
                     &connectionManager, &ConnectionManager::applicationDisconnected);

// -- setup ok -> start sever

    tcpServer.open();

// -- setup client side
    CommTcpClient client(7777); // client is any app

    ClientChannelManager clientChannelManager;

    QObject::connect(&client,               &CommTcpClient::received,
                     &clientChannelManager, &ClientChannelManager::processMessage);

    int disconnectedReceived = 0;
    QObject::connect(&client,               &CommTcpClient::disconnected,
                     [&] () { disconnectedReceived++; });

    QObject::connect(&clientChannelManager, &ClientChannelManager::outgoingMessage,
                     &client, &CommTcpClient::write);


    ClientSideControlChannel* clientControl = new ClientSideControlChannel;
    clientChannelManager.registerControlChannel(clientControl);

    int clientPingReceived = 0;
    QObject::connect(clientControl, &ClientSideControlChannel::pingReceived, [&] () { clientPingReceived++; });

    client.open();
    Waiter::wait([&] () { return client.isConnected(); }); // tcp connection ok
    Waiter::wait([&] () { return testChannelFactory.lastControlChannel != nullptr; });
    Waiter::wait([&] () { return clientControl->isActivated(); });
    ASSERT_TRUE(clientPingReceived == 1); // handshaking contains ping

    // TODO: should we check connected() signal or is that done elsewhere

    // connection should have been handshaked

    // -- test
    ASSERT_TRUE(testChannelFactory.lastControlChannel != nullptr);
    ServerSideControlChannel* serverControl = testChannelFactory.lastControlChannel;
    EXPECT_TRUE(serverControl->isOpen());

    int serverPingReceived = 0;
    QObject::connect(testChannelFactory.lastControlChannel, &ServerSideControlChannel::pingSouthReceived, [&] () { serverPingReceived++; });
    int channelCloseCalled = 0;
    QObject::connect(testChannelFactory.lastControlChannel, &ServerSideControlChannel::channelClosed, [&] () { channelCloseCalled++; });

    clientControl->ping();

    Waiter::wait([&] () { return serverPingReceived == 1; });
    EXPECT_EQ(serverPingReceived, 1);

    // answer should come back
    Waiter::wait([&] () { return clientPingReceived == 2; });
    EXPECT_EQ(clientPingReceived, 2);

// - other way round: server -> client
    serverControl->pingSouth();

    Waiter::wait([&] () { return clientPingReceived == 3; });
    EXPECT_EQ(clientPingReceived, 3);

    // answer should come back
    Waiter::wait([&] () { return serverPingReceived == 21; });
    EXPECT_EQ(serverPingReceived, 2);

    // -- closing
    client.close(); // whole tcp connection from client to server is closed: i.e. application closed

    WAIT_AND_ASSERT(channelCloseCalled == 1);

    // On client side closing means tearing down all data structures.
    // Now checking that just signal arrives
    EXPECT_EQ(disconnectedReceived, 1);
    // TODO: when client connection goes down, all control should be recreated

    // -- closing
    tcpServer.close(); // just to be nice
    TRACE("Tearing down PingBothWays");
}


// now using ServerSetup and ClientSetup and pings
TEST(CommunicationIntegration, SetupServerAndClientPingBothWaysWithSetupClasses)
{
    ServerSetup serverSetup;
    TestChannelFactory testChannelFactory;
    serverSetup.use(&testChannelFactory);
    serverSetup.start();

    // --
    ClientSetup clientSetup;

    int clientPingReceived = 0;
    QObject::connect(clientSetup.controlChannel, &ClientSideControlChannel::pingReceived, [&] () { clientPingReceived++; });

    clientSetup.start();

    WAIT_AND_ASSERT(testChannelFactory.lastControlChannel != nullptr);
    WAIT_AND_ASSERT(clientSetup.controlChannel->isActivated());
    ASSERT_TRUE(clientPingReceived == 1); // handshaking contains ping

    // connection should have been handshaked

    // -- test
    int serverPingReceived = 0;
    QObject::connect(testChannelFactory.lastControlChannel, &ServerSideControlChannel::pingSouthReceived, [&] () { serverPingReceived++; });

    clientSetup.controlChannel->ping();

    WAIT_AND_ASSERT(serverPingReceived == 1);
    // answer should come back
    WAIT_AND_ASSERT(clientPingReceived == 2);

// - other way round: server -> client
    testChannelFactory.lastControlChannel->pingSouth();

    WAIT_AND_ASSERT(clientPingReceived == 3);
    // answer should come back
    WAIT_AND_ASSERT(serverPingReceived == 2);

    // -- closing
    ASSERT_TRUE(testChannelFactory.controlChannelClosedCount == 0);
    clientSetup.tcpClient.close(); // whole tcp connection from client to server is closed: i.e. application closed

    WAIT_AND_ASSERT(testChannelFactory.controlChannelClosedCount == 1);
}


TEST(CommunicationIntegration, NewPlayerChannelCreatedAndDestroyed)
{
    ServerSetup serverSetup;
    TestChannelFactory testChannelFactory;
    serverSetup.use(&testChannelFactory);
    serverSetup.start();

    // --
    ClientSetup clientSetup;
    clientSetup.start();

    WAIT_AND_ASSERT(testChannelFactory.lastControlChannel != nullptr);
    WAIT_AND_ASSERT(clientSetup.controlChannel->isActivated());

    // -- new player (remember server is 'comms')

    PlayerMeta playerMeta(100, "foobar");

    ServerSidePlayerChannel* clientPlayer= serverSetup.channelManager->openPlayerChannel(playerMeta);
    Waiter::wait([&] () { return clientSetup.playersManager.numberOfPlayers() > 0; }, true);
    ASSERT_EQ(clientSetup.playersManager.numberOfPlayers(), 1);

    // Waiting OpenChannelAccepted to arrive
    Waiter::wait([&] () { return clientPlayer->isOpen(); }, true);

    // -- close

    serverSetup.channelManager->closePlayerChannel(clientPlayer->channelId()); // sends closing message, no response
    Waiter::wait([&] () { return clientSetup.playersManager.numberOfPlayers() < 1; });
    ASSERT_EQ(clientSetup.playersManager.numberOfPlayers(), 0);

    // closing happens in setup class desctructors
}


TEST(CommunicationIntegration, WhenClientDisconnectsAllServerSideChannelHandlersClosedAndReconnectedLater)
{
    TestChannelFactory testChannelFactory;
    ServerSetup serverSetup;
    serverSetup.use(&testChannelFactory);
    serverSetup.start();

    // we create dynamically because during test we destroy object
    ClientSetup* clientSetup = new ClientSetup();
    clientSetup->start();

    WAIT_AND_ASSERT(testChannelFactory.lastControlChannel != nullptr);
    WAIT_AND_ASSERT(clientSetup->controlChannel->isActivated());

// -- test
    PlayerMeta playerMeta(100, "FooPlayer");

    ServerSidePlayerChannel* serverSidePlayer = serverSetup.channelManager->openPlayerChannel(playerMeta);

    // Waiting OpenChannelAccepted to arrive (OPEN_ONGOING -> OPEN)
    Waiter::wait([&] () { return serverSidePlayer->isOpen(); }, true);

    // -- close
    int channelCloseCalled = 0;
    ASSERT_TRUE(testChannelFactory.lastControlChannel);
    QObject::connect(testChannelFactory.lastControlChannel, &ServerSideControlChannel::channelClosed,
                     [&] () { channelCloseCalled++;} );

    // when serverside notices disconnect all channels should be closed
    clientSetup->tcpClient.close();
    WAIT_AND_ASSERT(channelCloseCalled == 1);
    WAIT_AND_ASSERT(!serverSidePlayer->isOpen());

    // TODO: what happens to client side (typically this doesn't happen but ...)
    ASSERT_EQ(clientSetup->playersManager.numberOfPlayers(), 0);

    // test also tearing down (no crashes)
    delete clientSetup;

    // -- reconnect
    // first root channels handshake with ping messages
    // then playerchannels are opened

    clientSetup = new ClientSetup();
    clientSetup->start();
    Waiter::wait([&] () { return clientSetup->tcpClient.isConnected(); });

    // when server side recognizes connection is back it should reconnect
    WAIT_AND_ASSERT(serverSidePlayer->isOpen());
    ASSERT_EQ(clientSetup->playersManager.numberOfPlayers(), 1);
    ASSERT_TRUE(testChannelFactory.lastControlChannel != NULL);

    // -- tear down
    // closing happens in setup class desctructors
    delete clientSetup;
}


/* TODO: can be taken  away?
TEST(CommunicationIntegration, AppSendsDataAndPlayerResponds)
{
    // TODO: For some reason we get socket disconnect right away.
    //       If run alone OK, but if with others (even when all tests pass)
    //       we get disconnected() signal. Not sure why.
    //         - I guess having proper test fixture with tear down
    //           in any case (wait sockets closed) would solve situation.
    //
    //Waiter::wait([&] () { return false; });
    Waiter::wait([&] () { return false; }, true);

    ServerSetup serverSetup;
    serverSetup.start();

    ClientSetup* clientSetup = new ClientSetup();
    clientSetup->start();

    int playerInReceived = 0;
    int playerIdReceived = -1;
    QObject::connect(&(clientSetup->playersManager), &PlayersManager::playerIn,
                     [&] (int playerId) { playerInReceived++; playerIdReceived = playerId; });

    int playerMessageReceived = 0;
    QByteArray playerInMessage;
    QObject::connect(&(clientSetup->playersManager), &PlayersManager::playerMessageReceived,
                     [&] (int playerId, QByteArray msg) {
        Q_UNUSED(playerId);
        playerMessageReceived++; playerInMessage = msg;
    });

    Waiter::waitAndAssert([&] () { return clientSetup->tcpClient.isConnected(); });
    Waiter::waitAndAssert([&] () { return serverSetup.connectionManager->activeConnection(); });

    PlayerMeta playerMeta(100, "FooPlayer");
    ServerSidePlayerChannel* serverSidePlayer = serverSetup.channelManager->openPlayerChannel(playerMeta);
    Waiter::waitAndAssert([&] ()
        { return serverSidePlayer->isOpen(); }, true);

    int serverPlayerMessageReceived = 0;
    QByteArray serverPlayerMessage;
    QObject::connect(serverSetup.playerConnectionManager, &PlayerConnectionManager::playerMessageFromSouth,
                     [&] (int channelId, const QByteArray msg) {
        Q_UNUSED(channelId);
        serverPlayerMessageReceived++; serverPlayerMessage = msg;
    });

    // --
    QByteArray serverMsg("serverMsg");
    qDebug("###########################");
    serverSidePlayer->receivePlayerMessageFromNorth(serverMsg);
    Waiter::wait([&] () { return playerMessageReceived > 0; }, true, 5000, 500);
    ASSERT_EQ(playerMessageReceived, 1);
    EXPECT_EQ(playerIdReceived, 100);
    EXPECT_TRUE(QString(playerInMessage) == serverMsg);

    QByteArray responseMsg("responseMsg");
    clientSetup->playersManager.sendPlayerMessage(100, responseMsg);
    Waiter::wait([&] () { return serverPlayerMessageReceived > 0; });
    ASSERT_EQ(serverPlayerMessageReceived, 1);
    EXPECT_TRUE(QString(serverPlayerMessage) == responseMsg);

    // -- close nicely
    clientSetup->tcpClient.close();
    Waiter::wait([&] () { return !serverSidePlayer->isOpen(); });
}
*/

// TODO: test client tcp connection dead -> close all channels -> reconnecting

TEST(CommunicationIntegration, FullPipeFromSouthToNorth)
{
    ServerSetup serverSetup;
    TestChannelFactory testChannelFactory;
    serverSetup.use(&testChannelFactory);
    serverSetup.start();

    ClientSetup clientSetup;
    clientSetup.start();

    // -- wait south side up
    int playerInReceived = 0;
    int playerIdReceived = -1;
    QObject::connect(&(clientSetup.playersManager), &PlayersManager::playerIn,
                     [&] (int playerId) { playerInReceived++; playerIdReceived = playerId; });

    int playerMessageReceived = 0;
    QByteArray playerInMessage;
    QObject::connect(&(clientSetup.playersManager), &PlayersManager::playerMessageReceived,
                     [&] (int playerId, QByteArray msg) {
        Q_UNUSED(playerId);
        playerMessageReceived++; playerInMessage = msg;
    });

    Waiter::waitAndAssert([&] () { return clientSetup.tcpClient.isConnected(); });
    Waiter::waitAndAssert([&] () { return serverSetup.connectionManager->activeConnection(); });

    // -- north side client

    StubInvocationFactory stubInvocationFactory; // no need for head server connection
    StubConsoleApplication stubConsoleApp(&stubInvocationFactory);
    GBerryClient::QmlApplication northApplication(&stubConsoleApp);

    bool consoleConnectionIsOpen = false;
    QObject::connect(&northApplication, &GBerryClient::QmlApplication::consoleConnectionOpened,
                     [&] () { consoleConnectionIsOpen = true; });

    bool northSideClientMessageReceived = false;
    QString northSideClientMessage;
    QObject::connect(&northApplication, &GBerryClient::QmlApplication::playerMessageReceived,
                     [&] (QString msg) { northSideClientMessageReceived = true; northSideClientMessage = msg; });

    UserModel& userModel = stubConsoleApp.userModel();
    userModel.setUser(UserInfo("FooGuest", "", "", true, false));
    userModel.selectCurrentUser("FooGuest");

    northApplication.openConsoleConnection("localhost");

    WAIT_CUSTOM_AND_ASSERT(consoleConnectionIsOpen, 5000, 50);

    // wait whole communication channel to open (tcp ip communication back and worth)
    WAIT_AND_ASSERT(playerInReceived == 1);

    // channels open -> send message
    northApplication.sendMessage("test message");
    WAIT_AND_ASSERT(playerMessageReceived == 1);
    EXPECT_TRUE(QString(playerInMessage) == "test message");

    // and then other direction
    clientSetup.playersManager.sendPlayerMessage(playerIdReceived, "all ok");
    WAIT_AND_ASSERT(northSideClientMessageReceived);
    EXPECT_TRUE(northSideClientMessage == "all ok");

    // -- closing
    northApplication.closeConsoleConnection();

    WAIT_AND_ASSERT(clientSetup.playersManager.numberOfPlayers() == 0);
    TRACE("DONE");
}


// multiple players
TEST(CommunicationIntegration, FullPipeWithThreeNorthClients)
{
    TestChannelFactory testChannelFactory;
    ServerSetup serverSetup;
    serverSetup.use(&testChannelFactory);
    serverSetup.start();

    ClientSetup* southClientSetup = new ClientSetup(); // TODO: this is more like southsideclient
    southClientSetup->start();

    // -- wait south side up
    QMap<QString, int> playerIdByPlayerName;
    QObject::connect(&(southClientSetup->playersManager), &PlayersManager::playerIn,
                     [&] (int playerId) {
        playerIdByPlayerName[southClientSetup->playersManager.playerName(playerId)] = playerId; });


    int playerMessageReceived = 0;
    QByteArray playerInMessage;
    QObject::connect(&(southClientSetup->playersManager), &PlayersManager::playerMessageReceived,
                     [&] (int playerId, QByteArray msg) {
        Q_UNUSED(playerId);
        playerMessageReceived++; playerInMessage = msg;
    });

    Waiter::waitAndAssert([&] () { return southClientSetup->tcpClient.isConnected(); });
    Waiter::waitAndAssert([&] () { return serverSetup.connectionManager->activeConnection(); });

    // -- north side clients

    StubInvocationFactory stubInvocationFactory1; // no need for head server connection
    StubConsoleApplication stubConsoleApp1(&stubInvocationFactory1);
    GBerryClient::QmlApplication northApplication1(&stubConsoleApp1);

    StubInvocationFactory stubInvocationFactory2; // no need for head server connection
    StubConsoleApplication stubConsoleApp2(&stubInvocationFactory2);
    GBerryClient::QmlApplication northApplication2(&stubConsoleApp2);

    StubInvocationFactory stubInvocationFactory3; // no need for head server connection
    StubConsoleApplication stubConsoleApp3(&stubInvocationFactory3);
    GBerryClient::QmlApplication northApplication3(&stubConsoleApp3);


    QString northSideClient1Message;
    QObject::connect(&northApplication1, &GBerryClient::QmlApplication::playerMessageReceived,
                     [&] (QString msg) { northSideClient1Message = msg; });

    QString northSideClient2Message;
    QObject::connect(&northApplication2, &GBerryClient::QmlApplication::playerMessageReceived,
                     [&] (QString msg) { northSideClient2Message = msg; });

    QString northSideClient3Message;
    QObject::connect(&northApplication3, &GBerryClient::QmlApplication::playerMessageReceived,
                     [&] (QString msg) { northSideClient3Message = msg; });

    stubConsoleApp1.userModel().setUser(UserInfo("Guest1Foo", "", "", true, false));
    stubConsoleApp1.userModel().selectCurrentUser("Guest1Foo");
    northApplication1.openConsoleConnection("localhost");

    stubConsoleApp2.userModel().setUser(UserInfo("Guest2Foo", "", "", true, false));
    stubConsoleApp2.userModel().selectCurrentUser("Guest2Foo");
    northApplication2.openConsoleConnection("localhost");

    stubConsoleApp3.userModel().setUser(UserInfo("Guest3Foo", "", "", true, false));
    stubConsoleApp3.userModel().selectCurrentUser("Guest3Foo");
    northApplication3.openConsoleConnection("localhost");


    WAIT_CUSTOM_AND_ASSERT(northApplication1.isConsoleConnectionOpen(), 5000, 50);
    WAIT_CUSTOM_AND_ASSERT(northApplication2.isConsoleConnectionOpen(), 5000, 50);
    WAIT_CUSTOM_AND_ASSERT(northApplication3.isConsoleConnectionOpen(), 5000, 50);

    // wait whole communication channel to open (tcp ip communication back and worth)
    WAIT_AND_ASSERT(southClientSetup->playersManager.numberOfPlayers() == 3);
    ASSERT_TRUE(playerIdByPlayerName.size() == 3);

    // channels open -> send messages (mixed directions)
    northApplication1.sendMessage("test1 message");
    northApplication2.sendMessage("test2 message");
    // TODO: problem -> for guest players need to generate unique ids
    // TODO: by name find out who is who
    southClientSetup->playersManager.sendPlayerMessage(playerIdByPlayerName["Guest3Foo"], "all3 ok");

    WAIT_AND_ASSERT(playerMessageReceived == 2);
    WAIT_AND_ASSERT(northSideClient3Message == "all3 ok");
    // and then other direction
    southClientSetup->playersManager.sendPlayerMessage(playerIdByPlayerName["Guest1Foo"], "all1 ok");
    southClientSetup->playersManager.sendPlayerMessage(playerIdByPlayerName["Guest2Foo"], "all2 ok");
    northApplication2.sendMessage("test3 message");

    WAIT_WITH_TIMEOUT(northSideClient1Message == "all1 ok", 5000);
    DEBUG("northSideClient1Message:" << northSideClient1Message);

    WAIT_AND_ASSERT(northSideClient1Message == "all1 ok");
    WAIT_AND_ASSERT(northSideClient2Message == "all2 ok");
    WAIT_AND_ASSERT(playerMessageReceived == 3);

// -- close south side application
    delete southClientSetup;

    // wait situation settle
    WAIT_AND_ASSERT(testChannelFactory.controlChannelClosedCount == 1);

// -- new south side client
    ASSERT_TRUE(testChannelFactory.lastControlChannel == nullptr);

    southClientSetup = new ClientSetup();
    southClientSetup->start();

    playerMessageReceived = 0;
    QObject::connect(&(southClientSetup->playersManager), &PlayersManager::playerMessageReceived,
                     [&] (int playerId, QByteArray msg) {
        Q_UNUSED(playerId);
        playerMessageReceived++; playerInMessage = msg;
    });

    // reconnection occurs
    WAIT_AND_ASSERT(testChannelFactory.lastControlChannel != nullptr);
    WAIT_AND_ASSERT(testChannelFactory.lastControlChannel->isOpen());
    WAIT_AND_ASSERT(southClientSetup->playersManager.numberOfPlayers() == 3);
  // --

    // test sending messages
    southClientSetup->playersManager.sendPlayerMessage(playerIdByPlayerName["Guest1Foo"], "ok1");
    southClientSetup->playersManager.sendPlayerMessage(playerIdByPlayerName["Guest2Foo"], "ok2");
    northApplication3.sendMessage("yes3");

    WAIT_AND_ASSERT(northSideClient1Message == "all1 ok");
    WAIT_AND_ASSERT(northSideClient2Message == "all2 ok");
    WAIT_AND_ASSERT(playerMessageReceived == 1)

// -- closing
    northApplication1.closeConsoleConnection();
    northApplication2.closeConsoleConnection();
    northApplication3.closeConsoleConnection();

    WAIT_AND_ASSERT(southClientSetup->playersManager.numberOfPlayers() == 0);
    qDebug("DONE");
}
