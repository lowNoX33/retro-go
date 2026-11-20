# Freenove FNK0104B

- Status: Display, audio, Wi-Fi and physical-button profile
- Hardware: ESP32-S3, 2.8-inch ILI9341 display, FT6336U capacitive touch controller, ES8311 audio codec
- Reference: https://github.com/Freenove/Freenove_ESP32_S3_Display

The display, backlight, battery ADC, and SDMMC pins follow Freenove's FNK0104AB
configuration. The current Retro-Go input driver does not read the FT6336U
touchscreen. Controls are read from a PCF8575 I2C expander (address 0x20,
A0/A1/A2 to GND) on the shared I2C bus (SDA GPIO16, SCL GPIO15, 100 kHz).
If the address pins are left floating the expander may answer anywhere in
0x20-0x27; the firmware searches that range automatically and logs the address
it finds, but tying A0/A1/A2 to GND is still recommended.
Wire each button between the PCF8575 pin and GND:

- P00: Up
- P01: Down
- P02: Left
- P03: Right
- P04: Start
- P05: Select
- P06: A
- P07: B
- P10: Option (optional)

The board's BOOT button (GPIO0) is Menu.

The Option button opens the options menu, which contains "Wi-Fi options". If no
button is wired to P10, press Select + Start together to open it.

Touch remains available to other firmware but is not mapped to Retro-Go controls.
