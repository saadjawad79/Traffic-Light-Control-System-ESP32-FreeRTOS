# Traffic Control System using FreeRTOS on ESP32

If traffic at only one light:
- Set signal at that light to green indefinitely, until traffic at that light stops.

If traffic at two lights:
- Schedule normal routine (red-yellow-green) with appropriate delay on both of the lights. i.e. first there will be normal routine on one light, then it will be red, then normal routine on other light.

If traffic at three/four lights:
- Similar scheduling to traffic at two lights scenario
