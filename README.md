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
    1. [EEC-IV TFI](#eec-iv-tfi)
    2. [Toyota CAS](#toyota-241-cas)
2. [Static Configuration](#static-configuration)
    1. [Frequency and Trigger Inputs](#frequency-and-trigger-inputs)
    2. [Events](#events)
    3. [Sensors](#sensors)
    4. [Tables](#tables)
3. [Runtime Configuration](#runtime-configuration)
    1. [Frequency and Trigger Inputs](#frequency-and-trigger-inputs-1)
    2. [Events](#events-1)
    3. [Sensors](#sensors-1)
    4. [Tables](#tables-1)
    5. [Tasks](#tasks)
4. [Compiling](#compiling)
5. [Programming](#programming)
6. [Simulation](#simulation)
7. [Hardware](#hardware)
    1. [STM32F4](#stm32f4)

## Decoding
Currently the only two decoders styles implemented are a generic N+1 cam/crank decoder,
and an N tooth cam/crank decoder (useful only for batch injection and
distributor ignition).  There are preset implementations of these for a Toyota
24+1 CAS, and the Ford TFI.  

### EEC-IV TFI
EEE-IV TFI modules came in a few flavors. Currently only the
Grey(Pushstart) mounted-on-distributor varient is supported.  This
module controls dwell based on RPM, and allows ECM timing control via
the SPOUT signal.  The module outputs a PIP signal, which is a clean
filtered 50% duty cycle square wave formed from a rotating vane and hall
effect sensor inside the distributor.  Without a SPOUT signal from the
ECM, the module will fall-back on firing the coil on the rising edge of
PIP, which will still drive the engine in fixed-timing, fixed-dwell
mode.  

### Toyota 24+1 CAS
This is the standard Mk3 Toyota Supra cam angle sensor.  It has 24 tooth on a
primary wheel and 1 tooth on a secondary wheel, both gear driven by the exhaust
cam.

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
`injector_pw_compensation` | Points to table containing Voltage vs dead time
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
`params.therm` | For processing, Bias resistor and A, B and C parameters of the Steinart-Hart equation
`fault_config.min` | Raw sensor value, below this indicates sensor fault
`fault_config.max` | Raw sensor value, above this indicates sensor fault
`fault_config.fault_value` | During sensor fault, use this fallback value
`lag` | Lag filtering value. 0 means no filtering, 100 will effectively never change.
`window.total_width` | When method is windowed, total stride of degrees for a single averaging
`window.capture_width` | When method is windowed, window inside of total stride to average samples for
`window.offset` | When method is windowed, offset of capture window inside total window

For method `SENSOR_LINEAR`, the processed value is linear interpolated based on
the raw value between min and max (with the raw value being 0 - 4095 for 12-bit
ADCs).

The onboard ADC is not used. Instead an external ADC is connected
to SPI2 (PB12-PB15).  Currently a TLC2543 or AD7888 ADC is supported.

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
On the stm32f4 target, the console is exposed via a USB virtual console.  At all
times the console outputs status updates in the form of comma-delimited values
(the list of which is controlled by the space-delimited `config.feed` node).
Additionally, there is a command/response system for configuration and
retrieving more complex information.  Responses to commands are distinguished by
being prefixed with `* `, whereas the constant status updates have no
prefix.

There are various debug outputs that can also be enabled (primary the
`sim.event_logging` flag), these outputs are prefixed with a `# `.

All commands are newline delimited, and start with one of `list`, `get`, or
`set`, followed by a configuration node to be acted on.
- `list` returns all nodes prefixed with the given node. The node is optional,
  and if not specified it will return all possible configuration nodes
- `get` is used to retrieve configuration or status values
- `set` is used to set configuration or status values

Most nodes under `status` are simple display values representing current engine
status, and most nodes under `config` are configuration values.  Several other
toplevel nodes are special-use:

Node | Meaning
--- | ---
`flash` | `set flash` will save any runtime changes to flash
`stats` | `get stats` reports multiple newline-delimited performance metrics
`bootloader` | `get bootloader` will reboot into USB DFU mode for programming
`sim.test_trigger` | `set sim.test_trigger 2500` will set the test output to 2500 RPM
`sim.event_logging` | `set sim.event_logging on` will log all input and output changes to the status updates prefixed with `# `


A few example status nodes:

Node | Meaning
--- | ---
`status.current_time` | Current cpu time in ticks (4 MHz for STM42F4 target)
`status.decoder.rpm` | Current RPM
`status.sensors.iat` | IAT sensor processed value
`status.sensors.iat.fault` | IAT sensor fault status (`-` indicates no fault)
`status.fueling.pw_us` | Current fuel injector pulse width

Configuration nodes are divided into categories.  All values under
`config.fueling`, `config.decoder`, and `config.ignition` are simple in that
they take a single value to set or get, but the others are more complex data
structures that are configured with multiple key/value pairs.

### Frequency and Trigger inputs
The `config.hardware.freq` node takes a frequency input number, type, and edge:
```
get config.hardware.freq
* num_freq=4
get config.hardware.freq 0
* type=trigger edge=rising
set config.hardware.freq 1 type=trigger edge=rising
* 
```

### Events
The `config.events` config node, if used as a simple node, only sets the number
of configured events.  To actually configure an event, it takes an event number
and extra key/value options:
```
get config.events
* num_events=12
get config.events 0
* type=ignition angle=0 output=0 inverted=0
set config.events 0 inverted=1
* 
get config.events 0
* type=ignition angle=0 output=0 inverted=1
```

### Tables
When a table node is read like a simple value, it will return metadata about the
table.  If a `get` is given `[row][column]` indices (up to 16 at a time), it will return
the table values at those points.  `set` can be given key/value pairs to set
metadata or values:

```
get config.tables.ve
* name=ve naxis=2 cols=16 colname=RPM rows=16 rowname=MAP collabels=[250.0,500.0,900.0,1200.0,1600.0,2000.0,2400.0,3000.0,3600.0,4000.0,4400.0,5200.0,5800.0,6400.0,6800.0,7200.0] rowlabels=[20.0,30.0,40.0,50.0,60.0,70.0,80.0,90.0,100.0,120.0,140.0,160.0,180.0,200.0,220.0,240.0]
get config.tables.ve [0][0] [0][1]
* 65.00 30.00
set config.tables.ve name=myVE [0][0]=12.2
* 
get config.tables.ve
* name=myVE naxis=2 cols=16 colname=RPM rows=16 rowname=MAP collabels=[250.0,500.0,900.0,1200.0,1600.0,2000.0,2400.0,3000.0,3600.0,4000.0,4400.0,5200.0,5800.0,6400.0,6800.0,7200.0] rowlabels=[20.0,30.0,40.0,50.0,60.0,70.0,80.0,90.0,100.0,120.0,140.0,160.0,180.0,200.0,220.0,240.0]
get config.tables.ve [0][0]
* 12.20
```

### Sensors
The nodes under `config.sensors` will return sensor configuration as key/value
pairs, and can be set similarly:

```
get config.sensors.iat
* source=adc method=therm pin=1 therm-bias=2490.00 therm-a=1.461674e-03 therm-b=2.288757e-04 therm-c=1.644848e-07 fault-min=2 fault-max=4095 fault-val=10.00 lag=0.000000
set config.sensors.iat pin=2 therm-bias=2400.0
*
get config.sensors.iat
* source=adc method=therm pin=2 therm-bias=2400.00 therm-a=1.461674e-03 therm-b=2.288757e-04 therm-c=1.644848e-07 fault-min=2 fault-max=4095 fault-val=10.00 lag=0.000000
```

### Tasks
Tasks are low priority tasks that execute during idle cycles.  These are useful
for management of non-timing-critical outputs, such as boost control and fuel
  pumps.

Node | Meaning
--- | ---
`config.tasks.boost_control.overboost` | Fuel and ignition cuts are activated at this manifold pressure
`config.tasks.boost_control.pin` | PWM output for boost control solenoid
`config.tasks.boost_control.threshold` | Boost control will not activate until this pressure is reached, though the solenoid will be fully activated below level if at WOT
`config.tasks.cel.pin` | GPIO output for Check-Engine light
`config.tasks.cel.lean_boost_kpa` | CEL will trigger in a boosted lean condition if lean above this pressure
`config.tasks.cel.lean_boost_ego` | CEL will trigger in a boosted lean condition if leaner than this EGO value

# Compiling
Requires:
- Complete arm toolchain with hardware fpu support.
- BSD Make (bmake or pmake)
- check (for unit tests)

Before trying to compile, make sure to bring in the libopencm3 submodule:
```
git submodule update --init
```

To run the unit tests:
```
cd src/
bmake clean
bmake check
```

To build an ELF binary for the stm32f4:

```
cd libopencm3
make
cd ../src/
bmake clean
bmake
```
`tfi` is the resultant executable that can be loaded.  

# Programming
You can use gdb to load, especially for development, but dfu is supported.  Connect the stm32f4 via
USB and set it to dfu mode: For a factory chip, the factory bootloader can be
brought up by holding BOOT1 high, or for any already-programmed ViaEMS chip, the
`get bootloader` command will reboot it into DFU mode. Either way, there is a
make target:
```
bmake program`
```
that will load the binary.


# Simulation
The platform interface is also implemented for a Linux host machine.
```
bmake PLATFORM=hosted
```
This will build `tfi` as a Linux executable that will use stdin/stdout as the
console.  The test trigger that is enabled by default will provide enough inputs
to verify some basic functionality.  Full integration testing using this
simulation mode is planned.

# Hardware
The current primary hardware platform is an ST Micro STM32F407VGT
microcontroller.  There are a few non-production-ready hardware designs
available under https://github.com/via/tfi-board, though I would recommend
anyone attempting to use those designs contact me.  The STM32F4-DISCOVERY board
*can* be used with this firmware, with appropriate extra hardware for vehicle
interfacing.


## STM32F4
Note: These are subject to change with the next hardware design, which will be
moving to better support a 64 pin chip and more flexible PWM outputs.

System | Pins
--- | ---
Trigger 0 | `PA0`
Trigger 1 | `PA1`
Frequency | `PA0`, `PA1`, `PA2`, `PA3`
Test Trigger | `PB10`, `PB11`
`CANL` | `PB5`
`CANH` | `PB6`
USB | `PA9`, `PA11`, and `PA12`
ADC | `PB12`, `PB13`, `PB14`, and `PB15`
OUT | `PD0`-`PD15`
GPIO | `PE0`-`PE15`
PWM | `PC6`, `PC7`, `PC8`, and `PC9` (currently all fixed at same frequenty)

