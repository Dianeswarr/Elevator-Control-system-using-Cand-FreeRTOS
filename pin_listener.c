/**
 * Program skeleton for the course "Programming embedded systems"
 *
 * Lab 1: the elevator control system
 */

/**
 * Functions listening for changes of specified pins
 */

#include "FreeRTOS.h"
#include "task.h"
#include "pin_listener.h"
#include "assert.h"

/* counter for GPIO pins, for press and release events */
int temp = 0;

static void pollPin(PinListener * listener,
  xQueueHandle pinEventQueue) {

  listener -> pin_status = GPIO_ReadInputDataBit(listener -> gpio, listener -> pin); // get the  status off the pin 

  if (listener -> pin_status != listener -> status) {
    listener -> counter = listener -> counter + 1;
    if (listener -> counter > 1) {
      if (listener -> pin_status == 1) { // debouncing 2 times
        xQueueSend(pinEventQueue, & (listener -> risingEvent), portMAX_DELAY); //sending rising event 
      } else if (listener -> pin_status == 0) {
        xQueueSend(pinEventQueue, & (listener -> fallingEvent), portMAX_DELAY); // send falling event 
      }
      listener -> status = listener -> pin_status;
      listener -> counter = 0;

    }

  }

}

static void pollPinsTask(void * params) {
  PinListenerSet listeners = * ((PinListenerSet * ) params);
  portTickType xLastWakeTime;
  int i;

  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    for (i = 0; i < listeners.num; ++i)
      pollPin(listeners.listeners + i, listeners.pinEventQueue);

    vTaskDelayUntil( & xLastWakeTime, listeners.pollingPeriod);
  }
}

void setupPinListeners(PinListenerSet * listenerSet) {
  portBASE_TYPE res;

  res = xTaskCreate(pollPinsTask, "pin polling",
    100, (void * ) listenerSet,
    listenerSet -> uxPriority, NULL);
  assert(res == pdTRUE);
}