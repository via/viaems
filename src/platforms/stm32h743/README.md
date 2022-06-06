## STM32H743
This is a significantly faster chip than the F4, and the DMAMUX allows more
flexibility. 

### Pins
Use | Pins
--- | ---
Trigger 0 | `PA0`
Trigger 1 | `PA1`
ADC | `PA4`, `PA5`, `PA6`, and `PA7`
USB | `PA9`, `PA11`, and `PA12`
OUT | `PD0`-`PD15`
GPIO | `PE0`-`PE15`

### Resources
System | Use
--- | ---
SPI1 | MAX11632 ADC
TIM15 | SPI(ADC) sample timer
TIM8 | TIM2 4 MHz tick driver, triggers TIM2 on update
TIM2 | Main CPU Clock, event timer, and capture units for trigger inputs
USART1 | Debug console
SYSTICK | 100 Hz task driver

