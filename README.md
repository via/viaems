# TFI Computer
A Ignition controller for the Ford EEC-IV Thin Film Integration module.

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
at an appropriate time to control timing based on a engine inputs:
* IAT (Intake Air Temperature)
* CLT (Coolant Temperature)
* MAP (Manifold Absolute Pressure)

Since dwell is controlled by TFI itself, there is no use for measuring
battery voltage.  Spark advance is determined by a MAP/RPM table lookup,
and 1-d correction tables for CLT and IAT, allowing for cold-start and
weather-dependent correction.  All inputs can have failsafe defaults.

Since the PIP signal has a 50% duty cycle, the falling edge of PIP
should occur 45 degrees before whatever the base timing is (adjusted by
distributor rotation), allowing the controller to simply delay timing
from this falling edge.  With a base timing of 10 degrees before TDC,
this allows up to 55 degrees of timing advance.

## Configuration

