#ifndef VIAPB_SRC_PROTO_CONSOLE_PB_H
#define VIAPB_SRC_PROTO_CONSOLE_PB_H

#include <stdint.h>
#include <stdbool.h>

#include "viapb.h"

// Types from console.proto

typedef enum {
  viaems_console_SensorFault_SENSOR_NO_FAULT = 0,
  viaems_console_SensorFault_SENSOR_RANGE_FAULT = 1,
  viaems_console_SensorFault_SENSOR_CONNECTION_FAULT = 2,
} viaems_console_SensorFault;


typedef enum {
  viaems_console_DecoderLossReason_DECODER_NO_LOSS = 0,
  viaems_console_DecoderLossReason_DECODER_TRIGGER_TOOTH_VARIATION = 1,
  viaems_console_DecoderLossReason_DECODER_TRIGGER_COUNT_HIGH = 2,
  viaems_console_DecoderLossReason_DECODER_TRIGGER_COUNT_LOW = 3,
  viaems_console_DecoderLossReason_DECODER_EXPIRED = 4,
  viaems_console_DecoderLossReason_DECODER_OVERFLOWED = 5,
} viaems_console_DecoderLossReason;


struct viaems_console_Header {
  uint32_t seq;

  uint32_t timestamp;

};

bool pb_encode_viaems_console_Header(const struct viaems_console_Header *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Header_to_buffer(uint8_t buffer[10], const struct viaems_console_Header *msg);
unsigned pb_sizeof_viaems_console_Header(const struct viaems_console_Header *msg);

bool pb_decode_viaems_console_Header(struct viaems_console_Header *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Header_seq     1ul
#define PB_TAG_viaems_console_Header_timestamp     2ul
#define PB_MAX_SIZE_viaems_console_Header    10ul


struct viaems_console_Sensors {
  float map;

  float iat;

  float clt;

  float brv;

  float tps;

  float aap;

  float frt;

  float ego;

  float frp;

  float eth;

  float knock1;

  float knock2;

  viaems_console_SensorFault map_fault;

  viaems_console_SensorFault iat_fault;

  viaems_console_SensorFault clt_fault;

  viaems_console_SensorFault brv_fault;

  viaems_console_SensorFault tps_fault;

  viaems_console_SensorFault aap_fault;

  viaems_console_SensorFault frt_fault;

  viaems_console_SensorFault ego_fault;

  viaems_console_SensorFault frp_fault;

  viaems_console_SensorFault eth_fault;

  float map_rate;

  float iat_rate;

  float clt_rate;

  float brv_rate;

  float tps_rate;

  float aap_rate;

  float frt_rate;

  float ego_Rate;

  float frp_rate;

  float eth_rate;

};

bool pb_encode_viaems_console_Sensors(const struct viaems_console_Sensors *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Sensors_to_buffer(uint8_t buffer[150], const struct viaems_console_Sensors *msg);
unsigned pb_sizeof_viaems_console_Sensors(const struct viaems_console_Sensors *msg);

bool pb_decode_viaems_console_Sensors(struct viaems_console_Sensors *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Sensors_map     1ul
#define PB_TAG_viaems_console_Sensors_iat     2ul
#define PB_TAG_viaems_console_Sensors_clt     3ul
#define PB_TAG_viaems_console_Sensors_brv     4ul
#define PB_TAG_viaems_console_Sensors_tps     5ul
#define PB_TAG_viaems_console_Sensors_aap     6ul
#define PB_TAG_viaems_console_Sensors_frt     7ul
#define PB_TAG_viaems_console_Sensors_ego     8ul
#define PB_TAG_viaems_console_Sensors_frp     9ul
#define PB_TAG_viaems_console_Sensors_eth     10ul
#define PB_TAG_viaems_console_Sensors_knock1     14ul
#define PB_TAG_viaems_console_Sensors_knock2     15ul
#define PB_TAG_viaems_console_Sensors_map_fault     32ul
#define PB_TAG_viaems_console_Sensors_iat_fault     33ul
#define PB_TAG_viaems_console_Sensors_clt_fault     34ul
#define PB_TAG_viaems_console_Sensors_brv_fault     35ul
#define PB_TAG_viaems_console_Sensors_tps_fault     36ul
#define PB_TAG_viaems_console_Sensors_aap_fault     37ul
#define PB_TAG_viaems_console_Sensors_frt_fault     38ul
#define PB_TAG_viaems_console_Sensors_ego_fault     39ul
#define PB_TAG_viaems_console_Sensors_frp_fault     40ul
#define PB_TAG_viaems_console_Sensors_eth_fault     41ul
#define PB_TAG_viaems_console_Sensors_map_rate     64ul
#define PB_TAG_viaems_console_Sensors_iat_rate     65ul
#define PB_TAG_viaems_console_Sensors_clt_rate     66ul
#define PB_TAG_viaems_console_Sensors_brv_rate     67ul
#define PB_TAG_viaems_console_Sensors_tps_rate     68ul
#define PB_TAG_viaems_console_Sensors_aap_rate     69ul
#define PB_TAG_viaems_console_Sensors_frt_rate     70ul
#define PB_TAG_viaems_console_Sensors_ego_Rate     71ul
#define PB_TAG_viaems_console_Sensors_frp_rate     72ul
#define PB_TAG_viaems_console_Sensors_eth_rate     73ul
#define PB_MAX_SIZE_viaems_console_Sensors    150ul


struct viaems_console_Position {
  uint32_t time;

  uint32_t valid_before_timestamp;

  bool has_position;

  bool synced;

  viaems_console_DecoderLossReason loss_cause;

  float last_angle;

  float instantaneous_rpm;

  float average_rpm;

};

bool pb_encode_viaems_console_Position(const struct viaems_console_Position *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Position_to_buffer(uint8_t buffer[31], const struct viaems_console_Position *msg);
unsigned pb_sizeof_viaems_console_Position(const struct viaems_console_Position *msg);

bool pb_decode_viaems_console_Position(struct viaems_console_Position *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Position_time     1ul
#define PB_TAG_viaems_console_Position_valid_before_timestamp     2ul
#define PB_TAG_viaems_console_Position_has_position     3ul
#define PB_TAG_viaems_console_Position_synced     4ul
#define PB_TAG_viaems_console_Position_loss_cause     5ul
#define PB_TAG_viaems_console_Position_last_angle     6ul
#define PB_TAG_viaems_console_Position_instantaneous_rpm     7ul
#define PB_TAG_viaems_console_Position_average_rpm     8ul
#define PB_MAX_SIZE_viaems_console_Position    31ul


struct viaems_console_Calculations {
  float advance;

  float dwell_us;

  float fuel_us;

  float airmass_per_cycle;

  float fuelvol_per_cycle;

  float tipin_percent;

  float injector_dead_time;

  float pulse_width_correction;

  float lambda;

  float ve;

  float engine_temp_enrichment;

  bool rpm_limit_cut;

  bool boost_cut;

  bool fuel_overduty_cut;

  bool dwell_overduty_cut;

};

bool pb_encode_viaems_console_Calculations(const struct viaems_console_Calculations *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Calculations_to_buffer(uint8_t buffer[63], const struct viaems_console_Calculations *msg);
unsigned pb_sizeof_viaems_console_Calculations(const struct viaems_console_Calculations *msg);

bool pb_decode_viaems_console_Calculations(struct viaems_console_Calculations *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Calculations_advance     1ul
#define PB_TAG_viaems_console_Calculations_dwell_us     2ul
#define PB_TAG_viaems_console_Calculations_fuel_us     3ul
#define PB_TAG_viaems_console_Calculations_airmass_per_cycle     4ul
#define PB_TAG_viaems_console_Calculations_fuelvol_per_cycle     5ul
#define PB_TAG_viaems_console_Calculations_tipin_percent     6ul
#define PB_TAG_viaems_console_Calculations_injector_dead_time     7ul
#define PB_TAG_viaems_console_Calculations_pulse_width_correction     8ul
#define PB_TAG_viaems_console_Calculations_lambda     9ul
#define PB_TAG_viaems_console_Calculations_ve     10ul
#define PB_TAG_viaems_console_Calculations_engine_temp_enrichment     11ul
#define PB_TAG_viaems_console_Calculations_rpm_limit_cut     12ul
#define PB_TAG_viaems_console_Calculations_boost_cut     13ul
#define PB_TAG_viaems_console_Calculations_fuel_overduty_cut     14ul
#define PB_TAG_viaems_console_Calculations_dwell_overduty_cut     15ul
#define PB_MAX_SIZE_viaems_console_Calculations    63ul


struct viaems_console_Event {
  bool has_header;
  struct viaems_console_Header header;

  unsigned which_type;
  union {
    uint32_t trigger;
    uint32_t output_pins;
    uint32_t gpio_pins;
  } type;

};

bool pb_encode_viaems_console_Event(const struct viaems_console_Event *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Event_to_buffer(uint8_t buffer[18], const struct viaems_console_Event *msg);
unsigned pb_sizeof_viaems_console_Event(const struct viaems_console_Event *msg);

bool pb_decode_viaems_console_Event(struct viaems_console_Event *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Event_header     1ul
#define PB_TAG_viaems_console_Event_trigger     2ul
#define PB_TAG_viaems_console_Event_output_pins     3ul
#define PB_TAG_viaems_console_Event_gpio_pins     4ul
#define PB_MAX_SIZE_viaems_console_Event    18ul


struct viaems_console_EngineUpdate {
  bool has_header;
  struct viaems_console_Header header;

  bool has_position;
  struct viaems_console_Position position;

  bool has_sensors;
  struct viaems_console_Sensors sensors;

  bool has_calculations;
  struct viaems_console_Calculations calculations;

};

bool pb_encode_viaems_console_EngineUpdate(const struct viaems_console_EngineUpdate *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_EngineUpdate_to_buffer(uint8_t buffer[263], const struct viaems_console_EngineUpdate *msg);
unsigned pb_sizeof_viaems_console_EngineUpdate(const struct viaems_console_EngineUpdate *msg);

bool pb_decode_viaems_console_EngineUpdate(struct viaems_console_EngineUpdate *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_EngineUpdate_header     1ul
#define PB_TAG_viaems_console_EngineUpdate_position     2ul
#define PB_TAG_viaems_console_EngineUpdate_sensors     3ul
#define PB_TAG_viaems_console_EngineUpdate_calculations     4ul
#define PB_MAX_SIZE_viaems_console_EngineUpdate    263ul


// Nested enums for Configuration

typedef enum {
  viaems_console_Configuration_SensorSource_SOURCE_NONE = 0,
  viaems_console_Configuration_SensorSource_SOURCE_ADC = 1,
  viaems_console_Configuration_SensorSource_SOURCE_FREQ = 2,
  viaems_console_Configuration_SensorSource_SOURCE_PULSEWIDTH = 3,
  viaems_console_Configuration_SensorSource_SOURCE_CONST = 4,
} viaems_console_Configuration_SensorSource;


typedef enum {
  viaems_console_Configuration_SensorMethod_METHOD_LINEAR = 0,
  viaems_console_Configuration_SensorMethod_METHOD_LINEAR_WINDOWED = 1,
  viaems_console_Configuration_SensorMethod_METHOD_THERMISTOR = 2,
} viaems_console_Configuration_SensorMethod;


typedef enum {
  viaems_console_Configuration_TriggerType_DECODER_DISABLED = 0,
  viaems_console_Configuration_TriggerType_EVEN_TEETH = 1,
  viaems_console_Configuration_TriggerType_EVEN_TEETH_PLUS_CAMSYNC = 2,
  viaems_console_Configuration_TriggerType_MISSING_TOOTH = 3,
  viaems_console_Configuration_TriggerType_MISSING_TOOTH_PLUS_CAMSYNC = 4,
} viaems_console_Configuration_TriggerType;


typedef enum {
  viaems_console_Configuration_InputEdge_EDGE_RISING = 0,
  viaems_console_Configuration_InputEdge_EDGE_FALLING = 1,
  viaems_console_Configuration_InputEdge_EDGE_BOTH = 2,
} viaems_console_Configuration_InputEdge;


typedef enum {
  viaems_console_Configuration_InputType_INPUT_DISABLED = 0,
  viaems_console_Configuration_InputType_INPUT_TRIGGER = 1,
  viaems_console_Configuration_InputType_INPUT_SYNC = 2,
  viaems_console_Configuration_InputType_INPUT_FREQ = 3,
} viaems_console_Configuration_InputType;


// Nested messages for Configuration

struct viaems_console_Configuration_TableRow {
  unsigned values_count;
  float values[24];

};

bool pb_encode_viaems_console_Configuration_TableRow(const struct viaems_console_Configuration_TableRow *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_TableRow_to_buffer(uint8_t buffer[98], const struct viaems_console_Configuration_TableRow *msg);
unsigned pb_sizeof_viaems_console_Configuration_TableRow(const struct viaems_console_Configuration_TableRow *msg);

bool pb_decode_viaems_console_Configuration_TableRow(struct viaems_console_Configuration_TableRow *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_TableRow_values     1ul
#define PB_MAX_SIZE_viaems_console_Configuration_TableRow    98ul


struct viaems_console_Configuration_TableAxis {
  bool has_name;
  struct { char str[24]; unsigned len; } name;

  unsigned values_count;
  float values[24];

};

bool pb_encode_viaems_console_Configuration_TableAxis(const struct viaems_console_Configuration_TableAxis *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_TableAxis_to_buffer(uint8_t buffer[124], const struct viaems_console_Configuration_TableAxis *msg);
unsigned pb_sizeof_viaems_console_Configuration_TableAxis(const struct viaems_console_Configuration_TableAxis *msg);

bool pb_decode_viaems_console_Configuration_TableAxis(struct viaems_console_Configuration_TableAxis *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_TableAxis_name     1ul
#define PB_TAG_viaems_console_Configuration_TableAxis_values     2ul
#define PB_MAX_SIZE_viaems_console_Configuration_TableAxis    124ul


struct viaems_console_Configuration_Table1d {
  bool has_name;
  struct { char str[24]; unsigned len; } name;

  bool has_cols;
  struct viaems_console_Configuration_TableAxis cols;

  bool has_data;
  struct viaems_console_Configuration_TableRow data;

};

bool pb_encode_viaems_console_Configuration_Table1d(const struct viaems_console_Configuration_Table1d *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_Table1d_to_buffer(uint8_t buffer[252], const struct viaems_console_Configuration_Table1d *msg);
unsigned pb_sizeof_viaems_console_Configuration_Table1d(const struct viaems_console_Configuration_Table1d *msg);

bool pb_decode_viaems_console_Configuration_Table1d(struct viaems_console_Configuration_Table1d *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_Table1d_name     1ul
#define PB_TAG_viaems_console_Configuration_Table1d_cols     2ul
#define PB_TAG_viaems_console_Configuration_Table1d_data     3ul
#define PB_MAX_SIZE_viaems_console_Configuration_Table1d    252ul


struct viaems_console_Configuration_Table2d {
  bool has_name;
  struct { char str[24]; unsigned len; } name;

  bool has_cols;
  struct viaems_console_Configuration_TableAxis cols;

  bool has_rows;
  struct viaems_console_Configuration_TableAxis rows;

  unsigned data_count;
  struct viaems_console_Configuration_TableRow data[24];

};

bool pb_encode_viaems_console_Configuration_Table2d(const struct viaems_console_Configuration_Table2d *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_Table2d_to_buffer(uint8_t buffer[2678], const struct viaems_console_Configuration_Table2d *msg);
unsigned pb_sizeof_viaems_console_Configuration_Table2d(const struct viaems_console_Configuration_Table2d *msg);

bool pb_decode_viaems_console_Configuration_Table2d(struct viaems_console_Configuration_Table2d *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_Table2d_name     1ul
#define PB_TAG_viaems_console_Configuration_Table2d_cols     2ul
#define PB_TAG_viaems_console_Configuration_Table2d_rows     3ul
#define PB_TAG_viaems_console_Configuration_Table2d_data     4ul
#define PB_MAX_SIZE_viaems_console_Configuration_Table2d    2678ul


// Nested enums for Output

typedef enum {
  viaems_console_Configuration_Output_OutputType_OUTPUT_DISABLED = 0,
  viaems_console_Configuration_Output_OutputType_OUTPUT_FUEL = 1,
  viaems_console_Configuration_Output_OutputType_OUTPUT_IGNITION = 2,
} viaems_console_Configuration_Output_OutputType;


struct viaems_console_Configuration_Output {
  bool has_pin;
  uint32_t pin;

  bool has_type;
  viaems_console_Configuration_Output_OutputType type;

  bool has_inverted;
  bool inverted;

  bool has_angle;
  float angle;

};

bool pb_encode_viaems_console_Configuration_Output(const struct viaems_console_Configuration_Output *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_Output_to_buffer(uint8_t buffer[15], const struct viaems_console_Configuration_Output *msg);
unsigned pb_sizeof_viaems_console_Configuration_Output(const struct viaems_console_Configuration_Output *msg);

bool pb_decode_viaems_console_Configuration_Output(struct viaems_console_Configuration_Output *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_Output_pin     1ul
#define PB_TAG_viaems_console_Configuration_Output_type     2ul
#define PB_TAG_viaems_console_Configuration_Output_inverted     3ul
#define PB_TAG_viaems_console_Configuration_Output_angle     4ul
#define PB_MAX_SIZE_viaems_console_Configuration_Output    15ul


// Nested messages for Sensor

struct viaems_console_Configuration_Sensor_LinearConfig {
  float output_min;

  float output_max;

  float input_min;

  float input_max;

};

bool pb_encode_viaems_console_Configuration_Sensor_LinearConfig(const struct viaems_console_Configuration_Sensor_LinearConfig *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_Sensor_LinearConfig_to_buffer(uint8_t buffer[20], const struct viaems_console_Configuration_Sensor_LinearConfig *msg);
unsigned pb_sizeof_viaems_console_Configuration_Sensor_LinearConfig(const struct viaems_console_Configuration_Sensor_LinearConfig *msg);

bool pb_decode_viaems_console_Configuration_Sensor_LinearConfig(struct viaems_console_Configuration_Sensor_LinearConfig *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_Sensor_LinearConfig_output_min     1ul
#define PB_TAG_viaems_console_Configuration_Sensor_LinearConfig_output_max     2ul
#define PB_TAG_viaems_console_Configuration_Sensor_LinearConfig_input_min     3ul
#define PB_TAG_viaems_console_Configuration_Sensor_LinearConfig_input_max     4ul
#define PB_MAX_SIZE_viaems_console_Configuration_Sensor_LinearConfig    20ul


struct viaems_console_Configuration_Sensor_ConstConfig {
  float fixed_value;

};

bool pb_encode_viaems_console_Configuration_Sensor_ConstConfig(const struct viaems_console_Configuration_Sensor_ConstConfig *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_Sensor_ConstConfig_to_buffer(uint8_t buffer[5], const struct viaems_console_Configuration_Sensor_ConstConfig *msg);
unsigned pb_sizeof_viaems_console_Configuration_Sensor_ConstConfig(const struct viaems_console_Configuration_Sensor_ConstConfig *msg);

bool pb_decode_viaems_console_Configuration_Sensor_ConstConfig(struct viaems_console_Configuration_Sensor_ConstConfig *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_Sensor_ConstConfig_fixed_value     1ul
#define PB_MAX_SIZE_viaems_console_Configuration_Sensor_ConstConfig    5ul


struct viaems_console_Configuration_Sensor_ThermistorConfig {
  float a;

  float b;

  float c;

  float bias;

};

bool pb_encode_viaems_console_Configuration_Sensor_ThermistorConfig(const struct viaems_console_Configuration_Sensor_ThermistorConfig *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_Sensor_ThermistorConfig_to_buffer(uint8_t buffer[20], const struct viaems_console_Configuration_Sensor_ThermistorConfig *msg);
unsigned pb_sizeof_viaems_console_Configuration_Sensor_ThermistorConfig(const struct viaems_console_Configuration_Sensor_ThermistorConfig *msg);

bool pb_decode_viaems_console_Configuration_Sensor_ThermistorConfig(struct viaems_console_Configuration_Sensor_ThermistorConfig *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_Sensor_ThermistorConfig_a     1ul
#define PB_TAG_viaems_console_Configuration_Sensor_ThermistorConfig_b     2ul
#define PB_TAG_viaems_console_Configuration_Sensor_ThermistorConfig_c     3ul
#define PB_TAG_viaems_console_Configuration_Sensor_ThermistorConfig_bias     4ul
#define PB_MAX_SIZE_viaems_console_Configuration_Sensor_ThermistorConfig    20ul


struct viaems_console_Configuration_Sensor_FaultConfig {
  float min;

  float max;

  float value;

};

bool pb_encode_viaems_console_Configuration_Sensor_FaultConfig(const struct viaems_console_Configuration_Sensor_FaultConfig *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_Sensor_FaultConfig_to_buffer(uint8_t buffer[15], const struct viaems_console_Configuration_Sensor_FaultConfig *msg);
unsigned pb_sizeof_viaems_console_Configuration_Sensor_FaultConfig(const struct viaems_console_Configuration_Sensor_FaultConfig *msg);

bool pb_decode_viaems_console_Configuration_Sensor_FaultConfig(struct viaems_console_Configuration_Sensor_FaultConfig *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_Sensor_FaultConfig_min     1ul
#define PB_TAG_viaems_console_Configuration_Sensor_FaultConfig_max     2ul
#define PB_TAG_viaems_console_Configuration_Sensor_FaultConfig_value     3ul
#define PB_MAX_SIZE_viaems_console_Configuration_Sensor_FaultConfig    15ul


struct viaems_console_Configuration_Sensor_WindowConfig {
  float capture_width;

  float total_width;

  float offset;

};

bool pb_encode_viaems_console_Configuration_Sensor_WindowConfig(const struct viaems_console_Configuration_Sensor_WindowConfig *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_Sensor_WindowConfig_to_buffer(uint8_t buffer[15], const struct viaems_console_Configuration_Sensor_WindowConfig *msg);
unsigned pb_sizeof_viaems_console_Configuration_Sensor_WindowConfig(const struct viaems_console_Configuration_Sensor_WindowConfig *msg);

bool pb_decode_viaems_console_Configuration_Sensor_WindowConfig(struct viaems_console_Configuration_Sensor_WindowConfig *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_Sensor_WindowConfig_capture_width     1ul
#define PB_TAG_viaems_console_Configuration_Sensor_WindowConfig_total_width     2ul
#define PB_TAG_viaems_console_Configuration_Sensor_WindowConfig_offset     3ul
#define PB_MAX_SIZE_viaems_console_Configuration_Sensor_WindowConfig    15ul


struct viaems_console_Configuration_Sensor {
  bool has_source;
  viaems_console_Configuration_SensorSource source;

  bool has_method;
  viaems_console_Configuration_SensorMethod method;

  bool has_pin;
  uint32_t pin;

  bool has_lag;
  float lag;

  bool has_linear_config;
  struct viaems_console_Configuration_Sensor_LinearConfig linear_config;

  bool has_const_config;
  struct viaems_console_Configuration_Sensor_ConstConfig const_config;

  bool has_thermistor_config;
  struct viaems_console_Configuration_Sensor_ThermistorConfig thermistor_config;

  bool has_fault_config;
  struct viaems_console_Configuration_Sensor_FaultConfig fault_config;

  bool has_window_config;
  struct viaems_console_Configuration_Sensor_WindowConfig window_config;

};

bool pb_encode_viaems_console_Configuration_Sensor(const struct viaems_console_Configuration_Sensor *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_Sensor_to_buffer(uint8_t buffer[100], const struct viaems_console_Configuration_Sensor *msg);
unsigned pb_sizeof_viaems_console_Configuration_Sensor(const struct viaems_console_Configuration_Sensor *msg);

bool pb_decode_viaems_console_Configuration_Sensor(struct viaems_console_Configuration_Sensor *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_Sensor_source     1ul
#define PB_TAG_viaems_console_Configuration_Sensor_method     2ul
#define PB_TAG_viaems_console_Configuration_Sensor_pin     3ul
#define PB_TAG_viaems_console_Configuration_Sensor_lag     4ul
#define PB_TAG_viaems_console_Configuration_Sensor_linear_config     5ul
#define PB_TAG_viaems_console_Configuration_Sensor_const_config     6ul
#define PB_TAG_viaems_console_Configuration_Sensor_thermistor_config     7ul
#define PB_TAG_viaems_console_Configuration_Sensor_fault_config     8ul
#define PB_TAG_viaems_console_Configuration_Sensor_window_config     9ul
#define PB_MAX_SIZE_viaems_console_Configuration_Sensor    100ul


struct viaems_console_Configuration_KnockSensor {
  bool has_enabled;
  bool enabled;

  bool has_frequency;
  float frequency;

  bool has_threshold;
  float threshold;

};

bool pb_encode_viaems_console_Configuration_KnockSensor(const struct viaems_console_Configuration_KnockSensor *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_KnockSensor_to_buffer(uint8_t buffer[12], const struct viaems_console_Configuration_KnockSensor *msg);
unsigned pb_sizeof_viaems_console_Configuration_KnockSensor(const struct viaems_console_Configuration_KnockSensor *msg);

bool pb_decode_viaems_console_Configuration_KnockSensor(struct viaems_console_Configuration_KnockSensor *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_KnockSensor_enabled     1ul
#define PB_TAG_viaems_console_Configuration_KnockSensor_frequency     2ul
#define PB_TAG_viaems_console_Configuration_KnockSensor_threshold     3ul
#define PB_MAX_SIZE_viaems_console_Configuration_KnockSensor    12ul


struct viaems_console_Configuration_Sensors {
  bool has_aap;
  struct viaems_console_Configuration_Sensor aap;

  bool has_brv;
  struct viaems_console_Configuration_Sensor brv;

  bool has_clt;
  struct viaems_console_Configuration_Sensor clt;

  bool has_ego;
  struct viaems_console_Configuration_Sensor ego;

  bool has_frt;
  struct viaems_console_Configuration_Sensor frt;

  bool has_iat;
  struct viaems_console_Configuration_Sensor iat;

  bool has_map;
  struct viaems_console_Configuration_Sensor map;

  bool has_tps;
  struct viaems_console_Configuration_Sensor tps;

  bool has_frp;
  struct viaems_console_Configuration_Sensor frp;

  bool has_eth;
  struct viaems_console_Configuration_Sensor eth;

  bool has_knock1;
  struct viaems_console_Configuration_KnockSensor knock1;

  bool has_knock2;
  struct viaems_console_Configuration_KnockSensor knock2;

};

bool pb_encode_viaems_console_Configuration_Sensors(const struct viaems_console_Configuration_Sensors *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_Sensors_to_buffer(uint8_t buffer[1048], const struct viaems_console_Configuration_Sensors *msg);
unsigned pb_sizeof_viaems_console_Configuration_Sensors(const struct viaems_console_Configuration_Sensors *msg);

bool pb_decode_viaems_console_Configuration_Sensors(struct viaems_console_Configuration_Sensors *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_Sensors_aap     1ul
#define PB_TAG_viaems_console_Configuration_Sensors_brv     2ul
#define PB_TAG_viaems_console_Configuration_Sensors_clt     3ul
#define PB_TAG_viaems_console_Configuration_Sensors_ego     4ul
#define PB_TAG_viaems_console_Configuration_Sensors_frt     5ul
#define PB_TAG_viaems_console_Configuration_Sensors_iat     6ul
#define PB_TAG_viaems_console_Configuration_Sensors_map     7ul
#define PB_TAG_viaems_console_Configuration_Sensors_tps     8ul
#define PB_TAG_viaems_console_Configuration_Sensors_frp     9ul
#define PB_TAG_viaems_console_Configuration_Sensors_eth     10ul
#define PB_TAG_viaems_console_Configuration_Sensors_knock1     14ul
#define PB_TAG_viaems_console_Configuration_Sensors_knock2     15ul
#define PB_MAX_SIZE_viaems_console_Configuration_Sensors    1048ul


struct viaems_console_Configuration_Decoder {
  bool has_trigger_type;
  viaems_console_Configuration_TriggerType trigger_type;

  bool has_degrees_per_trigger;
  float degrees_per_trigger;

  bool has_max_tooth_variance;
  float max_tooth_variance;

  bool has_min_rpm;
  float min_rpm;

  bool has_num_triggers;
  uint32_t num_triggers;

  bool has_offset;
  float offset;

};

bool pb_encode_viaems_console_Configuration_Decoder(const struct viaems_console_Configuration_Decoder *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_Decoder_to_buffer(uint8_t buffer[28], const struct viaems_console_Configuration_Decoder *msg);
unsigned pb_sizeof_viaems_console_Configuration_Decoder(const struct viaems_console_Configuration_Decoder *msg);

bool pb_decode_viaems_console_Configuration_Decoder(struct viaems_console_Configuration_Decoder *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_Decoder_trigger_type     1ul
#define PB_TAG_viaems_console_Configuration_Decoder_degrees_per_trigger     2ul
#define PB_TAG_viaems_console_Configuration_Decoder_max_tooth_variance     3ul
#define PB_TAG_viaems_console_Configuration_Decoder_min_rpm     4ul
#define PB_TAG_viaems_console_Configuration_Decoder_num_triggers     5ul
#define PB_TAG_viaems_console_Configuration_Decoder_offset     6ul
#define PB_MAX_SIZE_viaems_console_Configuration_Decoder    28ul


struct viaems_console_Configuration_TriggerInput {
  bool has_edge;
  viaems_console_Configuration_InputEdge edge;

  bool has_type;
  viaems_console_Configuration_InputType type;

};

bool pb_encode_viaems_console_Configuration_TriggerInput(const struct viaems_console_Configuration_TriggerInput *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_TriggerInput_to_buffer(uint8_t buffer[4], const struct viaems_console_Configuration_TriggerInput *msg);
unsigned pb_sizeof_viaems_console_Configuration_TriggerInput(const struct viaems_console_Configuration_TriggerInput *msg);

bool pb_decode_viaems_console_Configuration_TriggerInput(struct viaems_console_Configuration_TriggerInput *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_TriggerInput_edge     1ul
#define PB_TAG_viaems_console_Configuration_TriggerInput_type     2ul
#define PB_MAX_SIZE_viaems_console_Configuration_TriggerInput    4ul


struct viaems_console_Configuration_CrankEnrichment {
  bool has_cranking_rpm;
  float cranking_rpm;

  bool has_cranking_temp;
  float cranking_temp;

  bool has_enrich_amt;
  float enrich_amt;

};

bool pb_encode_viaems_console_Configuration_CrankEnrichment(const struct viaems_console_Configuration_CrankEnrichment *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_CrankEnrichment_to_buffer(uint8_t buffer[15], const struct viaems_console_Configuration_CrankEnrichment *msg);
unsigned pb_sizeof_viaems_console_Configuration_CrankEnrichment(const struct viaems_console_Configuration_CrankEnrichment *msg);

bool pb_decode_viaems_console_Configuration_CrankEnrichment(struct viaems_console_Configuration_CrankEnrichment *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_CrankEnrichment_cranking_rpm     1ul
#define PB_TAG_viaems_console_Configuration_CrankEnrichment_cranking_temp     2ul
#define PB_TAG_viaems_console_Configuration_CrankEnrichment_enrich_amt     3ul
#define PB_MAX_SIZE_viaems_console_Configuration_CrankEnrichment    15ul


struct viaems_console_Configuration_Fueling {
  bool has_fuel_pump_pin;
  uint32_t fuel_pump_pin;

  bool has_cylinder_cc;
  float cylinder_cc;

  bool has_fuel_density;
  float fuel_density;

  bool has_fuel_stoich_ratio;
  float fuel_stoich_ratio;

  bool has_injections_per_cycle;
  uint32_t injections_per_cycle;

  bool has_injector_cc;
  float injector_cc;

  bool has_max_duty_cycle;
  float max_duty_cycle;

  bool has_crank_enrich;
  struct viaems_console_Configuration_CrankEnrichment crank_enrich;

  bool has_pulse_width_compensation;
  struct viaems_console_Configuration_Table1d pulse_width_compensation;

  bool has_injector_dead_time;
  struct viaems_console_Configuration_Table1d injector_dead_time;

  bool has_engine_temp_enrichment;
  struct viaems_console_Configuration_Table2d engine_temp_enrichment;

  bool has_commanded_lambda;
  struct viaems_console_Configuration_Table2d commanded_lambda;

  bool has_ve;
  struct viaems_console_Configuration_Table2d ve;

  bool has_tipin_enrich_amount;
  struct viaems_console_Configuration_Table2d tipin_enrich_amount;

  bool has_tipin_enrich_duration;
  struct viaems_console_Configuration_Table1d tipin_enrich_duration;

};

bool pb_encode_viaems_console_Configuration_Fueling(const struct viaems_console_Configuration_Fueling *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_Fueling_to_buffer(uint8_t buffer[11551], const struct viaems_console_Configuration_Fueling *msg);
unsigned pb_sizeof_viaems_console_Configuration_Fueling(const struct viaems_console_Configuration_Fueling *msg);

bool pb_decode_viaems_console_Configuration_Fueling(struct viaems_console_Configuration_Fueling *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_Fueling_fuel_pump_pin     1ul
#define PB_TAG_viaems_console_Configuration_Fueling_cylinder_cc     2ul
#define PB_TAG_viaems_console_Configuration_Fueling_fuel_density     3ul
#define PB_TAG_viaems_console_Configuration_Fueling_fuel_stoich_ratio     4ul
#define PB_TAG_viaems_console_Configuration_Fueling_injections_per_cycle     5ul
#define PB_TAG_viaems_console_Configuration_Fueling_injector_cc     6ul
#define PB_TAG_viaems_console_Configuration_Fueling_max_duty_cycle     7ul
#define PB_TAG_viaems_console_Configuration_Fueling_crank_enrich     16ul
#define PB_TAG_viaems_console_Configuration_Fueling_pulse_width_compensation     17ul
#define PB_TAG_viaems_console_Configuration_Fueling_injector_dead_time     18ul
#define PB_TAG_viaems_console_Configuration_Fueling_engine_temp_enrichment     19ul
#define PB_TAG_viaems_console_Configuration_Fueling_commanded_lambda     20ul
#define PB_TAG_viaems_console_Configuration_Fueling_ve     21ul
#define PB_TAG_viaems_console_Configuration_Fueling_tipin_enrich_amount     22ul
#define PB_TAG_viaems_console_Configuration_Fueling_tipin_enrich_duration     23ul
#define PB_MAX_SIZE_viaems_console_Configuration_Fueling    11551ul


// Nested enums for Ignition

typedef enum {
  viaems_console_Configuration_Ignition_DwellType_DWELL_FIXED_DUTY = 0,
  viaems_console_Configuration_Ignition_DwellType_DWELL_FIXED_TIME = 1,
  viaems_console_Configuration_Ignition_DwellType_DWELL_BRV = 2,
} viaems_console_Configuration_Ignition_DwellType;


struct viaems_console_Configuration_Ignition {
  bool has_type;
  viaems_console_Configuration_Ignition_DwellType type;

  bool has_fixed_dwell;
  float fixed_dwell;

  bool has_fixed_duty;
  float fixed_duty;

  bool has_dwell;
  struct viaems_console_Configuration_Table1d dwell;

  bool has_timing;
  struct viaems_console_Configuration_Table2d timing;

};

bool pb_encode_viaems_console_Configuration_Ignition(const struct viaems_console_Configuration_Ignition *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_Ignition_to_buffer(uint8_t buffer[2948], const struct viaems_console_Configuration_Ignition *msg);
unsigned pb_sizeof_viaems_console_Configuration_Ignition(const struct viaems_console_Configuration_Ignition *msg);

bool pb_decode_viaems_console_Configuration_Ignition(struct viaems_console_Configuration_Ignition *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_Ignition_type     1ul
#define PB_TAG_viaems_console_Configuration_Ignition_fixed_dwell     2ul
#define PB_TAG_viaems_console_Configuration_Ignition_fixed_duty     3ul
#define PB_TAG_viaems_console_Configuration_Ignition_dwell     4ul
#define PB_TAG_viaems_console_Configuration_Ignition_timing     5ul
#define PB_MAX_SIZE_viaems_console_Configuration_Ignition    2948ul


struct viaems_console_Configuration_BoostControl {
  bool has_pin;
  uint32_t pin;

  bool has_control_threshold_map;
  float control_threshold_map;

  bool has_control_threshold_tps;
  float control_threshold_tps;

  bool has_enable_threshold_map;
  float enable_threshold_map;

  bool has_overboost_map;
  float overboost_map;

  bool has_pwm_vs_rpm;
  struct viaems_console_Configuration_Table1d pwm_vs_rpm;

};

bool pb_encode_viaems_console_Configuration_BoostControl(const struct viaems_console_Configuration_BoostControl *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_BoostControl_to_buffer(uint8_t buffer[281], const struct viaems_console_Configuration_BoostControl *msg);
unsigned pb_sizeof_viaems_console_Configuration_BoostControl(const struct viaems_console_Configuration_BoostControl *msg);

bool pb_decode_viaems_console_Configuration_BoostControl(struct viaems_console_Configuration_BoostControl *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_BoostControl_pin     1ul
#define PB_TAG_viaems_console_Configuration_BoostControl_control_threshold_map     2ul
#define PB_TAG_viaems_console_Configuration_BoostControl_control_threshold_tps     3ul
#define PB_TAG_viaems_console_Configuration_BoostControl_enable_threshold_map     4ul
#define PB_TAG_viaems_console_Configuration_BoostControl_overboost_map     5ul
#define PB_TAG_viaems_console_Configuration_BoostControl_pwm_vs_rpm     6ul
#define PB_MAX_SIZE_viaems_console_Configuration_BoostControl    281ul


struct viaems_console_Configuration_CheckEngineLight {
  bool has_pin;
  uint32_t pin;

  bool has_lean_boost_ego;
  float lean_boost_ego;

  bool has_lean_boost_map_enable;
  float lean_boost_map_enable;

};

bool pb_encode_viaems_console_Configuration_CheckEngineLight(const struct viaems_console_Configuration_CheckEngineLight *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_CheckEngineLight_to_buffer(uint8_t buffer[16], const struct viaems_console_Configuration_CheckEngineLight *msg);
unsigned pb_sizeof_viaems_console_Configuration_CheckEngineLight(const struct viaems_console_Configuration_CheckEngineLight *msg);

bool pb_decode_viaems_console_Configuration_CheckEngineLight(struct viaems_console_Configuration_CheckEngineLight *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_CheckEngineLight_pin     1ul
#define PB_TAG_viaems_console_Configuration_CheckEngineLight_lean_boost_ego     2ul
#define PB_TAG_viaems_console_Configuration_CheckEngineLight_lean_boost_map_enable     3ul
#define PB_MAX_SIZE_viaems_console_Configuration_CheckEngineLight    16ul


struct viaems_console_Configuration_RpmCut {
  bool has_rpm_limit_start;
  float rpm_limit_start;

  bool has_rpm_limit_stop;
  float rpm_limit_stop;

};

bool pb_encode_viaems_console_Configuration_RpmCut(const struct viaems_console_Configuration_RpmCut *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_RpmCut_to_buffer(uint8_t buffer[10], const struct viaems_console_Configuration_RpmCut *msg);
unsigned pb_sizeof_viaems_console_Configuration_RpmCut(const struct viaems_console_Configuration_RpmCut *msg);

bool pb_decode_viaems_console_Configuration_RpmCut(struct viaems_console_Configuration_RpmCut *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_RpmCut_rpm_limit_start     1ul
#define PB_TAG_viaems_console_Configuration_RpmCut_rpm_limit_stop     2ul
#define PB_MAX_SIZE_viaems_console_Configuration_RpmCut    10ul


struct viaems_console_Configuration_Debug {
  bool has_test_trigger_enabled;
  bool test_trigger_enabled;

  bool has_test_trigger_rpm;
  float test_trigger_rpm;

};

bool pb_encode_viaems_console_Configuration_Debug(const struct viaems_console_Configuration_Debug *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_Debug_to_buffer(uint8_t buffer[7], const struct viaems_console_Configuration_Debug *msg);
unsigned pb_sizeof_viaems_console_Configuration_Debug(const struct viaems_console_Configuration_Debug *msg);

bool pb_decode_viaems_console_Configuration_Debug(struct viaems_console_Configuration_Debug *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_Debug_test_trigger_enabled     1ul
#define PB_TAG_viaems_console_Configuration_Debug_test_trigger_rpm     2ul
#define PB_MAX_SIZE_viaems_console_Configuration_Debug    7ul


struct viaems_console_Configuration {
  unsigned outputs_count;
  struct viaems_console_Configuration_Output outputs[16];

  unsigned triggers_count;
  struct viaems_console_Configuration_TriggerInput triggers[4];

  bool has_sensors;
  struct viaems_console_Configuration_Sensors sensors;

  bool has_ignition;
  struct viaems_console_Configuration_Ignition ignition;

  bool has_fueling;
  struct viaems_console_Configuration_Fueling fueling;

  bool has_decoder;
  struct viaems_console_Configuration_Decoder decoder;

  bool has_rpm_cut;
  struct viaems_console_Configuration_RpmCut rpm_cut;

  bool has_cel;
  struct viaems_console_Configuration_CheckEngineLight cel;

  bool has_boost_control;
  struct viaems_console_Configuration_BoostControl boost_control;

  bool has_debug;
  struct viaems_console_Configuration_Debug debug;

};

bool pb_encode_viaems_console_Configuration(const struct viaems_console_Configuration *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Configuration_to_buffer(uint8_t buffer[16205], const struct viaems_console_Configuration *msg);
unsigned pb_sizeof_viaems_console_Configuration(const struct viaems_console_Configuration *msg);

bool pb_decode_viaems_console_Configuration(struct viaems_console_Configuration *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Configuration_outputs     1ul
#define PB_TAG_viaems_console_Configuration_triggers     2ul
#define PB_TAG_viaems_console_Configuration_sensors     3ul
#define PB_TAG_viaems_console_Configuration_ignition     4ul
#define PB_TAG_viaems_console_Configuration_fueling     5ul
#define PB_TAG_viaems_console_Configuration_decoder     6ul
#define PB_TAG_viaems_console_Configuration_rpm_cut     7ul
#define PB_TAG_viaems_console_Configuration_cel     8ul
#define PB_TAG_viaems_console_Configuration_boost_control     9ul
#define PB_TAG_viaems_console_Configuration_debug     10ul
#define PB_MAX_SIZE_viaems_console_Configuration    16205ul


// Nested messages for Request

struct viaems_console_Request_Ping {
};

bool pb_encode_viaems_console_Request_Ping(const struct viaems_console_Request_Ping *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Request_Ping_to_buffer(uint8_t buffer[0], const struct viaems_console_Request_Ping *msg);
unsigned pb_sizeof_viaems_console_Request_Ping(const struct viaems_console_Request_Ping *msg);

bool pb_decode_viaems_console_Request_Ping(struct viaems_console_Request_Ping *msg, pb_read_fn r, void *user);
#define PB_MAX_SIZE_viaems_console_Request_Ping    0ul


struct viaems_console_Request_SetConfig {
  bool has_config;
  struct viaems_console_Configuration config;

};

bool pb_encode_viaems_console_Request_SetConfig(const struct viaems_console_Request_SetConfig *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Request_SetConfig_to_buffer(uint8_t buffer[16208], const struct viaems_console_Request_SetConfig *msg);
unsigned pb_sizeof_viaems_console_Request_SetConfig(const struct viaems_console_Request_SetConfig *msg);

bool pb_decode_viaems_console_Request_SetConfig(struct viaems_console_Request_SetConfig *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Request_SetConfig_config     1ul
#define PB_MAX_SIZE_viaems_console_Request_SetConfig    16208ul


struct viaems_console_Request_GetConfig {
};

bool pb_encode_viaems_console_Request_GetConfig(const struct viaems_console_Request_GetConfig *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Request_GetConfig_to_buffer(uint8_t buffer[0], const struct viaems_console_Request_GetConfig *msg);
unsigned pb_sizeof_viaems_console_Request_GetConfig(const struct viaems_console_Request_GetConfig *msg);

bool pb_decode_viaems_console_Request_GetConfig(struct viaems_console_Request_GetConfig *msg, pb_read_fn r, void *user);
#define PB_MAX_SIZE_viaems_console_Request_GetConfig    0ul


struct viaems_console_Request_FlashConfig {
};

bool pb_encode_viaems_console_Request_FlashConfig(const struct viaems_console_Request_FlashConfig *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Request_FlashConfig_to_buffer(uint8_t buffer[0], const struct viaems_console_Request_FlashConfig *msg);
unsigned pb_sizeof_viaems_console_Request_FlashConfig(const struct viaems_console_Request_FlashConfig *msg);

bool pb_decode_viaems_console_Request_FlashConfig(struct viaems_console_Request_FlashConfig *msg, pb_read_fn r, void *user);
#define PB_MAX_SIZE_viaems_console_Request_FlashConfig    0ul


struct viaems_console_Request_ResetToBootloader {
};

bool pb_encode_viaems_console_Request_ResetToBootloader(const struct viaems_console_Request_ResetToBootloader *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Request_ResetToBootloader_to_buffer(uint8_t buffer[0], const struct viaems_console_Request_ResetToBootloader *msg);
unsigned pb_sizeof_viaems_console_Request_ResetToBootloader(const struct viaems_console_Request_ResetToBootloader *msg);

bool pb_decode_viaems_console_Request_ResetToBootloader(struct viaems_console_Request_ResetToBootloader *msg, pb_read_fn r, void *user);
#define PB_MAX_SIZE_viaems_console_Request_ResetToBootloader    0ul


struct viaems_console_Request_FirmwareInfo {
};

bool pb_encode_viaems_console_Request_FirmwareInfo(const struct viaems_console_Request_FirmwareInfo *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Request_FirmwareInfo_to_buffer(uint8_t buffer[0], const struct viaems_console_Request_FirmwareInfo *msg);
unsigned pb_sizeof_viaems_console_Request_FirmwareInfo(const struct viaems_console_Request_FirmwareInfo *msg);

bool pb_decode_viaems_console_Request_FirmwareInfo(struct viaems_console_Request_FirmwareInfo *msg, pb_read_fn r, void *user);
#define PB_MAX_SIZE_viaems_console_Request_FirmwareInfo    0ul


struct viaems_console_Request {
  uint32_t id;

  unsigned which_request;
  union {
    struct viaems_console_Request_Ping ping;
    struct viaems_console_Request_FirmwareInfo fwinfo;
    struct viaems_console_Request_SetConfig setconfig;
    struct viaems_console_Request_GetConfig getconfig;
    struct viaems_console_Request_FlashConfig flashconfig;
    struct viaems_console_Request_ResetToBootloader resettobootloader;
  } request;

};

bool pb_encode_viaems_console_Request(const struct viaems_console_Request *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Request_to_buffer(uint8_t buffer[16217], const struct viaems_console_Request *msg);
unsigned pb_sizeof_viaems_console_Request(const struct viaems_console_Request *msg);

bool pb_decode_viaems_console_Request(struct viaems_console_Request *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Request_id     1ul
#define PB_TAG_viaems_console_Request_ping     2ul
#define PB_TAG_viaems_console_Request_fwinfo     3ul
#define PB_TAG_viaems_console_Request_setconfig     4ul
#define PB_TAG_viaems_console_Request_getconfig     5ul
#define PB_TAG_viaems_console_Request_flashconfig     6ul
#define PB_TAG_viaems_console_Request_resettobootloader     7ul
#define PB_MAX_SIZE_viaems_console_Request    16217ul


// Nested messages for Response

struct viaems_console_Response_Pong {
};

bool pb_encode_viaems_console_Response_Pong(const struct viaems_console_Response_Pong *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Response_Pong_to_buffer(uint8_t buffer[0], const struct viaems_console_Response_Pong *msg);
unsigned pb_sizeof_viaems_console_Response_Pong(const struct viaems_console_Response_Pong *msg);

bool pb_decode_viaems_console_Response_Pong(struct viaems_console_Response_Pong *msg, pb_read_fn r, void *user);
#define PB_MAX_SIZE_viaems_console_Response_Pong    0ul


struct viaems_console_Response_SetConfig {
  bool has_config;
  struct viaems_console_Configuration config;

  bool success;

  bool requires_restart;

};

bool pb_encode_viaems_console_Response_SetConfig(const struct viaems_console_Response_SetConfig *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Response_SetConfig_to_buffer(uint8_t buffer[16212], const struct viaems_console_Response_SetConfig *msg);
unsigned pb_sizeof_viaems_console_Response_SetConfig(const struct viaems_console_Response_SetConfig *msg);

bool pb_decode_viaems_console_Response_SetConfig(struct viaems_console_Response_SetConfig *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Response_SetConfig_config     1ul
#define PB_TAG_viaems_console_Response_SetConfig_success     2ul
#define PB_TAG_viaems_console_Response_SetConfig_requires_restart     3ul
#define PB_MAX_SIZE_viaems_console_Response_SetConfig    16212ul


struct viaems_console_Response_GetConfig {
  bool has_config;
  struct viaems_console_Configuration config;

  bool needs_flash;

};

bool pb_encode_viaems_console_Response_GetConfig(const struct viaems_console_Response_GetConfig *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Response_GetConfig_to_buffer(uint8_t buffer[16210], const struct viaems_console_Response_GetConfig *msg);
unsigned pb_sizeof_viaems_console_Response_GetConfig(const struct viaems_console_Response_GetConfig *msg);

bool pb_decode_viaems_console_Response_GetConfig(struct viaems_console_Response_GetConfig *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Response_GetConfig_config     1ul
#define PB_TAG_viaems_console_Response_GetConfig_needs_flash     2ul
#define PB_MAX_SIZE_viaems_console_Response_GetConfig    16210ul


struct viaems_console_Response_FlashConfig {
  bool success;

};

bool pb_encode_viaems_console_Response_FlashConfig(const struct viaems_console_Response_FlashConfig *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Response_FlashConfig_to_buffer(uint8_t buffer[2], const struct viaems_console_Response_FlashConfig *msg);
unsigned pb_sizeof_viaems_console_Response_FlashConfig(const struct viaems_console_Response_FlashConfig *msg);

bool pb_decode_viaems_console_Response_FlashConfig(struct viaems_console_Response_FlashConfig *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Response_FlashConfig_success     1ul
#define PB_MAX_SIZE_viaems_console_Response_FlashConfig    2ul


struct viaems_console_Response_FirmwareInfo {
  struct { char str[64]; unsigned len; } version;

  struct { char str[32]; unsigned len; } platform;

  struct { char str[16384]; unsigned len; } proto;

};

bool pb_encode_viaems_console_Response_FirmwareInfo(const struct viaems_console_Response_FirmwareInfo *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Response_FirmwareInfo_to_buffer(uint8_t buffer[16488], const struct viaems_console_Response_FirmwareInfo *msg);
unsigned pb_sizeof_viaems_console_Response_FirmwareInfo(const struct viaems_console_Response_FirmwareInfo *msg);

bool pb_decode_viaems_console_Response_FirmwareInfo(struct viaems_console_Response_FirmwareInfo *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Response_FirmwareInfo_version     1ul
#define PB_TAG_viaems_console_Response_FirmwareInfo_platform     2ul
#define PB_TAG_viaems_console_Response_FirmwareInfo_proto     3ul
#define PB_MAX_SIZE_viaems_console_Response_FirmwareInfo    16488ul


struct viaems_console_Response {
  uint32_t id;

  unsigned which_response;
  union {
    struct viaems_console_Response_Pong pong;
    struct viaems_console_Response_FirmwareInfo fwinfo;
    struct viaems_console_Response_SetConfig setconfig;
    struct viaems_console_Response_GetConfig getconfig;
    struct viaems_console_Response_FlashConfig flashconfig;
  } response;

};

bool pb_encode_viaems_console_Response(const struct viaems_console_Response *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Response_to_buffer(uint8_t buffer[16498], const struct viaems_console_Response *msg);
unsigned pb_sizeof_viaems_console_Response(const struct viaems_console_Response *msg);

bool pb_decode_viaems_console_Response(struct viaems_console_Response *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Response_id     1ul
#define PB_TAG_viaems_console_Response_pong     2ul
#define PB_TAG_viaems_console_Response_fwinfo     3ul
#define PB_TAG_viaems_console_Response_setconfig     4ul
#define PB_TAG_viaems_console_Response_getconfig     5ul
#define PB_TAG_viaems_console_Response_flashconfig     6ul
#define PB_MAX_SIZE_viaems_console_Response    16498ul


struct viaems_console_Message {
  unsigned which_msg;
  union {
    struct viaems_console_EngineUpdate engine_update;
    struct viaems_console_Event event;
    struct viaems_console_Request request;
    struct viaems_console_Response response;
  } msg;

};

bool pb_encode_viaems_console_Message(const struct viaems_console_Message *msg, pb_write_fn w, void *user);
unsigned pb_encode_viaems_console_Message_to_buffer(uint8_t buffer[16502], const struct viaems_console_Message *msg);
unsigned pb_sizeof_viaems_console_Message(const struct viaems_console_Message *msg);

bool pb_decode_viaems_console_Message(struct viaems_console_Message *msg, pb_read_fn r, void *user);
#define PB_TAG_viaems_console_Message_engine_update     2ul
#define PB_TAG_viaems_console_Message_event     3ul
#define PB_TAG_viaems_console_Message_request     4ul
#define PB_TAG_viaems_console_Message_response     5ul
#define PB_MAX_SIZE_viaems_console_Message    16502ul


#endif // VIAPB_SRC_PROTO_CONSOLE_PB_H


