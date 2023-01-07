### master (unreleased)

 - Implement simpler and easier-to-understand scheduler
 - Relicense to GPLv3
 - Add new platform: gd32f470
 - Add support for TLV2553 ADC, and remove support for the TLC2543

### 1.4.0 (2022 Feb 16)
This introduces a breaking change with interface software.

- New console and logging interface, as defined in INTERFACE.md. 
- Revert trigger inputs to old non-dma interrupts
- Fix linear window sensor glitch caused by race in current_angle()
- Implement missing tooth decoder

### 1.3.0 (2020 Nov 20)
- Switch build system to a GNU makefile at the toplevel
- New hosted mode that can run at realtime speed
- Fix bug in air density temperature correction and boost PWM logic
- **Boost duty is now a percent**

### 1.2.0 (2020 May 30)
Introduces a breaking change for the trigger inputs. They are now configured in
the frequency inputs section, and the pins dedicated to triggers have moved to
port A.

- Implement check engine light
- Implement cold start cranking enrich
- Record frequency and trigger edges with DMA, allowing higher rpm operation
- Implement dwell controlled by battery voltage
- Better ADC strategy:
  - Faster and more deterministic sampling
  - Implement customizable averaging windows based on crank angles
- Fix occasional miss due to incorrectly descheduling events
- Fix bug with `get bootloader`
- Move task execution to systick timer

### 1.1.0 (2019 Dec 14)
Introduces a breaking change with the console format for tables, as well as
various fixes and features
- Fix various console bugs
- Implement two-output test trigger
- Fix scheduler issue resulting in occasional ignition misses


### 1.0.0 (2019 Nov 10)
Initial release of viaems
