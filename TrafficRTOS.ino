#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>


// LEDs pinout
int signal1[] = {26, 25, 33};
int signal2[] = {32, 3, 21};
int signal3[] = {17, 16, 15};
int signal4[] = {19, 18, 5};

// Ultrasonic pinout. First trigger-echo pair corresponds to first sensor and so on.
int triggerPins[] = {13, 27, 22, 2};
int echoPins[] = {12, 14, 23, 4};


// Queue and Task Handler initialization
QueueHandle_t distanceQueue;

TaskHandle_t sensorTaskHandle;
TaskHandle_t trafficTaskHandle;
TaskHandle_t pedestrianTaskHandle;

// Threshold parameter for the distane calculated by the ultrasonic sensor.
int t = 20;


// Initialize the traffic signals as OUTPUT (to write values to LEDs)
void initTrafficSignals() {
  for (int i = 0; i < 3; i++) {
    pinMode(signal1[i], OUTPUT);
    pinMode(signal2[i], OUTPUT);
    pinMode(signal3[i], OUTPUT);
    pinMode(signal4[i], OUTPUT);
  }
}

// Initialize trigger pins as output and echo pins as input (Tx Rx of ultrasonic)
void initUltrasonicSensors() {
  for (int i = 0; i < 4; i++) {
    pinMode(triggerPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
  }
}

// Ultrasonic Sensor Task to recieve distance in queue. Send a 10ms pulse and calculate time duration and convert it to cm using speed of sound.
// Store distance in a Queue and send to trafficTask
void ultrasonicTask(void *param) {
  int distances[4];
  while (1) {
    for (int i = 0; i < 4; i++) {
      digitalWrite(triggerPins[i], LOW);
      delayMicroseconds(2);
      digitalWrite(triggerPins[i], HIGH);
      delayMicroseconds(10);
      digitalWrite(triggerPins[i], LOW);
      long duration = pulseIn(echoPins[i], HIGH);
      distances[i] = duration * 0.034 / 2;
    }
    xQueueSend(distanceQueue, distances, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// Traffic Light Control Task
void trafficTask(void *param) {
  int distances[4];
  while (1) {
    // If queue recieved from ultrasonic task
    if (xQueueReceive(distanceQueue, distances, portMAX_DELAY)) {
      int activeSignals = 0;
      int activeIndexes[4] = {0};
 
      // activeIndexes is an array containing those signal IDs (0 for signal 1, 1 for signal 2 and so on..) for which traffic is present. E.g. activeIndexes if traffic at 1 and 2 is
      // {1 2 0 0}. If traffic at 0 2 3 then active indexes = {0 2 3 0}
      //activeSignals is number of signals actually active. In first case this value is 2 and second case value is 3
      for (int i = 0; i < 4; i++) {
        if (distances[i] < t) {
          activeIndexes[activeSignals++] = i;
        }
      }
      Serial.print("Active Signals: ");
      for(int i = 0; i < activeSignals; i ++){
        //if first ultrasonic sensor active, print 1 and so on. print nothing if no sensor is sensing distance below threshold.
        Serial.print(activeIndexes[i] + 1);
        Serial.print(" ");
      }
      Serial.println();
      // If only one signal active, then proceed to controlSignalalways
      if (activeSignals == 1) {
        controlSignalalways(getSignalPins(activeIndexes[0]));
      //If more than one signal active, then proceed to controlSignal scheduled for each signal separately using iterator 'i'
      } else if (activeSignals > 1) {
        for (int i = 0; i < activeSignals; i++) {
          controlSignal(getSignalPins(activeIndexes[i]));
        }
      // If no signal present, reset to all Red signals
      } else if (activeSignals == 0) {
        reset();
      }
      delay(1000);
    }
  }
}

// To index the signals
int *getSignalPins(int index) {
  if (index == 0) return signal1;
  if (index == 1) return signal2;
  if (index == 2) return signal3;
  if (index == 3) return signal4;
  return nullptr;
}

// controlSignal is used when there is traffic on more than one signal so we need scheduling. It has one parameter signalPins which corresponds to any one
// of the signals with traffic
void controlSignal(int signalPins[]) {
  // Suspend ultrasonic task while scheduling is being processed so that during scheduling time, new distance is not read. Otherwise, this distance triggers new and
  // unneccesary controlSignal task after previous one has finished. 
  vTaskSuspend(sensorTaskHandle);
  // set all leds to LOW.
  lowAll();
  // set only red leds to HIGH.
  reset();
  // algo for Red-Yellow-Green with suitable and adjustable delay.
  digitalWrite(signalPins[0], HIGH);
  delay(1000);
  digitalWrite(signalPins[0], LOW);
  digitalWrite(signalPins[1], HIGH);
  delay(1000);
  digitalWrite(signalPins[1], LOW);
  digitalWrite(signalPins[2], HIGH);
  delay(1000);
  digitalWrite(signalPins[2], LOW);

  //resume ultrasonic sensor task after scheduling is complete.
  vTaskResume(sensorTaskHandle);
}

//controlSignalalways is used when there is only traffic at one signal. It has parameter signalPins which is index corresponding to the signal with traffic
void controlSignalalways(int signalPins[]) {
  // suspend ultrasonic task
  vTaskSuspend(sensorTaskHandle);
  // reset leds to only red HIGH.
  reset();
  // set red of given signal LOW
  digitalWrite(signalPins[0], LOW);
  // set green LED of given signal HIGH
  digitalWrite(signalPins[2], HIGH);
  vTaskResume(sensorTaskHandle);
}

// set all leds to LOW
void lowAll() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(signal1[i], LOW);
    digitalWrite(signal2[i], LOW);
    digitalWrite(signal3[i], LOW);
    digitalWrite(signal4[i], LOW);
  }
}

// only set red leds HIGH
void reset() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(signal1[0], HIGH);
    digitalWrite(signal1[1], LOW);
    digitalWrite(signal1[2], LOW);
    digitalWrite(signal2[0], HIGH);
    digitalWrite(signal2[1], LOW);
    digitalWrite(signal2[2], LOW);
    digitalWrite(signal3[0], HIGH);
    digitalWrite(signal3[1], LOW);
    digitalWrite(signal3[2], LOW);
    digitalWrite(signal4[0], HIGH);
    digitalWrite(signal4[1], LOW);
    digitalWrite(signal4[2], LOW);
  }
}

//setup function
void setup() {
  

  Serial.begin(115200);
  //initialization functions
  initTrafficSignals();
  initUltrasonicSensors();
  // initialze distance queue
  distanceQueue = xQueueCreate(4, sizeof(int[4]));
  if (distanceQueue == NULL) {
    while (1);
  }
  //create tasks
  xTaskCreate(ultrasonicTask, "UltrasonicTask", 2048, NULL, 1, &sensorTaskHandle);
  xTaskCreate(trafficTask, "TrafficTask", 2048, NULL, 1, &trafficTaskHandle);
}

void loop() {}