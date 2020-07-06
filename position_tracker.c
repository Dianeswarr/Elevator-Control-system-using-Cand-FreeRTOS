/**
 * Program skeleton for the course "Programming embedded systems"
 *
 * Lab 1: the elevator control system
 */

/**
 * Class for keeping track of the car position.
 */

#include "FreeRTOS.h"
#include "task.h"

#include "position_tracker.h"

#include "assert.h"

static void positionTrackerTask(PositionTracker * tracker) {
	u8 previousEdge = 0;
	u8 presentEdge = 0;
    portTickType xLastWakeTime;
  
    xLastWakeTime = xTaskGetTickCount();
	
   for (;;) {
	     presentEdge = GPIO_ReadInputDataBit(tracker->gpio, tracker->pin);  // pin 9 reading
	     if (previousEdge == 0 && presentEdge != previousEdge) {    // considering only raising edge.
		 
           if(xSemaphoreTake(tracker->lock, portMAX_DELAY))
		        {
					     if (tracker->direction == Up) {
						       tracker->position =tracker->position+1;       // incrementing upwards     
					  }
						
					 
					     else if (tracker->direction == Down) {
						       tracker->position =tracker->position-1;      // decrementing downwards
					                                          }
						  
					 
					     else if (tracker->position == Unknown) {
						       tracker->position = tracker->position;
					                                            }
						 					 
					 xSemaphoreGive(tracker->lock);
		        }
		  		  
		      else printf("No semaphore taken\n");
				    
	                                                           }
		
		previousEdge = presentEdge;
		
	    vTaskDelayUntil(&xLastWakeTime, tracker->pollingPeriod); 
  }
 

  vTaskDelay(portMAX_DELAY);

}

void setupPositionTracker(PositionTracker *tracker,
                          GPIO_TypeDef * gpio, u16 pin,
						  portTickType pollingPeriod,
						  unsigned portBASE_TYPE uxPriority) {
  portBASE_TYPE res;

  tracker->position = 0;
  tracker->lock = xSemaphoreCreateMutex();
  assert(tracker->lock != NULL);
  tracker->direction = Unknown;
  tracker->gpio = gpio;
  tracker->pin = pin;
  tracker->pollingPeriod = pollingPeriod;

  res = xTaskCreate(positionTrackerTask, "position tracker",
                    80, (void*)tracker, uxPriority, NULL);
  assert(res == pdTRUE);
}

void setDirection(PositionTracker *tracker, Direction dir) {
xSemaphoreTake(tracker->lock, portMAX_DELAY);        // locking semaphore
	tracker->direction = dir;         //updating the direction
   xSemaphoreGive(tracker->lock);                     // unlocking semaphore


}

s32 getPosition(PositionTracker *tracker) {
	vs32 pos;
  
  xSemaphoreTake(tracker->lock, portMAX_DELAY);             // locking semaphore
  pos = tracker->position;                   //updating the position
  xSemaphoreGive(tracker->lock);                                // unlocking semaphore
 

  return pos;

}

