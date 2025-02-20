# ArduinoDigitalClock <br/>
<br/>
Components: <br/>
Arduino Uno R3 <br/>
Adafruit 32x16 Led-Matrix <br/>
RTC DS3231 Chip <br/>
<br/>
Used Librarys:<br/>
https://github.com/Naguissa/uRTCLib/tree/master <br/>
https://github.com/adafruit/RGB-matrix-Panel <br/>
<br/>
Features: <br/>
Tracks daylight saving time <br/>
Changes Font color and doodles based on time of day <br/>
Displays temperature based on RTC Chip <br/>
<br/>
Fonts/doodles hardcoded, mapping them into any data structure overflows the Uno R3 memory on runtime
