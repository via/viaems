# TFI Computer
An Ignition controller for gasoline spark ignition engines.

Currently it supports the Ford EEC-IV Thin Film Integration module as a decoder
and ignitor, using the STM32F4-Discovery Cortex-M4 Board from ST, and a TLC2543
ADC from TI. It should be fairly generic to any engine, with any number of
ignition outputs, if a decoder is written.

## Decoding

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

This controller uses PIP to determine engine speed, and will rise SPOUT
at an appropriate time to control timing based on a engine inputs

Since dwell is controlled by TFI itself, there is no use for measuring
battery voltage.  Spark advance is determined by a MAP/RPM table lookup.

The decoder currently measures RPM by using the last two trigger times, creating
an average over 180 degrees of crank rotation.  Trigger-to-trigger variation
allowance in rpm is controlled by `config.decoder.offset`.

## Configuration
Static configuration is in `config.c`. The main configuration structure is
`config`, along with any custom table declarations, such as the provided example
`timing_vs_rpm_and_map`. Pin configurations are platform specific and are
documented in the platform source.

Member | Meaning
--- | ---
`num_events` | Number of configured events
`events` | Array of configured event structures, see Event Configuration below
`trigger` | Decoder type. Possible values in `decoder.h`
`decoder.offset` | Degrees between 'decoder' top-dead-center and 'crank' top-dead-center. TFI units by default emit the falling-edge trigger 45 degrees before they would trigger spark in failsafe mode
`decoder.trigger_max_rpm_change` | Percentage of rpm change between trigger events. 1.00 would mean the engine speed can double or halve between triggers without sync loss.
`decoder.trigger_min_rpm` | Minimum RPM for sync.  Should be just below the slowest cranking speed.
`decoder.t0_pin` | PORTB Pin for primary trigger
`decoder.t1_pin` | PORTB Pin for secondary trigger
`sensors` | Array of configured analog sensors.  See Sensor Configuration below.
`timing` | Points to table to do MAP/RPM lookup on for timing advance.
`ve` | Points to table for volumetric efficiency lookups
`commanded_lambda` | Points to table containing target lambda
`injector_pw_compensation` | Points to table containing Voltage vs dead time
`rpm_stop` | Stop event scheduling above this RPM (rev limiter)
`rpm_start` | Resume event scheduling when speed falls to this RPM (rev limiter)
`console.baud` | Serial console baud
`console.stop_bits` | Serial console stop bits
`console.data_bits` | Serial console data bits
`console.parity` | Serial console parity

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
`FUEL_EVENT` | Represents fuel injection event.  Not currently implemented.
`ADC_EVENT` | Initiates sensor data gathering event.  Sensor data
is often only useful at certain parts of the engine cycle.

The event structure has these fields:

Member | Meaning
--- | ---
`type` | Event type from above
`angle` | Base angle for an event
`output_id` | PORTD pin to use for this output
`inverted` | Set to one if active-low

### Sensor Configuration
Sensor inputs are controlled by the `sensors` config structure member, and is an
array of configured inputs.  The indexes into the array for various sensors are
hardcoded, use the provided enums to reference the sensors.  Each sensor entry
is a structure describing what pin to use, and how to translate the raw sensor
value into a usable number:

Member | Meaning
--- | ---
`pin` | pin to use on TLC2543
`process` | Processing function to use (see below)
`method` | Type of sensor, currently supported are analog and frequency based.
`params` | Union used to configure a sensor. Contains `range` used for calculated sensors, and `table` for table lookup sensors.
`params.range.min` | For processing, value that lowest raw sensor value reflects
`params.range.max` | For processing, value that highest raw sensor value reflects

The process function can point to any function that takes a pointer to the
sensor structure.  Currently the only provided ones are `sensor_process_linear`, which
linearly interpolates between `min` and `max`, and `sensor_process_freq`, which 
converts based on measured frequency.

The onboard ADC is not used. Instead a TLC2543 external ADC should be connected
to SPI2 (PB12-PB15).  Currently the first 10 inputs are supported.

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
The serial interface provides a (barbaric) means of setting some things
dynamically.  At startup, the serial interface will provide a constant stream of
data points:
```
* rpm=0 sync=1 loss=0 variance=0.000 t0_count=0 t1_count=0 map=60.0 adv=0.0 dwell_us=0 pw_us=0

```

Sending any command (or empty line) will drop to a command prompt. The data
stream can be resumed with the `feed` command.
Currently implemented commands:

Command | Meaning
--- | ---
`testtrigger` | Set fake trigger output on Pin A0 with given rpm. Ex: `testtrigger 5000`
`set` | Set variable or table (see below)
`get` | Set variable or table (see below)
`feed` | Resume constant data stream

The `set` and `get` commands can work with any variable that has been configured
in `console.c` `struct console_var vars[]`.  For any variable that is type UINT
or FLOAT, simply specify the value after the variable for set:
`set config.rpm_start 5200`
`get config.rpm_stop`

For tables, axis/metadata for the table will be fetched with a simple get on the
tablename, e.g. `get config.timing`.  To get or set a cell, reference it with
0-offset indexes into the table, horizontal column first. `set
config.timing[0][2] 20.0`, for example, will set the timing advance for RPM
offset 0 (400 rpms by default) and 2 (100 kpa) to 20 degrees.

Currently persistence is not supported.

# Compiling
Requires:
- Complete arm toolchain with hardware fpu support.
- BSD Make (bmake or pmake)
- check (for unit tests)

```
cd src/
make clean
make PLATFORM=stm32f4-discovery
```
`tfi` is the resultant executable that can be loaded.

To run the unit tests:
```
cd src/
make clean
make check
./check
```
