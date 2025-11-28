#pragma once

#include "../kernel.h"

#define SECTOR_SIZE 512
#define CLUSTER_SIZE 1
#define FAT_ENTRY_NUM 4096

extern uint16_t fat[FAT_ENTRY_NUM];
struct dir_entry {
  char name[8];
  char ext[3];
  uint8_t attr;
  uint16_t start_cluster;
  uint32_t size;
};

extern struct dir_entry root_dir[16];

void read_cluster(uint16_t cluster, void *buf);
void write_cluster(uint16_t cluster, void *buf);
void copy_name_dynamic(char **name_field, const char *src);
int create_file(const char *name, uint32_t size);
void list_root_dir();