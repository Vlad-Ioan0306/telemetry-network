#include "structs.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

// --- MEMINFO ---
typedef struct meminfo_reading {
  data_meminfo_read meminfo_reading_type;
  u32 meminfo_reading_value;
} meminfo_reading;

meminfo_reading *create_mi_reading(data_meminfo_read type,
                                   u32 mi_reading_value) {
  meminfo_reading *obj = malloc(sizeof(meminfo_reading));
  obj->meminfo_reading_type = type;
  obj->meminfo_reading_value = mi_reading_value;
  return obj;
}

meminfo_reading *create_mi_entries(u32 size) {
  meminfo_reading *arr = malloc(size * sizeof(meminfo_reading));
  return arr;
}

u32 get_mi_reading_value(meminfo_reading *obj) {
  return obj->meminfo_reading_value;
}
data_meminfo_read get_mi_reading_type(meminfo_reading *obj) {
  return obj->meminfo_reading_type;
}

void add_entry(meminfo_reading *arr, meminfo_reading *obj) {
  arr[obj->meminfo_reading_type].meminfo_reading_type =
      obj->meminfo_reading_type;
  arr[obj->meminfo_reading_type].meminfo_reading_value =
      obj->meminfo_reading_value;
}

// --- LOADAVG ---
typedef struct loadavg_reading {
  f32 min_1;
  f32 min_5;
  f32 min_15;
  u8 curr_proc;
  u16 total_proc;
  u32 pid;
} loadavg_reading;

loadavg_reading *create_lavg_reading(f32 min_1, f32 min_5, f32 min_15,
                                     u8 curr_proc, u16 total_proc, u32 pid) {

  loadavg_reading *obj = malloc(sizeof(loadavg_reading));
  obj->min_1 = min_1;
  obj->min_5 = min_5;
  obj->min_15 = min_15;
  obj->curr_proc = curr_proc;
  obj->total_proc = total_proc;
  obj->pid = pid;
  return obj;
}

// --- PACKET ---
s_mi serialize_mi(meminfo_reading *arr, int idx) {
  s_mi s_obj;
  s_obj.meminfo_reading_type = arr[idx].meminfo_reading_type;
  s_obj.meminfo_reading_value = arr[idx].meminfo_reading_value;

  s_obj.meminfo_reading_value = htonl(s_obj.meminfo_reading_value);
  s_obj.meminfo_reading_type = htons(s_obj.meminfo_reading_type);
  return s_obj;
}

s_lavg serialize_lavg(loadavg_reading *obj) {
  s_lavg s_obj;
  f32 min_1 = obj->min_1;
  f32 min_5 = obj->min_5;
  f32 min_15 = obj->min_15;
  s_obj.curr_proc = obj->curr_proc;
  s_obj.total_proc = obj->total_proc;
  s_obj.pid = obj->pid;

  s_obj.min_1 = htonl((u32)(min_1 * 100));
  s_obj.min_5 = htonl((u32)(min_5 * 100));
  s_obj.min_15 = htonl((u32)(min_15 * 100));
  s_obj.total_proc = htons(s_obj.total_proc);
  s_obj.pid = htonl(s_obj.pid);

  return s_obj;
}
