/**
 * Waiter class provides utility for processing Qt event loop messages while
 * waiting certain condition to fullfill. Typical use case in unit tests is
 * wait certain signal to be triggered.
 *
 * Waiter has checking interval how often it checks a condition.
 *
 * Typical example in unit tests:
 *     bool stopped = false;
 *     QObject::connect(&app, &ApplicationController::stopped, [&] () { stopped = true; });
 *
 *     app.launch();
 *     Waiter::wait([&] () { return stopped; });
 *     EXPECT_TRUE(launched);
 *
 * Note that while Waiter checks condition it doesn't fail test if waiting
 * timeouts. You need to use standard unit testing frameworks mechamnisms.
 *
 */

#ifndef WAITER_H
#define WAITER_H

#include <QCoreApplication>
#include <QDebug>
#include <QTime>
#include <QThread>
#include <functional>

#include <gtest/gtest.h>

// for understanding lambdas check
// http://www.cprogramming.com/c++11/c++11-lambda-closures.html

static const int DEFAULT_TIMEOUT_MS = 100;
static const int DEFAULT_WAITING_STEP_MS = 10;


class Waiter
{
//
public:
    Waiter(std::function<bool ()> func,
           int timeout_ms=DEFAULT_TIMEOUT_MS,
           int waitingStep_ms = DEFAULT_WAITING_STEP_MS) :
        _maxTime(timeout_ms),
        _waitingStep(waitingStep_ms),
        _minLoops(timeout_ms / DEFAULT_TIMEOUT_MS),
        _loopCounter(0),
        _startTime(NULL),
        _func(func)
    {
    }

    ~Waiter()
    {
        if (_startTime)
            delete _startTime;
    }

    void wait(bool debug = false)
    {
        _startTime = new QTime();
        _startTime->start();

        if (debug) {
            qDebug() << "Starting waiting: condition =" << _func() << ", timeout =" << _maxTime;

        }

        // loop counting is necessary for debugging cases because if break
        // point has been hit then time passes too quickly
        while ( ( (_startTime->elapsed() < _maxTime) || (_loopCounter < _minLoops) ) && !_func()  )
        {
            QCoreApplication::processEvents();
            QThread::msleep(_waitingStep);
            _loopCounter++;
        }
        if (debug)
            qDebug() << "Waited" << _loopCounter << "loops, condition =" << _func();

        // TODO: record reason of getting out of loop
        // TODO: util to evaluate func() (for asserts)
        // TODO: debug printing what happens
    }

    void resetAndWait()
    {
        _loopCounter = 0;
        if (_startTime)
        {
            delete _startTime;
            _startTime = NULL;
        }
        wait();
    }

    static void wait(std::function<bool ()> func,
                     bool debug=false,
                     int timeout_ms=DEFAULT_TIMEOUT_MS,
                     int waitingStep_ms=DEFAULT_WAITING_STEP_MS)
    {
        Waiter waiter(func, timeout_ms, waitingStep_ms);
        waiter.wait(debug);
    }

    // waits conditions and finally assert it is true
    static void waitAndAssert(std::function<bool ()> func,
                     bool debug=false,
                     int timeout_ms=DEFAULT_TIMEOUT_MS,
                     int waitingStep_ms=DEFAULT_WAITING_STEP_MS)
    {
        Waiter waiter(func, timeout_ms, waitingStep_ms);
        waiter.wait(debug);
        ASSERT_TRUE(func());
    }

private:
    int _maxTime;
    int _waitingStep;
    int _minLoops;
    int _loopCounter; // debug information
    QTime* _startTime;
    std::function<bool ()> _func;

};

#define WAIT(condition) Waiter::wait([&] () { return condition; } );
#define WAIT_WITH_TIMEOUT(condition, timeout) Waiter::wait([&] () { return condition; }, true, timeout );
#define WAIT_AND_ASSERT(condition) Waiter::wait([&] () { return condition; } ); ASSERT_TRUE(condition);
#define WAIT_CUSTOM_AND_ASSERT(condition, timeout, step) Waiter::wait([&] () { return condition; }, true, timeout, step ); ASSERT_TRUE(condition);


#endif // WAITER_H
