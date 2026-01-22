# ViaEMS
An engine management system for gasoline spark ignition engines.

The firmware is flexible to most engine configurations, and with 16
outputs it is suitable for up to an 8 cylinder engine with sequential fueling and
ignition.

Features:
- 16 high precision outputs with 250 nS scheduling accuracy usable for fuel or ignition
- 24x24 floating point table lookups
- Speed-density fueling with VE and lambda lookups
- Supports 8 12-bit ADC inputs when using AD7888
- Supports 16 GPIOs
- Configurable acceleration enrichment
- Configurable warm-up enrichment
- Boost control by RPM
- Frequency sensor inputs
- PWM outputs
- Fuelpump safety cutoff
- Mostly runtime-configurable over USB interface
- Check Engine / Malfunction Indicator control

1. [Decoding](#decoding)
    1. [Even Teeth](#even-teeth)
    2. [Missing Teeth](#missing-teeth)
2. [Static Configuration](#static-configuration)
    1. [Frequency and Trigger Inputs](#frequency-and-trigger-inputs)
    2. [Events](#events)
    3. [Sensors](#sensors)
    4. [Tables](#tables)
3. [Runtime Configuration](#runtime-configuration)
4. [Compiling](#compiling)
5. [Programming](#programming)
6. [Simulation](#simulation)
    1. [Hardware in the loop](#hardware-in-the-loop)
7. [Hardware](#hardware)
    1. [STM32F4](#stm32f4)
    2. [GD32F470](src/platforms/gd32f470/README.md).

## Decoding
Currently the only two decoders styles implemented are even-tooth and
missing-tooth wheels.  Wheels of each type are supported on both cam and
crankshaft, and both support an optional cam sync input. Each wheel
configuration has a tooth count and an angle per tooth.  For a cam wheel it is
expected that this totals up to 720 degrees -- for example a 24 tooth wheel on
the cam is 24 total teeth with 30 degrees per tooth.  A crank wheel totals 360
degrees. Without a cam sync, the current wheel angle has a random phase at the
granularity of the wheel.

### Even Teeth
This decoder is configured with a tooth count and angle per tooth. Two
decoder-specific options:
- `rpm-window-size` - RPM for table lookups is derived from an average this many
  teeth
- `min-triggers-rpm` - Number of teeth to have passed since sync-loss to
  re-establish an rpm, before (optionally) waiting for a cam sync pulse.

This wheel can be used without a cam sync, but will have random phase to the
granularity of the wheel, and is only useful on low resolution wheels. For
example, a Ford TFI distributor has a rising edge per cylinder: It would be
configured with 8 teeth, 90 degrees per tooth.  An ignition event could be
configured for each cylinder, and the random angle is not an issue due to the
use of a distributor.

An example of a even tooth wheel with a cam sync is the Toyota 24+1 cam angle
sensor - 24 cam teeth, 30 degrees per tooth, and an additional 1-tooth wheel.
The first tooth past the cam sync is at 0 degrees.

### Missing Teeth

This decoder is configured with a tooth count and angle per tooth.  The tooth
count includes the missing tooth, so a 36-1 wheel is 36 teeth. If the wheel is
crank-mounted without a cam sync, cam phase will be unknown and events should be
symmetric around 360 degrees (batch injection and wasted spark). The first tooth
after the gap is at 0 degrees.

## Static Configuration
See the runtime configuration section for details on the runtime control
interface.  Almost everything is configuable through that interface, but to
use a different engine configuration (read: not my Supra), it is probably best
to start with modifying the source code.

Static configuration is in `config.c`. The main configuration structure is
`config`, along with any custom table declarations, such as the provided example
`timing_vs_rpm_and_map`. Pin configurations are platform specific and are
documented in the platform source.

Member | Meaning
--- | ---
`num_events` | Number of configured events
`events` | Array of configured event structures, see Event Configuration below
`freq_inputs` | Array of configured frequency inputs
`decoder.type` | Decoder type. Possible values in `decoder.h`
`decoder.offset` | Degrees between 'decoder' top-dead-center and 'crank' top-dead-center. TFI units by default emit the falling-edge trigger 45 degrees before they would trigger spark in failsafe mode
`decoder.trigger_max_rpm_change` | Percentage of rpm change between trigger events. 1.00 would mean the engine speed can double or halve between triggers without sync loss.
`decoder.trigger_min_rpm` | Minimum RPM for sync.  Should be just below the slowest cranking speed.
`sensors` | Array of configured analog sensors.  See Sensor Configuration below.
`timing` | Points to table to do MAP/RPM lookup on for timing advance.
`ve` | Points to table for volumetric efficiency lookups
`commanded_lambda` | Points to table containing target lambda
`injector_deadtime_offset` | Points to table containing Voltage vs dead time
`injector_pw_correction` | Points to table containing pulsewidth offsets to apply 
`engine_temp_enrich` | Points to table containing CLT/MAP vs enrichment percentage
`tipin_enrich_amount` | Points to table containing Tipin enrich quantities
`tipin_enrich_duration` | Points to table containing Tipin enrich durations
`rpm_stop` | Stop event scheduling above this RPM (rev limiter)
`rpm_start` | Resume event scheduling when speed falls to this RPM (rev limiter)
`fueling.injector_cc_per_minute` | Injector flow rate
`fueling.cylinder_cc` | Individual cylinder volume
`fueling.injections_per_cycle` | Number of times an injector is fired per cycle.  1 for sequential, 2 for batched pairs, etc
`fueling.fuel_pump_pin` | GPIO port number that controls the fuel pump
`ignition.dwell_us` | Fixed (when in fixed dwell mode) time in uS to dwell ignition

### Frequency and Trigger inputs
Certain inputs are used as frequency inputs which may also act as the decoder
wheel trigger inputs. Each one has a selectable edge and type.

Member | Meaning
--- | ---
`type` | Can either be `TRIGGER` or `FREQ`. Some platforms can only use certain pins as trigger inputs
`edge` | Determines which edge is counted. `RISING_EDGE`, `FALLING_EDGE`, `BOTH_EDGES`

### Events
Event configuration is done with an array of schedulable events.  This entire
array is rescheduled at each trigger decode, allowing any angles (0-719) to be
used for events, as long as a trigger event occurs between any two fires of a
given event (e.g. for `FORD_TFI`, for 8 events, as long as the decoder fires
twice in 720 degrees, which of course it does).  Currently there are two types
of event:

Event | Meaning
--- | ---
`IGNITION_EVENT` | Represents spark ignition event.  Will start earlier than specified angle because of dwell, will stop at the angle specified minus any advance.
`FUEL_EVENT` | Represents fuel injection event.  These events will attempt to *end* fueling at the specified angle, but rescheduling will preserve the event duration rather than stop angle

The event structure has these fields:

Member | Meaning
--- | ---
`type` | Event type from above
`angle` | Base angle for an event
`output_id` | OUT pin to use for this output
`inverted` | Set to one if active-low

### Sensors
Sensor inputs are controlled by the `sensors` config structure member, and is an
array of configured inputs.  The indexes into the array for various sensors are
hardcoded, use the provided enums to reference the sensors.  Each sensor entry
is a structure describing what pin to use, and how to translate the raw sensor
value into a usable number:

Member | Meaning
--- | ---
`pin` | For ADC input, specifies which ADC pin. For FREQ inputs, specifies which frequenty input to use (hardware specific)
`method` | Processing method, currently supported are linear interpolation and thermistors
`source` | Type of sensor, currently supported are analog and frequency based.
`params` | Union used to configure a sensor. Contains `range` used for calculated sensors, and `table` for table lookup sensors.
`params.range.min` | For processing, value that lowest raw sensor value reflects
`params.range.max` | For processing, value that highest raw sensor value reflects
`raw_min` | The raw input value that the minimum level is associated with
`raw_max` | The raw input value that the maximum level is associated with
`params.therm` | For processing, Bias resistor and A, B and C parameters of the Steinart-Hart equation
`fault_config.min` | Raw sensor value, below this indicates sensor fault
`fault_config.max` | Raw sensor value, above this indicates sensor fault
`fault_config.fault_value` | During sensor fault, use this fallback value
`lag` | Lag filtering value. 0 means no filtering, 100 will effectively never change.
`window.total_width` | When method is windowed, total stride of degrees for a single averaging
`window.capture_width` | When method is windowed, window inside of total stride to average samples for
`window.offset` | When method is windowed, offset of capture window inside total window

For method `SENSOR_LINEAR`, the processed value is linear interpolated based on
the raw value between min and max (with the raw value being between `raw_min` and `raw_max`)

The onboard ADC is not used. Instead an external ADC is connected
to SPI2 (PB12-PB15).  Currently a TLV2553 or AD7888 ADC is supported.

### Tables
Tables can be up to 24x24 float values that are bilinearly interpolated. Use
`struct table` to define a table, with the following relevent fields:

Member | Meaning
--- | ---
`title` | Table title
`num_axis` | 1 or 2, for 1d vs 2d lookup
`axis[0]` | Only axis if 1d table, horizontal axis if 2d.
`axis[1]` | Vertical axis for 2d table.
`axis[?].name` | Axis Title
`axis[?].num` | Number of columns/rows in the axis
`axis[?].values` | Value for each column in the axis
`data.one` | Array of data points for 1d table
`data.two` | 2d array for 2d table.  First index represents vertical axis.

For a new table to be configurable over the console, it must be declared
externally such that it can be directly referenced in `console.c`.

## Runtime configuration
The EMS is uses a cbor-based (https://cbor.io/) binary RPC protocol for its
console.  For hosted mode, standard out and in are used to exchange messages,
and on the stm32f4 target, the console is exposed via a USB virtual console.
Details of the binary protocol are described in [INTERFACE.md](INTERFACE.md).

# Compiling
Requires:
- Complete arm toolchain with hardware fpu support.
- GNU make
- check (for unit tests)
- python3 to generate protobuf sources

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


# Programming
You can use gdb to load, especially for development, but dfu is supported.  Connect the stm32f4 via
USB and set it to dfu mode: For a factory chip, the factory bootloader can be
brought up by holding BOOT1 high, or for any already-programmed ViaEMS chip, the
`get bootloader` command will reboot it into DFU mode. Either way, there is a
make target:
```
make program
```
that will load the binary.


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

The hosted-mode simulator can be used with flviaems directly to help verify
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
