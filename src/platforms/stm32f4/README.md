## STM32F4xx

This platform primarily targets the STM32F427VGT 1MB 100 LQFP package, and
explicitly does not support the STM32F407. This is primarily due to:
 - Errata with the DMA that affects the 407
 - Dual bank flash allows hot updates in the 427

Other part numbers could be supported with minor changes.

Config data is stored in sectors 12-15, which are the first 64K of bank 2. For
1MB chips, make sure DB1M in OPTCR DB1M is set to use dual bank support.  On 2MB
chips, please update the linker script for the appropriate 

### Pins
Use | Pins
--- | ---
Trigger 0 | `PA0`
Trigger 1 | `PA1`
Flexfuel  | `PA3`
ADC | `PA5`, `PA6`, `PA7`, and `PA8`
USB | `PA9`, `PA11`, and `PA12`
OUT | `PD0`-`PD15`
GPIO | `PE0`-`PE15`
PWM | `PC6`-`PC9`

### Resources
System | Use
--- | ---
SPI1 | TLV2553 ADC
TIM1 | SPI(ADC) sample timer
TIM2 | Main CPU Clock, event timer, and capture units for trigger inputs
TIM3 | PWM driver, fixed at 30 Hz
TIM8 | TIM2 4 MHz tick driver, triggers TIM2 on update
TIM9 | Flexfuel frequency/pulsewidth measurement
USBFS | Console
SYSTICK | 100 Hz task driver


### Interrupts
Main engine handling is done in the DMA IRQ for the scheduled output double
buffer change, every 200 uS. It can be preempted by decoding updates, but is at
the same priority as all sensor handling (adc, ethanol). 

This means that only shared decoder access needs to be protected with interrupts
disabled, and is due to that triggers can come in at a dynamic rate, potentially
higher than the 200 uS period of the engine handling.

System   | Prio | Use
---      | ---  | ---
TIM2     |    2 | Trigger and decoder processing
DMA2 S0  |    3 | Process ADC samples
TIM9     |    3 | Process Ethanol sensor
DMA2 S1  |    3 | Engine processing and output scheduling
