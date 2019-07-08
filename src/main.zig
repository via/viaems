const c_tfi = @cImport({
    @cInclude("platform.h");
    @cInclude("util.h");
    @cInclude("decoder.h");
    @cInclude("scheduler.h");
    @cInclude("config.h");
    @cInclude("table.h");
    @cInclude("sensors.h");
    @cInclude("calculations.h");
    @cInclude("stats.h");
    @cInclude("tasks.h");
});

export fn main() c_int {
    c_tfi.platform_load_config();
    c_tfi.decoder_init(&c_tfi.config.decoder);
    c_tfi.console_init();
    c_tfi.platform_init();
    c_tfi.initialize_scheduler();
    c_tfi.enable_test_trigger(c_tfi.trigger_type.FORD_TFI, 2000);
    c_tfi.sensors_process(c_tfi.sensor_source.SENSOR_CONST);

    while (true) {
        c_tfi.stats_increment_counter(c_tfi.stats_field_t.STATS_MAINLOOP_RATE);
        c_tfi.console_process();
        c_tfi.handle_fuel_pump();
        c_tfi.handle_boost_control();
        c_tfi.handle_idle_control();
        c_tfi.adc_gather();
    }

    return 0;
}
