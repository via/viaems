#!/bin/env python

import argparse
from betterproto2 import Casing
import json
import sys

from viaems_proto.viaems import console

def update_table(table):
    def _axis(axis):
      return console.ConfigurationTableAxis(
              name=axis['name'],
              values=axis['values']
              )

    def _row(row):
        return console.ConfigurationTableRow(values=row)

    if table['num-axis'] == 1:
        return console.ConfigurationTable1D(
               name=table['title'],
               cols=_axis(table['horizontal-axis']),
               data=_row(table['data'])
               )
    else:
        result = console.ConfigurationTable2D(
                    name=table['title'],
                    cols=_axis(table['horizontal-axis']),
                    rows=_axis(table['vertical-axis']),
                    )
        for x in range(len(table['vertical-axis']['values'])):
            result.data.append(_row(table['data'][x]))
        return result

def update_freq(f):
    return console.ConfigurationTriggerInput(
            edge=console.ConfigurationInputEdge.EDGE_RISING if f['edge'] == 'rising' else
                 console.ConfigurationInputEdge.EDGE_FALLING if f['edge'] == 'falling' else
                 console.ConfigurationInputEdge.EDGE_BOTH if f['edge'] == 'both' else None,
            type=console.ConfigurationInputType.INPUT_TRIGGER if f['type'] == 'trigger' else 
                 console.ConfigurationInputType.INPUT_SYNC if f['type'] == 'sync' else
                 console.ConfigurationInputType.INPUT_FREQ if f['type'] == 'freq' else
                 console.ConfigurationInputType.INPUT_DISABLED
            )

def update_output(o):
    return console.ConfigurationOutput(
            type=console.ConfigurationOutputOutputType.OUTPUT_DISABLED if o['type'] == 'disabled' else 
                 console.ConfigurationOutputOutputType.OUTPUT_FUEL if o['type'] == 'fuel' else
                 console.ConfigurationOutputOutputType.OUTPUT_IGNITION if o['type'] == 'ignition' else
                 NONE,
            angle=o['angle'],
            pin=o['pin'],
            inverted=o['inverted']
            )
                 

def update_sensor(s):
    return console.ConfigurationSensor(
            source=console.ConfigurationSensorSource.SOURCE_ADC if s['source'] == 'adc' else 
                   console.ConfigurationSensorSource.SOURCE_FREQ if s['source'] == 'freq' else
                   console.ConfigurationSensorSource.SOURCE_PULSEWIDTH if s['source'] == 'pulsewidth' else
                   console.ConfigurationSensorSource.SOURCE_CONST if s['source'] == 'const' else
                   console.ConfigurationSensorSource.SOURCE_NONE,
            method=console.ConfigurationSensorMethod.METHOD_LINEAR if s['method'] == 'linear' else 
                   console.ConfigurationSensorMethod.METHOD_LINEAR_WINDOWED if s['method'] == 'linear-window' else
                   console.ConfigurationSensorMethod.METHOD_THERMISTOR if s['method'] == 'therm' else
                   None,
            pin=s['pin'],
            lag=s['lag'],
            linear_config=console.ConfigurationSensorLinearConfig(
                input_min=s["raw-min"],
                input_max=s["raw-max"],
                output_min=s["range-min"],
                output_max=s["range-max"]
                ) if s['source'] != 'const' and (s['method'] == 'linear' or s['method'] == 'linear-window') else None,
            window_config=console.ConfigurationSensorWindowConfig(
                opening=s["window-capture-opening"],
                count=s["window-count"],
                offset=s["window-offset"],
                ) if s['method'] == 'linear-window' else None,
            thermistor_config=console.ConfigurationSensorThermistorConfig(
                bias=s['therm-bias'],
                a=s['therm-a'],
                b=s['therm-b'],
                c=s['therm-c']
                ) if s['method'] == 'therm' else None,
            const_config=console.ConfigurationSensorConstConfig(
                fixed_value=s['fixed-value']
                ) if s['source'] == 'const' else None,
            fault_config=console.ConfigurationSensorFaultConfig(
                min=s['fault-min'],
                max=s['fault-max'],
                value=s['fault-value']
                )

            )

def parse_from_old_config(jsontext):
    old = json.loads(jsontext)["config"]

    new = console.Configuration()
    [new.outputs.append(update_output(old["outputs"][i])) for i in range(0,16)]
    [new.triggers.append(update_freq(old["trigger"][i])) for i in range(0,4)]

    new.boost_control = console.ConfigurationBoostControl(
                pin=old["boost-control"]["pin"],
                control_threshold_map=old["boost-control"]["control-threshold"],
                control_threshold_tps=old["boost-control"]["control-threshold-tps"],
                enable_threshold_map=old["boost-control"]["enable-threshold"],
                overboost_map=old["boost-control"]["overboost"],
                pwm_vs_rpm=update_table(old["boost-control"]["pwm"]),
                )

    new.fueling = console.ConfigurationFueling(
            fuel_pump_pin=old["fueling"]["fuel-pump-pin"],
            cylinder_cc=old["fueling"]["cylinder-cc"],
            fuel_density=old["fueling"]["fuel-density"],
            fuel_stoich_ratio=old["fueling"]["fuel-stoich-ratio"],
            injections_per_cycle=old["fueling"]["injections-per-cycle"],
            injector_cc=old["fueling"]["injector-cc"],
            max_duty_cycle=old["fueling"]["max-duty-cycle"] if "max-duty-cycle" in old["fueling"] else 0.0,
            crank_enrich=console.ConfigurationCrankEnrichment(
                cranking_rpm=old["fueling"]["crank-enrich"]["crank-rpm"],
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
            )
    new.cel = console.ConfigurationCheckEngineLight(
        pin=old["check-engine-light"]["pin"],
        lean_boost_ego=old["check-engine-light"]["lean-boost-ego"],
        lean_boost_map_enable=old["check-engine-light"]["lean-boost-kpa"]
        )
    new.decoder = console.ConfigurationDecoder(
        trigger_type=console.ConfigurationTriggerType.MISSING_TOOTH_PLUS_CAMSYNC if old["decoder"]["trigger-type"] == "missing+camsync" else
                     console.ConfigurationTriggerType.MISSING_TOOTH if old["decoder"]["trigger-type"] == "missing" else 
                     console.ConfigurationTriggerType.EVEN_TEETH if old["decoder"]["trigger-type"] == "even-teeth" else 
                     console.ConfigurationTriggerType.EVEN_TEETH_PLUS_CAMSYNC if old["decoder"]["trigger-type"] == "even-teeth+camsync" else 
                     console.ConfigurationTriggerType.DISABLED,
        degrees_per_trigger=old["decoder"]["degrees-per-trigger"],
        max_tooth_variance=old["decoder"]["max-variance"],
        min_rpm=old["decoder"]["min-rpm"],
        num_triggers=old["decoder"]["num-triggers"],
        offset=old["decoder"]["offset"]
        )
    new.ignition = console.ConfigurationIgnition(
        type=console.ConfigurationIgnitionDwellType.DWELL_BRV,
        fixed_dwell=old["ignition"]["dwell-time"],
        min_dwell_us=old["ignition"]["min-dwell"],
        ignitions_per_cycle=old["ignition"]["ignitions-per-cycle"],
        min_coil_cooldown_us=old["ignition"]["min-coil-cooldown"],
        dwell=update_table(old["tables"]["dwell"]),
        timing=update_table(old["tables"]["timing"])
        )

    new.sensors=console.ConfigurationSensors(
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
            )

    new.rpm_cut = console.ConfigurationRpmCut(
        rpm_limit_start=old["decoder"]["rpm-limit-start"],
        rpm_limit_stop=old["decoder"]["rpm-limit-stop"]
        )

    print(new.to_json(indent=True, casing=Casing.SNAKE))
    with open("test.pbconfig", "wb") as f:
        f.write(new.SerializeToString())

def main():
    """
    Tool to convert json configs from flviaems to viaems 1.7.0 and viaems-ui formats
    """

    parser = argparse.ArgumentParser(
        prog="viaems-tool.py",
        description="Helper tool for interfacing with a viaems target"
    )
    parser.add_argument(
        "--hosted",
        action="store_true",
        default=False,
        help="Connect to the hosted target via obj/hosted/viaems",
    )

    parser.add_argument(
        "--proxy",
        action="store_true",
        default=False,
        help="Connect to a real target via obj/hosted/proxy",
    )

    parse_from_old_config(sys.stdin.read())



if __name__ == "__main__":
    main()
