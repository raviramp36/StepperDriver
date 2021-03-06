/*
 * Multi-motor group driver
 *
 * Copyright (C)2017 Laurentiu Badea
 *
 * This file may be redistributed under the terms of the MIT license.
 * A copy of this license has been included with this distribution in the file LICENSE.
 */
#include "MultiDriver.h"

#define FOREACH_MOTOR(action) for (short i=count-1; i >= 0; i--){action;}

/*
 * Initialize motor parameters
 */
void MultiDriver::startMove(long steps1, long steps2, long steps3){
    long steps[3] = {steps1, steps2, steps3};
    /*
     * Initialize state for all active motors
     */
    FOREACH_MOTOR(
        if (steps[i]){
            motors[i]->startMove(steps[i]);
            event_timers[i] = 0;
        } else {
            event_timers[i] = -1;
        }
    );
    ready = false;
}
/*
 * Trigger next step action
 */
long MultiDriver::nextAction(void){
    static unsigned long next_action_time = 0;
    long next_action_interval = 0;

    microWaitUntil(next_action_time);
    
    // Trigger all the motors that need it (event timer = 0)
    FOREACH_MOTOR(
        if (event_timers[i] == 0){
            event_timers[i] = motors[i]->nextAction();
        }
    );
    // Find the time when the next pulse needs to fire
    // this is the smallest non-zero timer value from all active motors
    ready = true;
    FOREACH_MOTOR(
        if (event_timers[i] > 0){
            ready = false;
            if (event_timers[i] < next_action_interval || next_action_interval == 0){
                next_action_interval = event_timers[i];
            }
        }
    );
    // Reduce all event timers by the current left time so 0 marks next
    FOREACH_MOTOR(
        if (event_timers[i] > 0){
            event_timers[i] -= next_action_interval;
        }
    );
    next_action_time = micros() + next_action_interval;
    return next_action_interval;
}
/*
 * Optionally, call this to begin braking to stop early
 */
void MultiDriver::startBrake(void){
    FOREACH_MOTOR(
        if (event_timers[i] >= 0){
            motors[i]->startBrake();
        }
    )
}
/*
 * State querying
 */
bool MultiDriver::isRunning(void){
    bool running = false;
    FOREACH_MOTOR(
        if (motors[i]->getCurrentState() != Motor::STOPPED){
            running = true;
            break;
        }
    )
    return running;
}

/*
 * Move each motor the requested number of steps, in parallel
 * positive to move forward, negative to reverse, 0 to remain still
 */
void MultiDriver::move(long steps1, long steps2, long steps3){
    unsigned long next_event;
    startMove(steps1, steps2, steps3);
    while (!ready){
        next_event = nextAction();
    }
}

#define CALC_STEPS(i, deg) ((motors[i] && deg) ? motors[i]->calcStepsForRotation(deg) : 0)
void MultiDriver::rotate(long deg1, long deg2, long deg3){
    move(CALC_STEPS(0, deg1), CALC_STEPS(1, deg2), CALC_STEPS(2, deg3));
}

void MultiDriver::rotate(double deg1, double deg2, double deg3){
    move(CALC_STEPS(0, deg1), CALC_STEPS(1, deg2), CALC_STEPS(2, deg3));
}

void MultiDriver::startRotate(long deg1, long deg2, long deg3){
    startMove(CALC_STEPS(0, deg1), CALC_STEPS(1, deg2), CALC_STEPS(2, deg3));
}

void MultiDriver::startRotate(double deg1, double deg2, double deg3){
    startMove(CALC_STEPS(0, deg1), CALC_STEPS(1, deg2), CALC_STEPS(2, deg3));
}

void MultiDriver::setMicrostep(unsigned microsteps){
    FOREACH_MOTOR(motors[i]->setMicrostep(microsteps));
}

void MultiDriver::enable(void){
    FOREACH_MOTOR(motors[i]->enable());
}
void MultiDriver::disable(void){
    FOREACH_MOTOR(motors[i]->disable());
}
