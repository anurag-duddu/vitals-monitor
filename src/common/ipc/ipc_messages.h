/**
 * @file ipc_messages.h
 * @brief IPC message definitions for inter-process communication
 *
 * This header defines the wire format for messages between:
 *   - sensor-service (publisher)
 *   - alarm-service (subscriber)
 *   - ui-app (subscriber)
 *
 * All messages are little-endian and packed (no padding).
 * Messages are sent via nanomsg pub/sub on:
 *   ipc:///tmp/vitals-monitor/vitals.ipc
 *   ipc:///tmp/vitals-monitor/waveforms.ipc
 *   ipc:///tmp/vitals-monitor/alarms.ipc
 *
 * MESSAGE FORMAT:
 *   [msg_type: uint8_t][payload_len: uint16_t][payload: N bytes]
 */

#ifndef IPC_MESSAGES_H
#define IPC_MESSAGES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 *  Message Types
 * ============================================================ */

typedef enum {
    IPC_MSG_VITALS          = 0x01,  /* Vital signs update */
    IPC_MSG_WAVEFORM        = 0x02,  /* Waveform samples */
    IPC_MSG_ALARM           = 0x03,  /* Alarm event */
    IPC_MSG_ALARM_ACK       = 0x04,  /* Alarm acknowledgment */
    IPC_MSG_SENSOR_STATUS   = 0x05,  /* Sensor connection status */
    IPC_MSG_NIBP_START      = 0x06,  /* Request NIBP measurement */
    IPC_MSG_NIBP_RESULT     = 0x07,  /* NIBP measurement complete */
} ipc_msg_type_t;

/* ============================================================
 *  Message Header (common to all messages)
 * ============================================================ */

#pragma pack(push, 1)

typedef struct {
    uint8_t  msg_type;      /* ipc_msg_type_t */
    uint16_t payload_len;   /* Length of payload following this header */
    uint8_t  version;       /* Protocol version (currently 1) */
} ipc_msg_header_t;

#define IPC_PROTOCOL_VERSION 1

/* ============================================================
 *  Vitals Message (IPC_MSG_VITALS)
 *
 *  Published by sensor-service at 1 Hz
 * ============================================================ */

typedef struct {
    ipc_msg_header_t header;

    /* Patient context */
    uint8_t  patient_slot;      /* 0 or 1 for dual-patient mode */

    /* Continuous parameters */
    int16_t  hr;                /* Heart rate (bpm), -1 = invalid */
    int16_t  spo2;              /* SpO2 (%), -1 = invalid */
    int16_t  rr;                /* Resp rate (breaths/min), -1 = invalid */
    int16_t  temp_x10;          /* Temperature * 10 (e.g., 368 = 36.8C), -1 = invalid */

    /* NIBP (may be stale) */
    int16_t  nibp_sys;          /* Systolic (mmHg), -1 = invalid */
    int16_t  nibp_dia;          /* Diastolic (mmHg), -1 = invalid */
    int16_t  nibp_map;          /* MAP (mmHg), -1 = invalid */
    uint8_t  nibp_fresh;        /* 1 if NIBP was just measured */

    /* Timestamps */
    uint64_t timestamp_ms;      /* UTC milliseconds since epoch */
    uint32_t nibp_age_s;        /* Seconds since last NIBP */

    /* Signal quality */
    uint8_t  hr_quality;        /* 0-100, 0 = no signal */
    uint8_t  spo2_quality;      /* 0-100, 0 = no signal */
    uint8_t  ecg_lead_off;      /* Bitmask: bit0=LA, bit1=RA, bit2=LL */
    uint8_t  spo2_finger_off;   /* 1 = finger not detected */

} ipc_msg_vitals_t;

/* ============================================================
 *  Waveform Message (IPC_MSG_WAVEFORM)
 *
 *  Published by sensor-service at 10-20 Hz
 * ============================================================ */

#define IPC_WAVEFORM_MAX_SAMPLES 100

typedef enum {
    IPC_WAVEFORM_ECG    = 0,
    IPC_WAVEFORM_PLETH  = 1,
    IPC_WAVEFORM_RESP   = 2,
} ipc_waveform_type_t;

typedef struct {
    ipc_msg_header_t header;

    uint8_t  patient_slot;
    uint8_t  waveform_type;     /* ipc_waveform_type_t */
    uint16_t sample_rate_hz;    /* e.g., 500 for ECG, 100 for pleth */
    uint16_t sample_count;      /* Number of valid samples */
    uint64_t timestamp_ms;      /* Timestamp of first sample */

    /* Samples are signed 16-bit, scaled appropriately:
     * ECG: microvolts
     * Pleth: arbitrary units (0-4095 typical)
     * Resp: impedance delta */
    int16_t  samples[IPC_WAVEFORM_MAX_SAMPLES];

} ipc_msg_waveform_t;

/* ============================================================
 *  Alarm Message (IPC_MSG_ALARM)
 *
 *  Published by alarm-service when alarm state changes
 * ============================================================ */

typedef enum {
    IPC_ALARM_NONE      = 0,
    IPC_ALARM_LOW       = 1,    /* Technical/advisory */
    IPC_ALARM_MEDIUM    = 2,    /* Warning */
    IPC_ALARM_HIGH      = 3,    /* Critical */
} ipc_alarm_priority_t;

typedef enum {
    IPC_ALARM_PARAM_HR          = 0,
    IPC_ALARM_PARAM_SPO2        = 1,
    IPC_ALARM_PARAM_RR          = 2,
    IPC_ALARM_PARAM_TEMP        = 3,
    IPC_ALARM_PARAM_NIBP_SYS    = 4,
    IPC_ALARM_PARAM_NIBP_DIA    = 5,
    IPC_ALARM_PARAM_TECHNICAL   = 99,
} ipc_alarm_param_t;

typedef struct {
    ipc_msg_header_t header;

    uint8_t  patient_slot;
    uint8_t  alarm_id;          /* Unique ID for this alarm instance */
    uint8_t  priority;          /* ipc_alarm_priority_t */
    uint8_t  parameter;         /* ipc_alarm_param_t */

    uint8_t  is_new;            /* 1 = new alarm, 0 = update/clear */
    uint8_t  is_acknowledged;   /* 1 = acknowledged by user */
    uint8_t  is_silenced;       /* 1 = audio silenced */
    uint8_t  reserved;

    int16_t  threshold_value;   /* Threshold that was exceeded */
    int16_t  actual_value;      /* Actual measured value */

    uint64_t timestamp_ms;      /* When alarm triggered */

    char     message[64];       /* Human-readable message */

} ipc_msg_alarm_t;

/* ============================================================
 *  Sensor Status Message (IPC_MSG_SENSOR_STATUS)
 *
 *  Published by sensor-service when sensor state changes
 * ============================================================ */

typedef enum {
    IPC_SENSOR_ECG      = 0,
    IPC_SENSOR_SPO2     = 1,
    IPC_SENSOR_NIBP     = 2,
    IPC_SENSOR_TEMP     = 3,
} ipc_sensor_type_t;

typedef enum {
    IPC_SENSOR_DISCONNECTED = 0,
    IPC_SENSOR_CONNECTED    = 1,
    IPC_SENSOR_INITIALIZING = 2,
    IPC_SENSOR_ERROR        = 3,
} ipc_sensor_state_t;

typedef struct {
    ipc_msg_header_t header;

    uint8_t  patient_slot;
    uint8_t  sensor_type;       /* ipc_sensor_type_t */
    uint8_t  state;             /* ipc_sensor_state_t */
    uint8_t  reserved;

    char     error_msg[32];     /* Error description if state == ERROR */

} ipc_msg_sensor_status_t;

#pragma pack(pop)

/* ============================================================
 *  IPC Socket Paths
 * ============================================================ */

#define IPC_SOCKET_VITALS       "ipc:///tmp/vitals-monitor/vitals.ipc"
#define IPC_SOCKET_WAVEFORMS    "ipc:///tmp/vitals-monitor/waveforms.ipc"
#define IPC_SOCKET_ALARMS       "ipc:///tmp/vitals-monitor/alarms.ipc"
#define IPC_SOCKET_CONTROL      "ipc:///tmp/vitals-monitor/control.ipc"

/* ============================================================
 *  Utility Functions
 * ============================================================ */

/**
 * Get message type name for debugging.
 */
static inline const char *ipc_msg_type_name(ipc_msg_type_t type) {
    switch (type) {
        case IPC_MSG_VITALS:        return "VITALS";
        case IPC_MSG_WAVEFORM:      return "WAVEFORM";
        case IPC_MSG_ALARM:         return "ALARM";
        case IPC_MSG_ALARM_ACK:     return "ALARM_ACK";
        case IPC_MSG_SENSOR_STATUS: return "SENSOR_STATUS";
        case IPC_MSG_NIBP_START:    return "NIBP_START";
        case IPC_MSG_NIBP_RESULT:   return "NIBP_RESULT";
        default:                    return "UNKNOWN";
    }
}

#ifdef __cplusplus
}
#endif

#endif /* IPC_MESSAGES_H */
