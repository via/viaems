# ViaEMS
An engine management system for gasoline spark ignition engines.

The firmware is flexible to most engine configurations, supporting up to 16 high
precision outputs suitable for ignition coil drivers or fuel injectors.
Therefore it is suitable for an 8 cylinder engine with sequential fueling and
ignition.

The current primary hardware platform is an ST Micro STM32F407VGT
microcontroller.  There are a few non-production-ready hardware designs
available under https://github.com/via/tfi-board, though I would recommend
anyone attempting to use those designs contact me.  The STM32F4-DISCOVERY board
*can* be used with this firmware, with appropriate extra hardware for vehicle
interfacing.  See the hardware section for more detail.


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

## Configuration
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
`decoder.type` | Decoder type. Possible values in `decoder.h`
`decoder.offset` | Degrees between 'decoder' top-dead-center and 'crank' top-dead-center. TFI units by default emit the falling-edge trigger 45 degrees before they would trigger spark in failsafe mode
`decoder.trigger_max_rpm_change` | Percentage of rpm change between trigger events. 1.00 would mean the engine speed can double or halve between triggers without sync loss.
`decoder.trigger_min_rpm` | Minimum RPM for sync.  Should be just below the slowest cranking speed.
`sensors` | Array of configured analog sensors.  See Sensor Configuration below.
`timing` | Points to table to do MAP/RPM lookup on for timing advance.
`ve` | Points to table for volumetric efficiency lookups
`commanded_lambda` | Points to table containing target lambda
`injector_pw_compensation` | Points to table containing Voltage vs dead time
`tipin_enrich_amount` | Points to table containing Tipin enrich quantities
`tipin_enrich_duration` | Points to table containing Tipin enrich durations
`rpm_stop` | Stop event scheduling above this RPM (rev limiter)
`rpm_start` | Resume event scheduling when speed falls to this RPM (rev limiter)
`fueling.injector_cc_per_minute` | Injector flow rate
`fueling.cylinder_cc` | Individual cylinder volume
`fueling.injections_per_cycle` | Number of times an injector is fired per cycle.  1 for sequential, 2 for batched pairs, etc
`fueling.fuel_pump_pin` | GPIO port number that controls the fuel pump
`ignition.dwell_ud` | Fixed (currently) time in uS to dwell ignition

### Event Configuration
Event configuration is done with an array of schedulable events.  This entire
array is rescheduled at each trigger decode, allowing any angles (0-719) to be
used for events, as long as a trigger event occurs between any two fires of a
given event (e.g. for `FORD_TFI`, for 8 events, as long as the decoder fires
twice in 720 degrees, which of course it does).  Currently there are three types
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

### Sensor Configuration
Sensor inputs are controlled by the `sensors` config structure member, and is an
array of configured inputs.  The indexes into the array for various sensors are
hardcoded, use the provided enums to reference the sensors.  Each sensor entry
is a structure describing what pin to use, and how to translate the raw sensor
value into a usable number:

Member | Meaning
--- | ---
`pin` | pin to use on ADC
`method` | Processing method, currently supported are linear interpolation, table looksup, and thermistors
`source` | Type of sensor, currently supported are analog and frequency based.
`params` | Union used to configure a sensor. Contains `range` used for calculated sensors, and `table` for table lookup sensors.
`lag` | Lag filtering value. 0 means no filtering, 100 will effectively never change.
`params.range.min` | For processing, value that lowest raw sensor value reflects
`params.range.max` | For processing, value that highest raw sensor value reflects
`params.therm` | For processing, Bias resistor and A, B and C parameters of the Steinart-Hart equation
`fault_config.min` | Raw sensor value, below this indicates sensor fault
`fault_config.max` | Raw sensor value, above this indicates sensor fault
`fault_config.fault_value` | During sensor fault, use this fallback value

For method `SENSOR_LINEAR`, the processed value is linear interpolated based on
the raw value between min and max (with the raw value being 0 - 4096).

The onboard ADC is not used. Instead an external ADC is connected
to SPI2 (PB12-PB15).  Currently a TLC2543 or AD7888 ADC is supported.

### Table Configuration
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

For a new table to be configurable over the serial console, it must be declared
externally such that it can be directly referenced in `console.c`.

## Runtime configuration
The console is exposed via a serial interface, and on the stm32f4 target this
uses a USB virtual console. The console immediately outputs csv lines with a default set of values.
This set of values, along with any other configurable, can get viewed or changed
by commands and responses.  All commands are processed after a newline, and the
result with start with a `* `, distinguishing it from normal log output.

There are various debug outputs that can also be enabled (primary the
`sim.event_logging` flag), these outputs are prefixed with a `# `.

Any configuration node may support the command `list`, `get`, or `set`. Toplevel
nodes are `config` and `status`.  The hierarchy of config nodes can be traversed
by using `list`, and then any commands on the results.

A few specific example nodes:

Node | Meaning
--- | ---
`config` | Top-level configuration
`status` | Real-time status output
`config.tables.timing` | Timing table
`config.sensors.iat` | IAT sensor
`config.feed` | comma-delimited list of config nodes to output
`flash` | Set to save current configuration
`bootloader` | Set to reboot into dfu mode for firmware updates

Any config node that is a simple value (string, float, int) can be used with
`config.feed`.  Some config nodes are detailed. On get they will return a
space-delimited list of `=`-delimited key-value pairs. Any pair returned can
then be set on the node. For example

```
get config.tables.timing 
 * Name=test num_axis=2 rows=3 ...
get config.events 0
 * type=fuel angle=0 output=3 inverted=0
set config.events 2 inverted=1
```

Tables will treat any key that is in the form of `[row][col]` as a value to be
returned or set
```
get config.tables.timing [2][2]
set config.tables.timing name=Timing [2][2]=5.2
```

# Compiling
Requires:
- Complete arm toolchain with hardware fpu support.
- BSD Make (bmake or pmake)
- check (for unit tests)

Before trying to compile, make sure to bring in the libopencm3 submodule:
```
git submodule update --init
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

## Programming
You can use gdb to load, especially for development, but dfu is supported.  Connect the stm32f4 via
USB and set it to dfu mode: For a factory chip, the factory bootloader can be
brought up by holding BOOT1 high, or for any already-programmed ViaEMS chip, the
`get bootloader` command will reboot it into DFU mode. Either way, there is a
make target:
```
bmake program`
```
that will load the binary.

To run the unit tests:
```
cd src/
make clean
make check
```

## Hosted Simulation
The platform interface is also implemented for a Linux host machine.
```
bmake PLATFORM=hosted
```
This will build `tfi` as a Linux executable that will use stdin/stdout as the
console.  The test trigger that is enabled by default will provide enough inputs
to verify some basic functionality.  Full integration testing using this
simulation mode is planned.

# Hardware
Any supported platform has hardware-specific pin assignments.  

## STM32F4
Note: These are subject to change with the next hardware design, which will be
moving to better support a 64 pin chip, superior frequency inputs (currently
poorly supported), and more flexible PWM outputs.

System | Pins
--- | ---
Trigger 0 | `PB3`
Trigger 1 | `PB10`
Test Trigger | `PA0` (Emits square wave at fixed RPM)
`CANL` | `PB5`
`CANH` | `PB6`
USB | `PA9`, `PA11`, and `PA12`
ADC | `PB12`, `PB13`, `PB14`, and `PB15`
OUT | `PD0`-`PD15`
GPIO | `PE0`-`PE15`
PWM | `PC6`, `PC7`, `PC8`, and `PC9` (currently all fixed at same frequenty)

