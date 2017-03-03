# Boltwood CS || protocol
================

## Sensor as receiver

* 0x01 - REQUEST_POLL - say that there's someone waiting for data
* 0x02 - FRAME_START  - data frame start (ending with '\n')

Without getting REQUEST_POLL, sensor will send data for 5 times and sleep.

All given symbols go after FRAME_START:

* 'a' - ACK (should be sent by computer after each good data portion)
* 'm' - next text is command sequence following by CRC16 ('m' not used in CRC)
* 'n' - NACK (should be sent by computer after each bad data portion)

Commands (ending with CRC16\n)

* 'b' - close Roof
* 'c' - get calibration
* 'l' - get wetness calibration
* 't' following by 5 integers - set thresholds
* 't' without data - set default thresholds

After sending command wait a little for getting ACK ('A'). If none (or NACK), try again.
All received data will be sent back in 'Q' frame type.

## Sensor as sender

* 0x02 - FRAME_START
* '!'  - PURGE_START
* '|'  - MEMORY_DUMP_START
* '^'  - BOOT_LOADER_START

All given commands go after FRAME_START:

* 'A' - ACK
* 'M' - report data frame
* 'N' - NACK
* 'P' - poll request (before FRAME_START there's one 0x00 byte)

### Report data frame types

* 'C' - termopile calibration
* 'D' - **sensor report**
* 'I' - reset
* 'K' - wetness calibration
* 'Q' - report a command received from the PC
* 'R' - roof close
* 'T' - thresholds
* 'W' - wetness data
>> debugging???
* 'X' - "X rhr %d %f", rhr, rainheatt (control of the rain heater)
* 'Y' - "Y %f %d %d", rainheatt, suppressrh, suppressrrh (direct setting of the rain heater target temperature)
* 'Z' - "Z %d %d %d", rainheatt, suppressrh, suppressrrh (direct setting of the rain heater PWM hardware value)

## Data parameters and fields

### Set thresholds ("t 1 2 3 4 5")
(integer values)

1. cloudy threshold *10
1. very cloudy threshold *10
1. windy threshold *10
1. rain threshold (equal)
1. wet threshold (equal)

### Termopile calibration ('C')
    "C %d %lf %lf %lf"
1. eThermopileCal (1)
1. eBestK (-6.571e-7)
1. eBestD (0.371)
1. eBestOffs (-2.00)
1.  ? (10.1)

### Sensor report ('D')
    D %u %u %u %u %u %u %lf %lf %lf %c %c %d %lf %lf %d %lf %d %lf %lf %d %d %d %d %d %u %u %u %u %u %u %lf %d %d %u %u"
1. humidstatTempCode - humidity and ambient temperature sensor code (0)
>   0 = OK;
    1 - write failure for humidity;
    2 - measurement never finished for humidity;
    3 - write failure for ambient;
    4 - measurement never finished for ambient;
    5 - data line was not high for humidity;
    6 - data line was not high for ambient.
1. cloudCond - cloud conditions (3)
>  (dT = skyMinusAmbientTemperature)
    0 - unknown;
    1 - clear (dT > eCloudyThresh);
    2 - cloudy (dT <= eCloudyThresh); 3 - very cloudy (dT <= eVeryCloudyThresh)
1. windCond - wind conditions (1)
>   0 - unknown (e.g., sensor wet);
    1 - ok (windSpeed < eWindyThresh);
    2 - windy (windSpeed >= eWindyThresh);
    3 - very windy (windSpeed >= eVeryWindyThresh).
1. rainCond - rain sensor conditions (1)
>   0 - unknown;
    1 - not raining;
    2 - recently raining;
    3 - raining.
1. skyCond - sky sensor conditions (3)
>   0 - unknown;
    1 - clear;
    2 - cloudy;
    3 - very cloudy;
    4 - wet.
1. roofCloseRequested =0 normally, =1 if roof close was requested on this cycle (1)
1. skyMinusAmbientTemperature - Tsky-Tamb (-3.1)
>   Tsky: 999.9 saturated hot;
    -999.9 saturated cold;
    -998.0 if sensor is wet.
1. ambientTemperature - ambient temperature (29.3)
>   if -40 and humidity is 0 there is a problem with communication to those sensors
    (likely water where it shouldn't be).
1. windSpeed - wind speed (0.0)
> -1 - still heating; -2 - wet; -4 - not heating; -5 - shortened circuit; -6 - no probe
1. wetSensor - wet sensor value (N)
> 'N' when dry, 'W' when wet now, 'w' when wet in last minute
1. rainSensor - rain sensor value (N)
> 'N' when no rain, 'R' when rain drops hit on this cycle, 'r' for drops in last
    minute
1. relativeHumidityPercentage - relative humidity in % (22)
1. dewPointTemperature - dew point temperature (5.2)
1. caseTemperature -  thermopile case temperature(41.2)
> 999.9 saturated hot, -99.9 saturated cold
1. rainHeaterPercentage - PWM percentage for rain heater (0)
1. blackBodyTemperature - calibration black body temperature (factory only) (-99.9)
>   999.9 - saturated hot,
    -99.9 - saturated cold.
1. rainHeaterState - state of rain sensor heater (0)
>   0 if too hot;
    1 if at or nearly at requested temp;
    2..5 if too cold;
    6 if cannot control due to a saturated case temperature (causes shutdown);
    7 is used by firmware (tmain) to indicate that normal control is being used instead of direct
        use of this.
1. powerVoltage -  voltage actually on the +24V line at the sensor head (24.4)
1. anemometerTemeratureDiff - anemometer tip temperature difference from ambient, limited by reducing
    anemometer heater power when 25^o C is reached (24.6)
1. wetnessDrop -  maximum drop in wetness oscillator counts this cycle due to rain drops(3)
1. wetnessAvg - wetness oscillator count difference from base dry value (162)
1. wetnessDry -  wetness oscillator count difference for current dry from base dry value (213)
1. rainHeaterPWM - rain heater PWM value (000)
1. anemometerHeaterPWM - anemometer heater PWM value (039)
1. thermopileADC -  thermopile raw A/D output (0106)
1. thermistorADC - thermopile thermistor raw A/D output (0352)
1. powerADC - power supply voltage monitor raw A/D output (0959)
1. blockADC - calibration block thermistor raw A/D output (1023)
1. anemometerThermistorADC - anemometer tip thermistor raw A/D output (0138)
1. davisVaneADC - Davis vane raw A/D output (only for factory calibration) (0141)
1. dkMPH - external anemometer used (only for factory calibration) (0.0)
1. extAnemometerDirection - external anemometer wind direction (only for factory calibration) (049)
1. rawWetnessOsc - raw counts from the wetness oscillator (12990)
1. dayCond - day conditions value (3)
>   3 full daylight;
    2 twilight;
    1 night;
    0 unknown.
1. daylightADC -  daylight photodiode raw A/D output (0502)
>   0 means no light, 1023 max light.

### Reset ('I')
    "I %u %u 0x%x %u 0x%x %u"
1. serialNumber - sensor's serial number
1. version - firmvare version
1. mcucsrv - ???
1. crashCode - ?
1. lastCyc - ?
1. eSendErrs - (error counter?)

### Wetness calibration ('K')
    "K %d %lf %u %lf %lf %u %d %d"

1. eWetCal (1)
1. eWetOscFactor (1.001)
1. eRawWetAvg (13228)
1. eCaseT - wetness sensor case temperature (max?) (30.9)
1. eshtAmbientT - ambient (termopile in bottom part of sensor) temperature (to turn on heater?) (24.4)
1. enomOsc (13209)
1. oscDry (213)
1. minWetAvg (161)


### Thresholds ('T')
    "T %u %u %u %lf %lf %lf %lf %d %d %d %d %d  %d %d %d %d %d"
1. serialNumber - sensor's serial number (00794)
1. version - firmvare version (00062)
1. eSendErrs - (error counter?) (02603)
1. eCloudyThresh - cloudy threshold (-25.0)
1. eVeryCloudyThresh - very cloudy threshold (-10.0)
1. eWindyThresh - windy threshold (15.0)
1. eVeryWindyThresh - very windy threshold (WTF?) (30.0)
1. eRainThresh - rain threshold (12)
1. eWetThresh - wet threshold (100)
1. eDaylightCode - daylight code (1)
1. eDayThresh - day threshold (132)
1. eVeryDayThresh - very much light threshold (220)
1. ? Ty (5)
1. ?SNBot (0)
1. ?SNTop (65535)
1. ?EV (0)
1. ?WCC (1)

### Wetness data ('W')
    "W %lf %lf %u %u %u %u %u"
    41.2 29.3 13157 13266 13134 12990   162

1. caseVal - current case temperature ? (41.2)
1. ambT - current ambient temperature ? (29.3)
1. wAvgW - ?  (13157)
1. wAvgC - ? (13266)
1. nomos - ? (13134)
1. rawWT - ? (12990)
1. wetAvg - ? (162)

