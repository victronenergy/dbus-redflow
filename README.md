dbus-redflow
============
This application reads data from Redflow ZBM batteries, and publishes the data on the D-Bus. It is designed to run on the color control CX.

Architecture
============
The application consists of 3 layers:
* Data acquisition layer: the _BatteryUpdater_ classe retrieves data from known batteries over Modbus RTU. The _DeviceScanner_ class find batteries.
* Data model: _BatteryControl_ represents all battery data. _BatterySummary_ computes some statistics for the entire battery bank
* D-Bus layer: _BatterySummaryBridge_ pushes data from _BatterySummary_ to the D-Bus, and _BatteryControllerBridge_ does the same with _BatteryController_.
