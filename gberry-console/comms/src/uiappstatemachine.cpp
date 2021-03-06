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
 
 #include "uiappstatemachine.h"
#include "uiappstatemachine_private.h"

#define LOG_AREA "UIAppStateMachine"
#include "log/log.h"

UIAppStateMachine::UIAppStateMachine(
        IApplicationController* waitApp,
        IApplicationController* mainui,
        ILaunchController* currentApp) :
    _waitapp(waitApp),
    _mainui(mainui),
    _currentApp(currentApp)
{
    _impl = new UIAppStateMachinePrivate();

    auto registerState = [&] (State* state) {
        _stateMachine.addState(state);
        connect(state, &State::enteredState, [&] (QString stateName) { _currentStateName = stateName; });
        return state;
    };

    // -- first create all states
    State *startup = registerState(new State(STATE_STARTUP));
    _stateMachine.setInitialState(startup);

    State *waitAppVisibleLaunchingMainUI = registerState(new State(STATE_WAITAPP_LAUNCHING_MAINUI));

    State *mainuiVisible = registerState(new State(STATE_MAINUI));

    State *waitAppVisibleLaunchingApp = registerState(new State(STATE_WAITAPP_LAUNCHING_APP));

    State *appVisible = registerState(new State(STATE_APP));

    // -- actions and state changes

    // ACTION: show waiting screen
    connect(startup, &State::entered, _waitapp, &IApplicationController::launch);
    startup->addTransition(_impl, SIGNAL(waitappLaunchValidated()), waitAppVisibleLaunchingMainUI);

    // ACTION: once waiting screen visible start mainui
    connect(waitAppVisibleLaunchingMainUI, &State::entered, _mainui, &IApplicationController::launch);
    //waitAppVisibleLaunchingMainUI->addTransition(_mainui, SIGNAL(launched()), mainuiVisible);
    waitAppVisibleLaunchingMainUI->addTransition(_impl, SIGNAL(mainuiLaunchValidated()), mainuiVisible);

    connect(mainuiVisible, &State::entered, [&] () {
        _waitapp->pause();
    });

    // ACTION: closing mainui and showing waitapp until app has started
    mainuiVisible->addTransition(_impl, SIGNAL(appLaunchRequested()), waitAppVisibleLaunchingApp);
    connect(waitAppVisibleLaunchingApp, &State::entered, [&] () {
        _waitapp->resume();
        _mainui->stop();
        _currentApp->launch();
    });

    //waitAppVisibleLaunchingApp->addTransition(_currentApp, SIGNAL(launched()), appVisible);
    waitAppVisibleLaunchingApp->addTransition(_impl, SIGNAL(appLaunchValidated()), appVisible);
    connect(appVisible, &State::entered, [&] () {
        _waitapp->pause();
    });

    // ACTION: if launch fails then show mainui again
    waitAppVisibleLaunchingApp->addTransition(_currentApp, SIGNAL(launchFailed()), waitAppVisibleLaunchingMainUI);

    // this works as there is no other target states, otherwise this should tight to transition
    connect(appVisible, &State::exited, [&] () {
        _waitapp->resume();
        _currentApp->stop();
    });

    // ACTION: when exit requested, show first waitapp then launch mainui
    appVisible->addTransition(_impl, SIGNAL(appExitRequested()), waitAppVisibleLaunchingMainUI);

    // ACTION: if application dies -> launch mainui (while showing wait screen)
    appVisible->addTransition(_currentApp, SIGNAL(died()), waitAppVisibleLaunchingMainUI);

    //

    // TODO: other states

}

UIAppStateMachine::~UIAppStateMachine()
{
    delete _impl;
}

void UIAppStateMachine::start()
{
    _stateMachine.start();
}

void UIAppStateMachine::lauchApplication(const QString& applicationId)
{
    _currentApp->useApplication(applicationId);
    _impl->emitAppLaunchRequested();
}

void UIAppStateMachine::exitApplication(const QString& applicationId)
{
    if (applicationId.isEmpty() || applicationId == _currentApp->fullApplicationId()) {
        _impl->emitAppExitRequested();
    } else {
        WARN("Ignoring exit request as application is not current application: applicationId =" << applicationId);
    }
}

QString UIAppStateMachine::debugCurrentStateName() const
{
    return _currentStateName;
}

void UIAppStateMachine::applicationConnectionValidated(const QString& applicationId)
{
    if (applicationId == "waitapp") {
        _impl->emitWaitAppLaunchValidated();
    } else if (applicationId == "mainui") {
        _impl->emitMainUILaunchValidated();
    } else {
        _impl->emitAppLaunchValidated();
    }

    // TODO: is it possible that states get confused? we could store code and require to have matching too
}

// TODO: during devtime "waitapp" and "mainui" codes could be valid

/*
QState *startup = new QState();
machine.addState(startup);
machine.setInitialState(startup);
// on entry lauch waiting screen
// once waiting screen running (how to get that signal) can we have multiple apps connected to comms
//   -> start launching mainui

QState *waitScreenLaunchingMainUI = new QState();
machine.addState(waitScreenVisible);
// on entry start launching mainui
// once ok, next state

QState *mainuiRunning = new QState();
machine.addState(mainuiRunning);
// transition: when app launch requested
// transition: when main ui dead (watchdog)

QState *waitScreenLaunchingApp = new QState();
machine.addState(waitScreenLaunchingApp);
// ontimeout?? -> show mainui
// once ok transition

QState *appRunning = new QState();
machine.addState(waitScreenLaunchingApp);
// on exit requested
// on force exist requested
// died (watchdog)
// ping failed several times, stuck? -> kill

machine.start();

// TODO: how to move from initial state
// TODO: qobject parents
// TODO: inherit QStateMachine or wrap it? Factory to build it?

// TODO: unit testing

// Inherit QState

*/

// TODO: piece of code how to launch an executable
// TODO: piece of code to signal PAUSE and continue
// TODO: piece of code kill app (forcing going to menu)
// TODO: piece of code for monitoring an app, if it is live (WatchDog)

// TODO: waiting app could be used to show error messages
//     - could also so reasons why transitions occurred
