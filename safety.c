#include "FreeRTOS.h"

#include "task.h"

#include "stm32f10x_tim.h"

#include "stm32f10x_gpio.h"

#include <stdio.h>

#include "global.h"

#include "assert.h"

#define POLL_TIME (10 / portTICK_RATE_MS)

#define MOTOR_UPWARD   (TIM3->CCR1)
#define MOTOR_DOWNWARD (TIM3->CCR2)
#define MOTOR_STOPPED  (!MOTOR_UPWARD && !MOTOR_DOWNWARD)

#define STOP_PRESSED  GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3)
#define AT_FLOOR      GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7)
#define DOORS_CLOSED  GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8)



static portTickType xLastWakeTime;
s32 Lift_position;

static void check(u8 assertion, char * name) {
  if (!assertion) {
    printf("SAFETY REQUIREMENT %s VIOLATED: STOPPING ELEVATOR\n", name);
    for (;;) {
      setCarMotorStopped(1);
      vTaskDelayUntil( & xLastWakeTime, POLL_TIME);
    }
  }
}

static void safetyTask(void * params) {
  s16 DoorOpenTime = -1;
  s16 timeSinceStopPressed = -1;
  s16 motor_moving_time = -1;
  s16 Atfloor_sec = 0;
  s16 stoptime = 0;
  s32 current_position1 = 0;
  s32 previous_position1 = 0;
  s32 speed;
  s32 Motor_moving = 0;
  s32 Motor_direction;
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    // Environment assumption 1: the doors can only be opened if
    //                           the elevator is at a floor and
    //                           the motor is not active

    check((AT_FLOOR && MOTOR_STOPPED) || DOORS_CLOSED,
      "env1");

    // fill in environment assumption 2: The elevator moves at a maximum speed of 50cm/s
    if (!MOTOR_STOPPED) {
      Motor_moving = 1;
      if (motor_moving_time < 0) {
        motor_moving_time = 0;
      } else
        motor_moving_time = motor_moving_time + POLL_TIME;
      if (motor_moving_time * portTICK_RATE_MS >= 1000) {
        current_position1 = getCarPosition();
        speed = (current_position1 - previous_position1);
        check(speed <= 50 && speed >= -50, "env2");
        previous_position1 = current_position1;
        motor_moving_time = 0;
      }
    }
    if (MOTOR_STOPPED) {
      motor_moving_time = -1;
      previous_position1 = getCarPosition();
      Motor_moving = 0;
    }

    // fill in environment assumption 3
    Lift_position = getCarPosition();
    check(Lift_position <= 1 || (Lift_position >= 399 && Lift_position <= 401) || Lift_position >= 799 || !AT_FLOOR, "env3");

    // fill in your own environment assumption 4: The lift should not change its direction when it is between the floor, it can change direction when it reaches the floor.
    if (MOTOR_UPWARD) // current motor direction
    {
      check(Motor_direction == 1 || Motor_direction == 3, "env4");
    }
    if (MOTOR_DOWNWARD) // current motor direction
    {
      check(Motor_direction == 2 || Motor_direction == 3, "env4");
    }

    if (MOTOR_UPWARD) {
      Motor_direction = 1; // previous value(state) is stored
    } else if (MOTOR_DOWNWARD) {

      Motor_direction = 2; // previous value(state) is stored
    } else if (AT_FLOOR) {
      Motor_direction = 3; // previous value(state) is stored
    } else {
      Motor_direction = Motor_direction;
    }

    // System requirement 1: if the stop button is pressed, the motor is
    //                       stopped within 1s

    if (STOP_PRESSED) {
      if (timeSinceStopPressed < 0)
        timeSinceStopPressed = 0;
      else

        timeSinceStopPressed += POLL_TIME;
      check(timeSinceStopPressed * portTICK_RATE_MS <= 1000 || MOTOR_STOPPED,
        "req1");
      timeSinceStopPressed = 0;

    } else {
      timeSinceStopPressed = -1;
    }

    // System requirement 2: the motor signals for upwards and downwards
    //                       movement are not active at the same time

    check(!MOTOR_UPWARD || !MOTOR_DOWNWARD,
      "req2");

    // fill in safety requirement 3

    check(Lift_position >= 0 && Lift_position <= 800, "req3"); // range of the elevator only from 0 to 800

    // fill in safety requirement 4  

    if (Motor_moving && MOTOR_STOPPED) // "Motor_moving" is the previous value( previous state) of whether the motor is moving or not
    {
      check((STOP_PRESSED || AT_FLOOR), "req4");
    }
    // fill in safety requirement 5
    if (MOTOR_STOPPED && AT_FLOOR)

    {
      Atfloor_sec = Atfloor_sec + POLL_TIME;
    } else {
      check(Atfloor_sec == 0 || Atfloor_sec * portTICK_RATE_MS >= 1000, "req5"); // checks whether lift is atleast waiting for 1s.
      Atfloor_sec = 0;
    }



    //  fill in safety requirement 7: The lift should never stop anywhere more than 2 minutes.
    if (MOTOR_STOPPED) {
      if (stoptime < 0)
        stoptime = 0;
      else
        stoptime += POLL_TIME;
      check(stoptime * portTICK_RATE_MS <= 120000, "req7");
    } else {
      stoptime = -1;
    }
    vTaskDelayUntil( & xLastWakeTime, POLL_TIME);
  }
}

void setupSafety(unsigned portBASE_TYPE uxPriority) {
  xTaskCreate(safetyTask, "safety", 100, NULL, uxPriority, NULL);
}