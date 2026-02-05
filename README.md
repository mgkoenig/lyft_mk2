# LYFT mk2

This is an advanced next-level version of the famous [Lyft](https://github.com/mgkoenig/lyft) project. Apart from general improvements (lessons learned from the previous version) the focus of this re-design was placed on versatility and flexibility. Therefore, a dual-controller architecture was chosen. One controller is responsible for the desk communication only, while the second controller (__ESP32__) acts as interface platform with its enormous possibilities. Lyft can now be extended to support various access options, such as _web interfaces_ or _mobile apps_. Maybe your desk is one of the few things missing in your _home assistant_ setup? With the built-in ESP controller (running MicroPython), these and many more options are right at your fingertips. 


