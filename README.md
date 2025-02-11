# Matrix Clock (and more) using WS2812b LEDs

![all_modes](https://user-images.githubusercontent.com/55962781/113493423-2a029f00-94ad-11eb-8f39-43af33dc5daa.jpg)

**NOTE**: A newer ESP32-based version with more features that does not require Home Assistant can be found here: [MatrixClock ESP-32](https://github.com/Resinchem/Matrix-Clock-ESP32)

Updated version of the original 3D LED Clock, countdown timer, scoreboard and new text display mode, using 400 WS2811b pixels.

## Key Features:
* Multiple large number 'fonts': original 7-segment, modern or hybrid
* Temperature can display indoor or outdoor temperature (via MQTT/External service) in F or C
* Scoreboard supports 3 character "team names" (e.g. DET, BOS, IND, - or any 3 characters you define).
* User controllable colors for all display modes
* Text display supports almost all ASCII characters from 32-122 (except for a couple reserved characters)
* Text display effects, including flash, flash alternate, fade in/out, appear/appear flash and rainbow
* Control via 30+ MQTT commands, local buttons or built-in web browser

(While basic functionality can be controlled via local buttons or the built-in web server, most of the advanced features including the text display mode requires a properly configured MQTT broker).

### Full build details can be viewed at [Resinchem Tech Blog](https://resinchemtech.blogspot.com/2021/04/ws2812b-led-matrix-clock-scoreboard-and.html) 

### Details on installation, configuration, settings and other options can be found on the [wiki](https://github.com/Resinchem/LED-Matrix-Clock_WS2812b/wiki).

>*If you found this project helpful, would like to say thanks or help support future development:*<br>
>[![buy_me_a_coffee_sm](https://user-images.githubusercontent.com/55962781/159586675-7476e996-a990-4918-8825-aa6812f3ea28.jpg)](https://www.buymeacoffee.com/resinchemtech)
