#include "structs.h"

#include <arpa/inet.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define SOCK_ERR 1
#define FILE_ERR 2
#define ADDR_ERR 3
#define SEND_ERR 4

void delay(u64 ms) { usleep(ms * 1000); }

loadavg_reading *get_lavg_reading();
meminfo_reading *get_mi_readings();
void main_loop(const char *ip, int port);

int main(int argc, char *argv[]) {
  char *dest_ip = (argc > 1) ? argv[1] : "127.0.0.1";
  int port = 8080;

  main_loop(dest_ip, port);
  return 0;
}

void send_readings(int sockfd, struct sockaddr_in *dest_addr);

void main_loop(const char *ip, int port) {
  int packets_to_send = 60;

  int sockfd;
  struct sockaddr_in dest_addr;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sockfd < 0) {
    printf("Failed to create/connect socket\n");
    exit(SOCK_ERR);
  }

  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, ip, &dest_addr.sin_addr) <= 0) {
    printf("Invalid/unsupported address ( ONLY IPv4 )\n");
    close(sockfd);
    exit(ADDR_ERR);
  }

  while (packets_to_send > 0) {
    send_readings(sockfd, &dest_addr);
    packets_to_send--;
    delay(1000);
  }
  close(sockfd);
}

void send_readings(int sockfd, struct sockaddr_in *dest_addr) {

  meminfo_reading *mi_arr = get_mi_readings();
  loadavg_reading *lavg_reading = get_lavg_reading();

  packet pack = {.timestamp = htobe64((u64)time(NULL)),
                 .lavg_readings = serialize_lavg(lavg_reading)};
  for (int i = 0; i < 5; i++) {
    pack.mi_readings[i] = serialize_mi(mi_arr, i);
  }

  ssize_t sent_bytes = sendto(sockfd, &pack, sizeof(packet), 0,
                              (struct sockaddr *)dest_addr, sizeof(*dest_addr));

  if (sent_bytes < 0) {
    printf("Couldn't send, sendto failed\n");
    close(sockfd);
    exit(SEND_ERR);
  }

  free(mi_arr);
  free(lavg_reading);
}

loadavg_reading *get_lavg_reading() {
  FILE *loadavg = fopen("/proc/loadavg", "r");

  if (loadavg == NULL) {
    printf("Failed to open /proc/loadavg\n");
    exit(FILE_ERR);
  }

  char buffer[128];
  fgets(buffer, 128, loadavg);
  f32 min_1_t;
  f32 min_5_t;
  f32 min_15_t;
  u8 curr_proc;
  char slash;
  u16 total_proc;
  u16 pid_t;
  sscanf(buffer, "%f %f %f %" SCNu8 "%c %" SCNu16 " %" SCNu16, &min_1_t,
         &min_5_t, &min_15_t, &curr_proc, &slash, &total_proc, &pid_t);
  loadavg_reading *lavg_reading = create_lavg_reading(
      min_1_t, min_5_t, min_15_t, curr_proc, total_proc, pid_t);

  fclose(loadavg);
  return lavg_reading;
}

meminfo_reading *get_mi_readings() {
  FILE *meminfo = fopen("/proc/meminfo", "r");
  if (meminfo == NULL) {
    printf("Failed to open /proc/meminfo\n");
    exit(FILE_ERR);
  }

  meminfo_reading *mi_arr = create_mi_entries(5);

  char buffer[128];
  for (data_meminfo_read type = mem_total; type <= cached; type++) {
    u32 value;
    char garbage[32];
    fgets(buffer, 128, meminfo);
    sscanf(buffer, "%s%" SCNu32, garbage, &value);

    meminfo_reading *mi_reading = create_mi_reading(type, value);
    add_entry(mi_arr, mi_reading);
    free(mi_reading);
  }

  fclose(meminfo);
  return mi_arr;
}
