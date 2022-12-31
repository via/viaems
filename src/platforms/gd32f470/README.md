## GD32F470

This chip is pin compatible with the STM32F407, but with a 240 MHz clock and 
0-cycle "flash", it is significantly faster chip than the STM32F4.  More
importantly, it is available, whereas STM32 chips are currently unavailable.

The only notable difference is the lack of the DMA errata, allowing us to use 
the second DMA for multiple things.

### Pins
Use | Pins
--- | ---
Trigger 0 | `PA0`
Trigger 1 | `PA1`
ADC | `PA5`, `PA6`, `PA7`, and `PA8`
USB | `PA9`, `PA11`, and `PA12`
OUT | `PD0`-`PD15`
GPIO | `PE0`-`PE15`
PWM | `PC6`-`PC9`

### Resources
System | Use
--- | ---
SPI0 | MAX11632 ADC
TIMER0 | SPI(ADC) sample timer
TIMER1 | Main CPU Clock, event timer, and capture units for trigger inputs
TIMER2 | PWM driver, fixed at 30 Hz
TIMER7 | TIM2 4 MHz tick driver, triggers TIM2 on update
USBFS | Console
SYSTICK | 100 Hz task driver

