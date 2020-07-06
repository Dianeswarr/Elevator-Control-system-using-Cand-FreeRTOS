#include "FreeRTOS.h"

#include "task.h"

#include <stdio.h>

#include "stm32f10x_tim.h"

#include "global.h"

#include "planner.h"

#include "assert.h"

#include <limits.h>

#define FLOOR_1 0
#define FLOOR_2 400
#define FLOOR_3 800
#define MOTOR_UPWARD   (TIM3->CCR1)
#define MOTOR_DOWNWARD (TIM3->CCR2)
#define MOTOR_STOPPED  (!MOTOR_UPWARD && !MOTOR_DOWNWARD)
#define CAPACITY 3


/**
 * Global queue declaration.
 */
int z;
int queue[CAPACITY];
unsigned int size = 0;
unsigned int rear = CAPACITY - 1; // Initally assumed that rear is at end
unsigned int front = 0;

/* Function declaration for various operations on queue */
int enqueue(int data);
int dequeue();
int isFull();
int isEmpty();
int enqueue(int data) {
  int i;

  for (i = 0; i < CAPACITY; i++) {
    if (queue[i] == data) {
      return -1;
    }
  }
  // Queue is full throw Queue out of capacity error.
  if (isFull()) {
    return 0;
  }

  // Ensure rear never crosses array bounds
  rear = (rear + 1) % CAPACITY;

  // Increment queue size
  size++;

  // Enqueue new element to queue
  queue[rear] = data;

  // Successfully enqueued element to queue
  return 1;
}
int dequeue() {
  int data;

  // Queue is empty, throw Queue underflow error
  if (isEmpty()) {
    return 0;
  }

  // Dequeue element from queue
  data = queue[front];
  queue[front] = 0;
  // Ensure front never crosses array bounds
  front = (front + 1) % CAPACITY;

  // Decrease queue size
  size--;

  return data;
}
int isFull() {
  return (size == CAPACITY);
}
int isEmpty() {
  return (size == 0);
}
int x = 0;
int Lift_height = 0;
int Lift_height1 = 0;
int z;

int Calculate_Currentfloor(void) // calculating the current floor
{
  Lift_height = getCarPosition();
  Lift_height1 = (Lift_height / 399) + 1;
  return Lift_height1;
}

//
static void plannerTask(void * params) {
  int Current_floor = 0;
  int y = 0;
  PinEvent key;
  for (;;) {
    xQueueReceive(pinEventQueue, & key, 2000 * portTICK_RATE_MS); // events received from pinlistener

    switch (key) {
    case TO_FLOOR_1:
      printf(" floor 1-Pressed \n");
      x = 1;
      enqueue(x);

      break;
    case TO_FLOOR_2:
      printf("floor 2-Pressed \n");
      x = 2;
      enqueue(x);

      break;
    case TO_FLOOR_3:
      printf("floor 3-Pressed \n");
      x = 3;
      enqueue(x);
      break;
    case STOP_PRESSED:
      setCarMotorStopped(1);
      printf("Stop-pressed \n");
      break;
    case STOP_RELEASED:
      setCarMotorStopped(0);
      printf("Stop- released \n");
      break;
    case ARRIVED_AT_FLOOR:
      printf("AT floor");

      break;
    case LEFT_FLOOR:

      printf(" Between the floor  \n");
      break;
    case DOORS_CLOSED:
      printf(" Doors are  closed  \n");
      break;
    case DOORS_OPENING:
      printf(" Doors are opening  \n");
      break;
    case UNASSIGNED:
      printf("floor 1/2/3 is released \n");
      break;

    }
    Current_floor = Calculate_Currentfloor();
    if ((Current_floor == y && getCarPosition() < 2 && getCarPosition() >= 0) || (Current_floor == y && getCarPosition() < 402 && getCarPosition() >= 399) || (Current_floor == y && getCarPosition() <= 800 && getCarPosition() >= 798)) {
      vTaskDelay(3000 / portTICK_RATE_MS); // waits for 3 sec if it has reached the desired destination
      y = dequeue();
    }
    if (y == 0) {
      y = Current_floor;
    }

    if (y == 1) {
      setCarTargetPosition(FLOOR_1);
    } else if (y == 2) {
      setCarTargetPosition(FLOOR_2);

    } else if (y == 3) {
      setCarTargetPosition(FLOOR_3);

    }

  }

}

void setupPlanner(unsigned portBASE_TYPE uxPriority) {
  xTaskCreate(plannerTask, "planner", 100, NULL, uxPriority, NULL);
}