# ViaEMS
An engine management system for gasoline spark ignition engines.

The firmware is flexible to most engine configurations, and with 16
outputs it is suitable for up to an 8 cylinder engine with sequential fueling and
ignition.

Features:
- 16 high precision outputs with 250 nS scheduling resolution for fuel or ignition
- 24x24 floating point table lookups
- Speed-density fueling with VE and lambda lookups
- Supports 11 12-bit ADC inputs when using TLV2553, or 8 with AD7888
- Configurable acceleration enrichment
- Configurable warm-up enrichment
- Boost control by RPM
- Frequency sensor inputs
- Fuelpump safety cutoff
- Runtime-configurable over USB interface
- Check Engine / Malfunction Indicator control

1. [Vehicle inputs and outputs](#vehicle-inputs-and-outputs)
2. [Configuration Interface](#configuration-interface)
3. [Compiling](#compiling)
4. [Programming](#programming)
5. [CLI Tool](#cli-tool)
6. [Simulation](#simulation)
    1. [Hardware in the loop](#hardware-in-the-loop)
7. [Hardware](#hardware)
    1. [STM32F4](#stm32f4)
    2. [GD32F470](src/platforms/gd32f470/README.md).

## Vehicle inputs and outputs

### Outputs
16 high precision outputs are supported. Each output can be configured for ignition
or fuel control.  Ignition outputs are intended to drive an ignition coil via a coil driver, and as such have
configurable polarity and a few dwell options: Table lookup for battery voltage, fixed dwell, and a time based on engine
speed.

Fuel outputs are intended to drive high impedence fuel injectors via low side drivers.

### Trigger Wheels
Currently the only two decoders styles implemented are even-tooth and
missing-tooth wheels.  Wheels of each type are supported on both cam and
crankshaft, and both support an optional cam sync input. Each wheel
configuration has a tooth count and an angle per tooth.  For a cam wheel it is
expected that this totals up to 720 degrees -- for example a 24 tooth wheel on
the cam is 24 total teeth with 30 degrees per tooth.  A crank wheel totals 360
degrees. 

#### Even Teeth
This wheel can be used without a cam sync, but will have random phase to the
granularity of the wheel, and is only useful on low resolution wheels. For
example, a Ford TFI distributor has a rising edge per cylinder: It would be
configured with 8 teeth, 90 degrees per tooth.  An ignition event could be
configured for each cylinder, and the random angle is not an issue due to the
use of a distributor.

An example of a even tooth wheel with a cam sync is the Toyota 24+1 cam angle
sensor - 24 cam teeth, 30 degrees per tooth, and an additional 1-tooth wheel.
The first tooth past the cam sync is at 0 degrees.

#### Missing Teeth

This decoder is configured with a tooth count and angle per tooth.  The tooth
count includes the missing tooth, so a 36-1 wheel is 36 teeth. If the wheel is
crank-mounted without a cam sync, cam phase will be unknown and events should be
symmetric around 360 degrees (batch injection and wasted spark). The first tooth
after the gap is at 0 degrees.

### Sensors
The minimum set of sensors required for basic speed density operation is:
 - MAP (Manifold Pressure)
 - IAT (Intake temperature)

Several other inputs are used for engine processing. If the sensor is not available,
set the input source to a constant fixed value. 
Input | Usage
---   | ---
CLT   | Engine temperature enrichment
TPS   | Acceleration enrichment and boost control
AAP   | Altitude correction
BRV   | Injector dead time and dwell time calculations
FRT   | Fuel temperature compensation

The remaining sensors, while supported, are only used for data logging purposes currently.
Input | Future Use |
---  | ---
EGO | Auto-calibration, fault-detection, closed loop operation
ETH | Automatic stoich ratio and timing adjustment for flexfuel
KNK | Automatic timing advance/retard at low load

See the below configuration sections for more details on sensor interfacing.

## Configuration Interface
A default configuration is stored in the `default_config` data structure in
`config.c`, and can be modified when the runtime interface is not desirable.
For all other cases, the runtime configuration interface should be used for
config changes.  ViaEMS uses protocol buffers for messages between a computer
and the target. In the future, these messages can be sent/received over
ethernet, but in the meantime the primary configuration interface is USB, which
provides a CDC serial interface.

See #[console.proto](proto/console.proto) for details on what status reporting and configuration
options are available. High level details of the configuration options are
below.

### Streaming interface
To provide a reliable interface over a serial or USB link, binary protobuf
messages are encoded into NUL-delimited frames.  Each frame contains a
payload with a 16 bit length prefix and a 32 bit CRC trailer: 
[0,2) | [2, N+2] | [N+2, N+6)
--- | --- | ---
16-bit length of PDU | PDU of LEN bytes | 32-bit CRC over PDU

The frame is COBS-encoded to ensure it has no NUL bytes, and then it is sent over
the interface with a trailing NUL byte to indicate the end of the frame.

### EngineUpdate messages
Engine processing occurs at a fixed platform-specific rate. Currently this is
every 200 uS, or 5000 Hz.  All latest engine position and sensor information is
used to calculate fuel and ignition values, and a plan of events for the next
period is scheduled.  All the inputs and results of the calculations are sent as
an EngineUpdate message over the interface, and can be logged via the companion
software.

### Triggers
There are four dedicated trigger inputs, and how they are used is largely up to
the platform and vehicle configuration.  On STM32F4, the first two inputs are
intended for trigger or sync inputs, and the last two for frequency inputs.
Alternatively, a single frequency AND pulsewidth input can use the fourth input,
typical for a flex-fuel sensor.

Input | Meaning
--- | ---
`INPUT_TRIGGER` | Input will be used as a trigger input for wheel position/RPM.
`INPUT_SYNC` | Input will be used as a cam phase signal
`INPUT_FREQ` | Input will be used as a frequency-based sensor input

Each input can be configured for sensitivity to rising, falling, or both edges.

### Outputs
Output configuration is done with an array of schedulable events.  The array is
up to 16 events long, which for example allows for 8 ignition and 8 fuel events
in a sequential 8-cylinder application. This entire list is rescheduled at each
engine processing iteration (200 uS), allowing any angles (0-719) to be used for
events.  The configured angles are relative to TDC.
Currently there are two types of event:

Event | Meaning
--- | ---
`OUTPUT_IGNITION` | Represents spark ignition event.  Will start earlier than specified angle because of dwell, will stop at the angle specified minus any advance.
`OUTPUT_FUEL` | Represents fuel injection event.  These events will attempt to *end* fueling at the specified angle, but rescheduling will preserve the event duration rather than stop angle

Each output event has several fields:

Member | Meaning
--- | ---
`type` | Event type from above
`angle` | angle for an event
`output_id` | OUT pin to use for this output
`inverted` | Set to true if active-low

### Sensors
Sensor inputs are controlled by the `sensors` structure, which contains all the
sensors that are currently supported. Each member has a number of configuration
settings, but are largely broken down into these sections:

Setting | Meaning
--- | ---
`pin` | For ADC input, specifies which ADC pin. For FREQ inputs, specifies which frequenty input to use (hardware specific)
`lag` | A simple first-order low pass filter, using the provided value as alpha. A value of zero disables the filter completely.
`method` | Processing method, currently supported are linear interpolation and thermistors. Additionally, linear interpolation can be gated on a window of crank angles to collect samples, useful for matching MAP inputs to intake timing
`source` | Type of sensor, currently supported are analog, frequency, pulsewidth, and constant/fixed-value inputs

Linear inputs interpolate a raw frequency or voltage input in the `input_min` to
`input_max` range to the output range specified by `output_min` and
`output_max`. Thermistor inputs use the Steinhart-hart equation and are
configured with the A, B, and C values as well as a bias resistor value in ohms.

The window configuration allows an engine cycle to be split into N windows,
configured by `window.count`, and in that window, `window.opening` describes how
many degrees of crank angle to average data over. After the window, all
collected samples are ignored.  The phase into a window can be controlled with
the `window.offset` value.  For example, a `count` of 8, a `opening` of 45, and
an `offset` of 20, means that samples would be collected from crank angles 
```
[20, 65), 
[110, 175), 
[200, 265), 
...
```
A window is generally used for the MAP sensor to average over an intake stroke,
as otherwise the signal can be very noisy.

Each sensor has a fault range, specified by `fault.min` and `fault.max`,
referencing raw sensor input values. If this range is exceeded, the sensor will
report the configured `fault.value` as the value to use.


For the current hardware platforms, the onboard ADC is not used. Instead an
external ADC is connected via SPI. The TLV2553 or AD7888 ADC are supported.

### Tables
Tables can be up to 24x24 float values that are bilinearly interpolated, or 1x24 values with linear interpolation.

# Compiling
Requires:
- Complete arm toolchain with hardware fpu support.
- GNU make
- check (for unit tests)
- python3 to generate protobuf sources
- libusb-1.0 for the proxy utility

Before trying to compile, make sure to bring in submodules in contrib:
```
git submodule update --init
```

Next, build protocol buffer source files:
```
pip install -r proto/requirements.txt
make proto
```

To run the unit tests:
```
make PLATFORM=test check
```

To build an ELF binary for the stm32f4:

```
make
```
`obj/stm32f4/viaems` is the resultant executable that can be loaded.  

```
make PLATFORM=hosted proxy
```
will produce a `obj/hosted/proxy` executable that pipes stdin/stdout to a usb target.  This is used for the integration
tests as well as the commandline tools.

# Programming
You can use gdb to load, especially for development, but dfu is supported.  Connect the stm32f4 via
USB and set it to dfu mode: For a factory chip, the factory bootloader can be
brought up by holding BOOT1 high, or for any already-programmed ViaEMS chip, the
ResetToBootloader command will reboot it into DFU mode. Either way, there is a
make target:
```
make program
```
that will load the binary.

# CLI Tool
There is a commandline tool, `py/scripts/tool.py` that offers convenience access to configs and bootloader functions:

```
# Show info about the target
py/scripts/tool.py --proxy --fwinfo

# Get the current running config as a json payload
py/scripts/tool.py --proxy --get-config 

# Set the current running config as a json payload
py/scripts/tool.py --proxy --set-config < config.json 

# Reset the target into bootloader, allowing programming
py/scripts/tool.py --proxy --bootloader
```

# Simulation
The platform interface is also implemented for a Linux host machine.
```
make PLATFORM=hosted
``` 
This will build `viaems` as a Linux executable that will use stdin/stdout as
the consoled. The executable can take a "replay" file with the -r option that
takes a file used to simulate specific scenarios.  Each line in the provided
file carries a type, delay, and extra fields. Currently a trigger and adc
command are implemented. For example, the line
```
t 622 0
```
indicates that, after 622 delay ticks, a trigger input on pin 0 should be
simulated. Likewise, the line
```
a 800 0.0 0.0 1.8 0.8 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0
```
indicates that after 800 ticks, the 16 values after (in volts) should be
simulated as an ADC input.

The hosted-mode simulator can be used with flviaems or viaems-ui directly to help verify
communications, but it is also used for the integration tests to validate
various scenarios.  These tests can be found in the `py/integration-tests`
directory.  The tested scenarios produce a VCD dump for all the inputs and
outputs from the simulation, allowing easy visualization with a tool like
GTKWave for debugging purposes .

## Hardware-in-the-loop
The same tests described above can be run against a real hardware target.  This
is achieved by using custom test bench hardware using an FTDI interface and FPGA
to replay the scenario files and collect the outputs.  More information on that
hardware/gateware is available: https://github.com/via/viaems-fpga-tb

There is currently no automation for preparing/configuring the devices for this,
but that work is planned. In the meantime:
1) Connect a ViaEMS target via USB, ensure it is flashed with image to test
2) Connect the test bench hardware via USB
4) Connect the trigger inputs, GPIOs, outputs, and SPI ADC connections to the
test bench.
4) Load the bitstream onto the test bench (it is only stored in ram)
5) Ensure two tools are locally built:
  * `viaems-fpga-tb`, the rust tool in the above repo for interfacing with the
    test bench hardware, must be in PATH
  * `make PLATFORM=hosted proxy` to build the target console interface tool
6) Run the python integration tests as above, but as root and with the --hil flag:
```
sudo python py/integration-tests/smoke-tests.py --hil
```

For each scenario, the console feed from the target will be collected into a
trace file, and the hardware events collected by test bench hardware will be
merged and validated just as with a hosted simulation, with a viewable waveform
dumped into a VCD file.

# Hardware
The current supported microcontrollers are the STMicro STM32F427 and GigaDevice
GD32F450/470.  There are a few non-production-ready hardware designs
available under https://github.com/via/viaems-boards, though I would recommend
anyone attempting to use those designs contact me. Details on each platform are
available in the README files in their platform directories.

## [STM32F4](src/platforms/stm32f4/README.md)

## [GD32F4](src/platforms/gd32f4/README.md)
