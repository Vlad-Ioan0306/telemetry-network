#pragma once

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

// --- MEMINFO ---
typedef enum data_meminfo_read {
  mem_total,
  mem_free,
  mem_available,
  buffers,
  cached
} data_meminfo_read;

typedef struct meminfo_reading meminfo_reading;

meminfo_reading *create_mi_reading(data_meminfo_read type,
                                   u32 mi_reading_value);
meminfo_reading *create_mi_entries(u32 size);
void add_entry(meminfo_reading *arr, meminfo_reading *obj);
data_meminfo_read get_mi_reading_type(meminfo_reading *obj);
u32 get_mi_reading_value(meminfo_reading *obj);

// --- LOADAVG ---
typedef enum data_lavg_read {
  min_1,
  min_5,
  min_15,
  curr_total_proc,
  pid
} data_lavg_read;

typedef struct loadavg_reading loadavg_reading;

loadavg_reading *create_lavg_reading(f32 min_1, f32 min_5, f32 min_15,
                                     u8 curr_proc, u16 total_proc, u32 pid);

// --- PACKET ---
typedef struct serialized_mi {
  u16 meminfo_reading_type;
  u32 meminfo_reading_value;
} __attribute__((packed)) s_mi;

s_mi serialize_mi(meminfo_reading *arr, int idx);

typedef struct serialized_lavg {
  u32 min_1;
  u32 min_5;
  u32 min_15;
  u8 curr_proc;
  u16 total_proc;
  u32 pid;
} __attribute__((packed)) s_lavg;

s_lavg serialize_lavg(loadavg_reading *obj);

typedef struct packet {
  u64 timestamp;
  s_mi mi_readings[5];
  s_lavg lavg_readings;
} __attribute__((packed)) packet;
