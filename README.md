ESP32 Battery-Powered Distance Sensor w/ Digital Filter Analysis

Complete embedded systems project featuring a VL53L0X ToF distance sensor (100-1200mm range) with real-time LCD display and PWM LED proximity indicator. Includes comparison of Moving Average vs. Exponential Moving Average filters for noise reduction in stationary measurements.

Key Features:

Custom ESP32 firmware (C++) with I2C communication and PWM control
Gamma-corrected LED brightness for perceptual linearity
Digital filtering implementations (FIR/IIR) with performance analysis
50% variance reduction achieved with both filter types
Python analysis script with matplotlib visualization

Hardware: ESP32, VL53L0X ToF sensor, L7805CV regulator, 16x2 LCD, LED indicator, 9V battery
Skills: Embedded C/C++, I2C protocol, signal processing, PWM, system design, data analysis
