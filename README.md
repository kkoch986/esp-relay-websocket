# esp-relay-websocket

Firmware for an ESP8266 that connects to a websocket server and can receive messages to toggle the relay on and off.

Meant to be used with something like [Chataigne](https://github.com/benkuper/Chataigne)
to enable cheap, reliable orchestration of relay actuated props.

I've been using platformio environments to manage the compilation of each prop
so i can control the HOSTNAME that it shows up with when it joins the network.

As of now it uses [WifiManager](https://github.com/tzapu/WiFiManager) which is configured to
also collect some extra parameters regarding the websocket host and the "address" that will
be used to trigger the relay.

It expects to receive messages like `<address> <value 0|1>`, address must not contain any spaces
and must match what was set when the device was configured.

It is also built to support a status LED (common anode RGB LED) and a push button which can be used
to force it to reset its Wifi configuration in the event changes need to be made
(by holding it down for ~3 seconds until the light starts blinking red).


## Note about he BluetoothSerial.h error

I tend to get an error that `BluetoothSerial.h` cannot be found.
Since im not using it anyway, i just delete that file (see the `preclean` make target).

