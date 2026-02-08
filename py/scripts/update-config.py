#!/bin/env python

import argparse
import json
import sys

from google.protobuf import json_format

from viaems_proto import console_pb2 as console

def update_table(table):
    def _axis(axis):
        result = console.Configuration.TableAxis()
        result.name = name=axis['name']
        result.values.extend(axis['values'])
        return result

    def _row(row):
        return console.Configuration.TableRow(values=row)

    if table['num-axis'] == 1:
        return console.Configuration.Table1d(
               name=table['title'],
               cols=_axis(table['horizontal-axis']),
               data=_row(table['data'])
               )
    else:
        result = console.Configuration.Table2d(
                    name=table['title'],
                    cols=_axis(table['horizontal-axis']),
                    rows=_axis(table['vertical-axis']),
                    )
        for x in range(len(table['vertical-axis']['values'])):
            result.data.append(_row(table['data'][x]))
        return result

def update_freq(f):
    return console.Configuration.TriggerInput(
            edge=console.Configuration.InputEdge.EDGE_RISING if f['edge'] == 'rising' else
                 console.Configuration.InputEdge.EDGE_FALLING if f['edge'] == 'falling' else
                 console.Configuration.InputEdge.EDGE_BOTH if f['edge'] == 'both' else None,
            type=console.Configuration.InputType.INPUT_TRIGGER if f['type'] == 'trigger' else 
                 console.Configuration.InputType.INPUT_SYNC if f['type'] == 'sync' else
                 console.Configuration.InputType.INPUT_FREQ if f['type'] == 'freq' else
                 console.Configuration.InputType.INPUT_DISABLED
            )

def update_output(o):
    return console.Configuration.Output(
            type=console.Configuration.Output.OutputType.OUTPUT_DISABLED if o['type'] == 'disabled' else 
                 console.Configuration.Output.OutputType.OUTPUT_FUEL if o['type'] == 'fuel' else
                 console.Configuration.Output.OutputType.OUTPUT_IGNITION if o['type'] == 'ignition' else
                 NONE,
            angle=o['angle'],
            pin=o['pin'],
            inverted=o['inverted']
            )
                 

def update_sensor(s):
    return console.Configuration.Sensor(
            source=console.Configuration.SensorSource.SOURCE_ADC if s['source'] == 'adc' else 
                   console.Configuration.SensorSource.SOURCE_FREQ if s['source'] == 'freq' else
                   console.Configuration.SensorSource.SOURCE_PULSEWIDTH if s['source'] == 'pulsewidth' else
                   console.Configuration.SensorSource.SOURCE_CONST if s['source'] == 'const' else
                   console.Configuration.SensorSource.SOURCE_NONE,
            method=console.Configuration.SensorMethod.METHOD_LINEAR if s['method'] == 'linear' else 
                   console.Configuration.SensorMethod.METHOD_LINEAR_WINDOWED if s['method'] == 'linear-window' else
                   console.Configuration.SensorMethod.METHOD_THERMISTOR if s['method'] == 'therm' else
                   None,
            pin=s['pin'],
            lag=s['lag'],
            linear_config=console.Configuration.Sensor.LinearConfig(
                input_min=s["raw-min"],
                input_max=s["raw-max"],
                output_min=s["range-min"],
                output_max=s["range-max"]
                ) if s['source'] != 'const' and (s['method'] == 'linear' or s['method'] == 'linear-window') else None,
            window_config=console.Configuration.Sensor.WindowConfig(
                opening=s["window-capture-opening"],
                count=s["window-count"],
                offset=s["window-offset"],
                ) if s['method'] == 'linear-window' else None,
            thermistor_config=console.Configuration.Sensor.ThermistorConfig(
                bias=s['therm-bias'],
                a=s['therm-a'],
                b=s['therm-b'],
                c=s['therm-c']
                ) if s['method'] == 'therm' else None,
            const_config=console.Configuration.Sensor.ConstConfig(
                fixed_value=s['fixed-value']
                ) if s['source'] == 'const' else None,
            fault_config=console.Configuration.Sensor.FaultConfig(
                min=s['fault-min'],
                max=s['fault-max'],
                value=s['fault-value']
                )

            )

def parse_from_old_config(jsontext, destpb):
    old = json.loads(jsontext)["config"]

    new = console.Configuration()
    [new.outputs.append(update_output(old["outputs"][i])) for i in range(0,16)]
    [new.triggers.append(update_freq(old["trigger"][i])) for i in range(0,4)]

    new.boost_control.CopyFrom(console.Configuration.BoostControl(
                pin=old["boost-control"]["pin"],
                control_threshold_map=old["boost-control"]["control-threshold"],
                control_threshold_tps=old["boost-control"]["control-threshold-tps"],
                enable_threshold_map=old["boost-control"]["enable-threshold"],
                overboost_map=old["boost-control"]["overboost"],
                pwm_vs_rpm=update_table(old["boost-control"]["pwm"]),
                ))

    new.fueling.CopyFrom(console.Configuration.Fueling(
            fuel_pump_pin=old["fueling"]["fuel-pump-pin"],
            cylinder_cc=old["fueling"]["cylinder-cc"],
            fuel_density=old["fueling"]["fuel-density"],
            fuel_stoich_ratio=old["fueling"]["fuel-stoich-ratio"],
            injections_per_cycle=old["fueling"]["injections-per-cycle"],
            injector_cc=old["fueling"]["injector-cc"],
            max_duty_cycle=old["fueling"]["max-duty-cycle"] if "max-duty-cycle" in old["fueling"] else 0.0,
            crank_enrich=console.Configuration.CrankEnrichment(
                cranking_rpm=int(old["fueling"]["crank-enrich"]["crank-rpm"]),
                cranking_temp=old["fueling"]["crank-enrich"]["crank-temp"],
                enrich_amt=old["fueling"]["crank-enrich"]["enrich-amt"],
                ),
            pulse_width_compensation=update_table(old["tables"]["injector_pw_correction"]),
            injector_dead_time=update_table(old["tables"]["injector_dead_time"]),
            engine_temp_enrichment=update_table(old["tables"]["temp-enrich"]),
            commanded_lambda=update_table(old["tables"]["lambda"]),
            ve=update_table(old["tables"]["ve"]),
            tipin_enrich_amount=update_table(old["tables"]["tipin-amount"]),
            tipin_enrich_duration=update_table(old["tables"]["tipin-time"])
            ))
    new.cel.CopyFrom(console.Configuration.CheckEngineLight(
        pin=old["check-engine-light"]["pin"],
        lean_boost_ego=old["check-engine-light"]["lean-boost-ego"],
        lean_boost_map_enable=old["check-engine-light"]["lean-boost-kpa"]
        ))
    new.decoder.CopyFrom(console.Configuration.Decoder(
        trigger_type=console.Configuration.TriggerType.MISSING_TOOTH_PLUS_CAMSYNC if old["decoder"]["trigger-type"] == "missing+camsync" else
                     console.Configuration.TriggerType.MISSING_TOOTH if old["decoder"]["trigger-type"] == "missing" else 
                     console.Configuration.TriggerType.EVEN_TEETH if old["decoder"]["trigger-type"] == "even-teeth" else 
                     console.Configuration.TriggerType.EVEN_TEETH_PLUS_CAMSYNC if old["decoder"]["trigger-type"] == "even-teeth+camsync" else 
                     console.Configuration.TriggerType.DISABLED,
        degrees_per_trigger=old["decoder"]["degrees-per-trigger"],
        max_tooth_variance=old["decoder"]["max-variance"],
        min_rpm=old["decoder"]["min-rpm"],
        num_triggers=old["decoder"]["num-triggers"],
        offset=old["decoder"]["offset"]
        ))
    new.ignition.CopyFrom(console.Configuration.Ignition(
        type=console.Configuration.Ignition.DwellType.DWELL_BRV,
        fixed_dwell=old["ignition"]["dwell-time"],
        min_dwell_us=old["ignition"]["min-dwell"],
        ignitions_per_cycle=old["ignition"]["ignitions-per-cycle"],
        min_coil_cooldown_us=old["ignition"]["min-coil-cooldown"],
        dwell=update_table(old["tables"]["dwell"]),
        timing=update_table(old["tables"]["timing"])
        ))

    new.sensors.CopyFrom(console.Configuration.Sensors(
            aap=update_sensor(old["sensors"]["aap"]),
            brv=update_sensor(old["sensors"]["brv"]),
            clt=update_sensor(old["sensors"]["clt"]),
            ego=update_sensor(old["sensors"]["ego"]),
            eth=update_sensor(old["sensors"]["eth"]),
            frp=update_sensor(old["sensors"]["frp"]),
            frt=update_sensor(old["sensors"]["frt"]),
            iat=update_sensor(old["sensors"]["iat"]),
            map=update_sensor(old["sensors"]["map"]),
            tps=update_sensor(old["sensors"]["tps"])
            ))

    new.rpm_cut.CopyFrom(console.Configuration.RpmCut(
        rpm_limit_start=old["decoder"]["rpm-limit-start"],
        rpm_limit_stop=old["decoder"]["rpm-limit-stop"]
        ))

    j = json_format.MessageToJson(new, 
                                  always_print_fields_with_no_presence=True,
                                  preserving_proto_field_name=True,
                                  indent=2)
    print(j)
    if destpb:
        with open(destpb, "wb") as f:
            f.write(new.SerializeToString())

def main():
    """
    Tool to convert json configs from flviaems to viaems 1.7.0 and viaems-ui formats
    """

    parser = argparse.ArgumentParser(
        prog="update-config.py",
        description="Tool to convert flviaems json configs to viaems 1.7.0 formats"
    )
    parser.add_argument(
        "--protobuf",
        action="store",
        default=None,
        help="Path to store a protobuf binary config",
    )

    args = parser.parse_args()
    parse_from_old_config(sys.stdin.read(), args.protobuf)



if __name__ == "__main__":
    main()
