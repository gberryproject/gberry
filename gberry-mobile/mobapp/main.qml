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
 
import QtQuick 2.4
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

import GBerry 1.0

import "settings"
import "login"

import "js/MobileClientMessages.js" as Messages
import "js/AppBox.js" as AppBox
import "settings/SettingsModel.js" as SettingsModel

Window {
    id: root
    visible: true
    width: DisplayProfile.windowWidth //screen.preferredWindowWidth
    height: DisplayProfile.windowHeight //screen.preferredWindowHeight

    // global settings
    ApplicationSettings { id: gsettings }
    GDisplayProfile { z: 1000; id: gdisplay; } //scaleFactor: 0.5

    // for desktop development - easy test of scaling
    onHeightChanged: { gdisplay.adjust(width, height) }
    onWidthChanged: { gdisplay.adjust(width, height) }

    MouseArea {
        id: mainMouseArea
        anchors.fill: parent
        //propagateComposedEvents: true

        onClicked: {
            console.debug("MOUSE CLICKED ON MAIN MOUSEAREA")

            // make sure dropdown menus are closed
            toggleGeneralActions(false)
            toggleLocalGeneralActions(false)
        }
    }

    Rectangle {
        id: topbar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        color: "#C2E0FF" // LightSkyBlue"
        width: root.width
        height: topbarContainer.height * 1.15
        gradient: gsettings.titleGradient

        Item {
            id: topbarContainer
            anchors.centerIn: parent
            height: gdisplay.touchCellHeight() * 0.5
            width: root.width //- gdisplay.touchCellWidth() / 2

            Item {
                //color: "yellow"
                id: toggleLocalGeneralActionsButtonFrame
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter

                height: topbarContainer.height
                width: toggleLocalGeneralActionsButton.width *2

                IconButton {
                    id: toggleLocalGeneralActionsButton
                    anchors.left: parent.left
                    anchors.leftMargin: gdisplay.touchCellWidth() * 0.20
                    anchors.verticalCenter: parent.verticalCenter

                    targetHeight: topbarContainer.height
                    height: preferredHeight
                    width: preferredWidth
                    imageSource: "images/menu_bars.svg"

                    onButtonClicked: {
                        toggleLocalGeneralActions()
                        toggleGeneralActions(false)
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        toggleLocalGeneralActionsButton.clickButton()
                    }
                }
            }


            Item {
                //color: "green"
                anchors.left: toggleLocalGeneralActionsButtonFrame.right
                anchors.right: toggleGeneralActionsButtonFrame.left
                anchors.leftMargin: gdisplay.touchCellWidth()/4
                anchors.rightMargin: gdisplay.touchCellWidth()/4
                anchors.top: parent.top
                anchors.topMargin: -2
                anchors.bottomMargin: -1
                anchors.bottom: parent.bottom

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: currentPlayerLabel.left
                    anchors.rightMargin: sourceSize.width * 0.5

                    source: {
                        if (UserModel.currentIsGuest) {
                            return ""
                        }

                        if (LoginModel.isLoggedIn) {
                            return "images/user_green.svg"
                        } else {
                            return "images/user_yellow.svg"
                        }
                    }

                    sourceSize.width: topbarContainer.height * 0.5
                }

                Text {
                    id: currentPlayerLabel

                    color: mobapp.loggedIn ? "black" : "grey"
                    anchors.centerIn: parent
                    font.pixelSize: toggleLocalGeneralActionsButton.height - 4
                    text: UserModel.currentUserIsActive ? UserModel.currentUserName : " --- "
                }

                MouseArea {
                    anchors.fill: parent
                    propagateComposedEvents: true
                    onClicked: {
                        if (!anyGeneralActionsVisible()) {
                            onUserManagementSelected()
                        } else {
                            mouse.accepted = false // propagate
                        }
                    }
                }
            }

            Item {
                //color: "yellow"
                id: toggleGeneralActionsButtonFrame
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: toggleLocalGeneralActionsButtonFrame.height
                width: toggleLocalGeneralActionsButtonFrame.width

                IconButton {
                    id: toggleGeneralActionsButton
                    //visible: false
                    enabled: generalActions.hasActions
                    anchors.right: parent.right
                    anchors.rightMargin: topbarContainer.height * 0.20
                    anchors.verticalCenter: parent.verticalCenter

                    imageSource: "images/menu_dots.svg"
                    targetHeight: topbarContainer.height

                    height: preferredHeight
                    width: toggleLocalGeneralActionsButton.width
                    rotation: 90

                    onButtonClicked: {
                        console.debug("button clicked!")
                        toggleGeneralActions()
                        toggleLocalGeneralActions(false)
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: toggleGeneralActionsButton.clickButton()
                }
            }
        }
    }

    Item {
        anchors.top: topbar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        DefaultMainArea {
            id: ui
            visible: true
            anchors.fill: parent
        }

        BasicControls {
            id: basicControls
            visible: false
            anchors.fill: parent
            maxWidth: root.width
            //maxHeight: root.height - topbar.height
            maxHeight: Screen.primaryOrientation === Qt.PortraitOrientation ? root.height : root.width

            //onMaxHeightChanged: { console.debug("### BC macHeight: " + maxHeight) }

            Component.onCompleted: {
                basicControls.buttonPressed.connect(function (buttonID) {basicControlButtonPressed(buttonID)})
            }
        }

        GConfirmationDialog {
            id: msgDiag
            visible: false
            border.width: 1
            border.color: "gray"
            textPixelSize: gdisplay.smallSizeText
            onOption1Selected: {
                msgDiag.visible = false
                var js = { action: "ConfirmationQuestionResponse",
                           questionId: msgDiag.questionId,
                           ref: msgDiag.option1Id }
                mobapp.sendMessage(JSON.stringify(js))
            }
            onOption2Selected: {
                msgDiag.visible = false
                var js = { action: "ConfirmationQuestionResponse",
                           questionId: msgDiag.questionId,
                           ref: msgDiag.option2Id }
                mobapp.sendMessage(JSON.stringify(js))
            }

        }


        Rectangle {
            id: appbox
            visible: false
            color: "pink"
            anchors.fill: parent

            Component.onCompleted: {
                AppBox.initialiaze(appbox)
                // make connection between dynamic content and our function
                // (this is not direct connect, we just define function that should
                //  be connected when dynamic content is in place)
                AppBox.connectOutgoingMessageTo(receiveFromAppBox)
            }

            function receiveFromAppBox(message) {
                console.debug("### RECEIVED FROM APPBOX: " + message)
                var js = Messages.createCustomAppBoxMsg(message)
                mobapp.sendMessage(js)
            }
        }

        Settings {
            id: settingsView
            visible: false // initial state
        }

    }

    GeneralActions {
        id: generalActions
        visible: false
        anchors.top: topbar.bottom
        anchors.right: parent.right
        maxWidth: Screen.primaryOrientation === Qt.PortraitOrientation ? root.width : root.height
        maxHeight: (Screen.primaryOrientation === Qt.PortraitOrientation ? root.height : root.width) - topbar.height
    }

    GeneralActions {
        id: localGeneralActions
        visible: false
        anchors.top: topbar.bottom
        anchors.left: parent.left
        //maxWidth: Screen.primaryOrientation === Qt.PortraitOrientation ? root.width : root.height
        //maxHeight: (Screen.primaryOrientation === Qt.PortraitOrientation ? root.height : root.width) - topbar.height

        maxWidth: root.width
        maxHeight: root.height - topbar.height

        onWidthChanged: { console.debug("##### maxWidth: " + maxWidth) }
        onHeightChanged: { console.debug("#### maxHeight: " + maxHeight) }

        onMaxHeightChanged: { console.debug("##### maxWidth: " + maxWidth) }
        onMaxWidthChanged: { console.debug("#### maxHeight: " + maxHeight) }
    }


    ModalDialogFrame {
        content: LoginView {
            id: loginview

            function show(msg) { parent.show() }
            function hide() { parent.hide() }
        }
    }

    ModalDialogFrame {
        content: LoginWait {
            id: loginWait
            property bool reconnectRequired: false

            function show(msg) { parent.show() }
            function hide() { parent.hide() }

            onViewClosed: { parent.hide() }

            onLoginOk: {
                // if valid user -> reconnect
                if (reconnectRequired && UserModel.currentUserIsActive) {
                    onConnectRequested()
                }
            }
        }
    }

    ModalDialogFrame {
        //onBackgroundClicked: console.debug("robber of events!")
        id: aboutview

        content: AboutView {
            onViewClosed: parent.hide()
        }
    }

    ModalDialogFrame {
        id: disconnectConsoleDialog

        content: GConfirmationDialog {
            textPixelSize: gdisplay.smallSizeText
            questionText: qsTr("You are logged in. Disconnect?")
            option1Text: qsTr("Yes")
            option2Text: qsTr("No")

            onOption1Selected: {
                parent.hide()
                onDisconnectRequested()
            }

            onOption2Selected: {
                parent.hide()
            }
        }
    }

    ModalDialogFrame {
        content: GErrorDialog {
            id: errorDialog
            //visible: false
            textPixelSize: gdisplay.smallSizeText

            //onVisibleChanged: console.debug("VISIBLE CHANGED FOR ERROR")
            function show(msg) { errorMessage = msg; parent.show() }
            function hide() { parent.hide() }

            onAcknowledged: { hide() }
        }
    }


    // aligned to bottom of view - no interaction - just fading out
    GFeedbackDialog {
        id: feedbackBox
        visible: false
        //feedbackMessage: "This is a test message, quite long This is a test message, quite long This is a test message, quite long"
        showingTime: 2000 // ms
        height: preferredHeight
        initialOpacity: 0.6 // show partly what is behind
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }


    // -------------------- FUNCTIONS -------------------

    function basicControlButtonPressed(button)
    {
        Log.debug("CLICKED: " + button)
        var js = {action: "SelectBasicControlAction",
                  id: button}
        var data = JSON.stringify(js)
        Log.debug("SEND: " + data)
        mobapp.sendMessage(data)
    }

    // 'show' is optional, forces visibility to true/false
    function toggleGeneralActions(show) {
        if (typeof show !== 'undefined')
            generalActions.show(show)
        else
            generalActions.show(!generalActions.visible)
    }

    function onGeneralActionSelected(actionId) {
        mobapp.sendMessage(Messages.createGeneralActionMessage(actionId))

        // TODO: action button should have a feedback
        // TODO: and because of that dropdown menu shouldn't close immediately
        generalActions.show(false)
    }

    // 'show' is optional, forces visibility to true/false
    function toggleLocalGeneralActions(show) {
        if (typeof show !== 'undefined')
            localGeneralActions.show(show)
        else
            localGeneralActions.show(!localGeneralActions.visible)
    }

    function anyGeneralActionsVisible() {
        return generalActions.visible || localGeneralActions.visible
    }

    function onUserManagementSelected(show) {
        if (typeof show === 'undefined' || show === true) {
            // TODO: if already logged in then show logout dialog
            loginview.show()
            settingsView.visible = false
            aboutview.hide()
        } else {
            loginview.hide()
            // TODO: view model not good one, we don't know what to show
        }
    }

    function onLocalGeneralActionSelected(actionId) {
        console.debug("LOCAL GENERAL ACTION: " + actionId)
        toggleLocalGeneralActions(false) // hide

        if (actionId === "Settings") {
            settingsView.visible = true
            // TODO: state?
            loginview.hide()
            aboutview.hide()

        } else if (actionId === "Logout") {
            // TODO: remove
            onDisconnectRequested()

        } else if (actionId === "About") {
            if (!aboutview.visible) {
                aboutview.show()
                // we keep other view as it is on the background
                //loginview.hide()
                //settingsView.visible = false
            }

        } else if (actionId === "Webstore") {
            console.debug("Opening webstore: " + SettingsModel.serverAddress())
            // TODO: should this be configurable?? (/store)
            Qt.openUrlExternally("http://" + SettingsModel.serverAddress() +"/store")

        } else if (actionId === "Home") {
            // first put to unvisible to that hooks may set model
            aboutview.hide()
            loginview.hide()
            settingsView.visible = false

            ui.hostNameToConnect = SettingsModel.consoleAddress()
            console.debug("UPDATING CONSOLE ADDRES TO " + ui.hostNameToConnect)

            // TODO: could we use stacked view?

        } else if (actionId === "Console") {
            if (mobapp.loggedIn) {
                disconnectConsoleDialog.show()
            }

            // first put to unvisible to that hooks may set model
            /*
            aboutview.hide()
            loginview.hide()
            settingsView.visible = false
            ui.visible = true
            appbox.visible = false
            */
        }


        // TODO: other kind of list, now always need to add if
    }

    function onPlayerMessageReceived(data) {
        console.log("PLAYER MESSAGE: " + data)

        var js = JSON.parse(data)
        if (js["action"] === "ConfirmationQuestion") {
            if (typeof(js["title"]) !== "undefined") {
                msgDiag.titleText = js["title"]
            }
            msgDiag.questionText = js["text"]
            msgDiag.questionId = js["questionId"]

            // we expect exactly two options (might be dangerous)
            msgDiag.option1Id = js["options"][0]["id"]
            msgDiag.option1Text = js["options"][0]["text"]

            if (js["options"].length > 1) {
                msgDiag.option2Id = js["options"][1]["id"]
                msgDiag.option2Text = js["options"][1]["text"]
            } else {
                // clear potential previous data (TODO: actually should be dialog responsibility=
                msgDiag.option2Id = ""
                msgDiag.option2Text = ""
            }

            // TODO: we should be modal and disable controls on background
            msgDiag.visible = true

        } else if (js["action"] === "CloseQuestion") {
            // currently we may have only one dialog instanceof
            msgDiag.visible = false

        } else if (js["action"] === "DefineAppBox") {
            AppBox.createContent(js["data"])

        } else if (js["action"] === "ShowAppBox") {
            appbox.visible = true
            AppBox.showContent() // TODO: should appbox be also set visible here??
            basicControls.visible = false
            ui.visible = false
            msgDiag.visible = false

        } else if (js["action"] === "AppBoxMessage") {
            AppBox.sendMessage(js["data"])

        } else if (js["action"] === "ShowBasicControls") {
            appbox.visible = false
            basicControls.visible = true
            console.debug("### " + js['enable'].toString())
            if (js.hasOwnProperty("enable")) {
                basicControls.enable(js['enable']) // list
            } else {
                basicControls.enable([]) // all buttons
            }

            ui.visible = false
            msgDiag.visible = false

        } else if (js["action"] === "DefineGeneralActions") {
            generalActions.setActions(js["actions"])

        } else if (js["action"] === "FeedbackMessage") {
            feedbackBox.show(js["message"])
        }
    }

    function onLoginViewClosed()
    {
        // no actions, really a cancel operation of showing login view
        loginview.hide()
    }
    // TODO: how case when already connection but logging out and loggin in

    function onLogin(userName)
    {
        console.debug("onLogin()")
        loginview.hide()
        var wasConnectedToConsole = mobapp.loggedIn

        if (mobapp.loggedIn) {
            console.debug("Disconnecting previous connection")
            onDisconnectRequested()
        }

        console.debug("Selecting new user")
        UserModel.selectCurrentUser(userName)

        // login is async operation -> reconnection to console can occur on background

        if (!UserModel.currentIsGuest) {
            loginWait.reconnectRequired = wasConnectedToConsole
            loginWait.show()
            console.debug("Initiating login")
            LoginModel.login()

            // once login is ready we need to do connect:
            //   - happens in LoginWait::onLoginOk()

        } else {
            console.debug("Guest login")

            // if valid user -> reconnect
            if (wasConnectedToConsole && UserModel.currentUserIsActive) {
                onConnectRequested()
            }
        }


        // no really actions
        //   - name on title bar should update automatically
     }

     function onConnectRequested() {
         if (!UserModel.currentUserIsActive) {
             errorDialog.show(qsTr("No user selected"))
             return
         }

        var username = UserModel.currentUserName
        var password = UserModel.currentPassword
        var guest = UserModel.currentIsGuest
        var rememberPassword = UserModel.currentIsRememberPassword

        console.debug("LOGIN: " + username + ", " + password + ", " + (guest ? "GUEST" : "NORMAL") + ", " + (rememberPassword ? "REMEMBER" : "-"))
        console.debug("USING CONSOLE ADDRESS: " + SettingsModel.consoleAddress())
        mobapp.openConsoleConnection(SettingsModel.consoleAddress()) // if opening fails then signal is emitted
        loginview.hide()
        ui.state = "CONNECTING"


        // TODO: if connection takes time, we should actually show some kind of status bar

        // TODO: widget should take care by itself a status
        //currentPlayerLabel.text = username

        // TODO: how to show login errors?
    }

    function onDisconnectRequested() {
        mobapp.closeConsoleConnection()

        // TODO: we should have some kind of stacked view -> no matter what is open
        loginview.hide()
        settingsView.visible = false
        aboutview.hide()
        basicControls.visible = false
        appbox.visible = false

        ui.visible = true
        ui.state = "DISCONNECTED"

        generalActions.clearActions()
    }


    function onConnectFailed(errorMsg) {
        console.debug("Login failed: " + errorMsg)
        errorDialog.show(errorMsg)
        ui.state = "DISCONNECTED"
    }

    function onLogout(userName) {
        loginview.hide()

        if (LoginModel.isLoggedIn) {
            LoginModel.logout()
        }

        if (mobapp.loggedIn) {
            onDisconnectRequested()
        }
        UserModel.unselectCurrentUser()
    }

    // TODO: we should handle connection closed

    Component.onCompleted: {
        Log.initLog("main", Log.DEBUG_LEVEL)

        mobapp.playerMessageReceived.connect(onPlayerMessageReceived)
        mobapp.consoleConnectionOpenFailed.connect(onConnectFailed)
        mobapp.consoleConnectionClosed.connect(onDisconnectRequested) // TODO: should we have some kind of info for user what happened

        generalActions.actionSelected.connect(onGeneralActionSelected)

        localGeneralActions.setActions(
            [{actionId: "Home", actionName: "Home"},
             {actionId: "Console", actionName: "Console"},
             {actionId: "Webstore", actionName: "Webstore"},
             {actionId: "Settings", actionName: "Settings"},
             {actionId: "About", actionName: "About"}
        ])
        localGeneralActions.actionSelected.connect(onLocalGeneralActionSelected)

        generalActions.calcScaling()
        localGeneralActions.calcScaling()

        loginview.viewClosed.connect(onLoginViewClosed)
        loginview.login.connect(onLogin)
        loginview.logout.connect(onLogout)

        ui.connectToConsoleRequested.connect(onConnectRequested)
        ui.hostNameToConnect = SettingsModel.consoleAddress()

        Log.debug("desktopAvailableHeight: " + Screen.desktopAvailableHeight)

        Log.debug("screen.preferredWindowWidth: " + screen.preferredWindowWidth)
        Log.debug("screen.preferredWindowHeight: " + screen.preferredWindowHeight)

        // TODO: how to impl dynamic binding for devenv
        //root.width = screen.preferredWindowWidth
        //root.height = screen.preferredWindowHeight
        // TODO: more info

        // TODO: should change name of prop!!!
        if (UserModel.autoLoginEnabled) {
            Log.debug("Auto login enabled")
            onConnectedRequest()
        }

        // debug
        console.debug("CurrentUserActive: " + UserModel.currentUserIsActive.toString())
        console.debug("Current user name: '" + UserModel.currentUserName + "'")

        // testing
        //feedbackDialog.show("jjadadadasdafdafa")
        //errorDialog.show("Test error")
        //loginWait.show()
    }
}

// TODO: could camera be used to take user photo
