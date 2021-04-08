# benetherington/heartrate
A quick Arduino sketch using a Polar heartrate receiver to display individual
heartbeats, averaged heartrate, and a heartrate histogram.

## Dependencies
contrem/arduino-timer

## Hardware

* [Adafruit FeatherWing OLED - 128x64 OLED Add-on For Feather - STEMMA QT / Qwiic](https://www.adafruit.com/product/4650)

* [Heart Rate Educational Starter Pack with Polar Wireless Sensors](https://www.adafruit.com/product/1077)

* [Adafruit Feather M0 WiFi - ATSAMD21 + ATWINC1500](https://www.adafruit.com/product/3010) (Though you'd be hard
  pressed to find a Feather that wouldn't work!)

* [3.7v Li-Ion battery](https://www.adafruit.com/product/328) The enclosure is sized for Adafruit's biggest, linked
  here. It's 2500mAh.

## 3D Printed Enclosure
In the CAD folder, you'll find the original FreeCAD files, STLs of the four
parts, and two pre-sliced Gcode files for the Prusa Mk3. You'll also need velcro
cable straps, and an M3 flat-head screw. I used a 10mm long screw to take
advantage of the extra bite at the back of the hole which is printed as an
overhang in this orientation, but a shorter one would work as well. You don't
need much holding power here.

There are a few things to note about these files:
* The bottomcase has mounting posts spaced for the M0 WiFi. They're wider than
  the Feather standard calls for. If you use another, you'll have to modify the
  file. In the left sidebar, navigate to Enclosure > bottomcase > posts002 >
  posts001, and change constraint 4.
* If you don't want to use a screw for the battery door, consider adding tabs to
  the top of the door and the top of the door frame. The tabs holding the
  topcase and bottomcase together are a good example of how this might look.
* This design does not feature easy access to the USB port. I figured protection
  against sweat droplets was a priority. Instead, I spec'd Adafruit's biggest
  JST battery and added sleep capabilities to the Arduino code.
