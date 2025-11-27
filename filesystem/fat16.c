#include "./fat16.h"
#include "../drivers/virtio.h"
#include "../kernel.h"

uint16_t fat[FAT_ENTRY_NUM];
struct dir_entry root_dir[16];

uint32_t cluster_to_sector(uint16_t cluster) {
  uint32_t data_start =
      1 + 2 * FAT_ENTRY_NUM * 2 / SECTOR_SIZE + sizeof(root_dir) / SECTOR_SIZE;
  return data_start + (cluster - 2) * CLUSTER_SIZE;
}

void read_cluster(uint16_t cluster, void *buf) {
  read_write_disk(buf, cluster_to_sector(cluster), false);
}

void write_cluster(uint16_t cluster, void *buf) {
  read_write_disk(buf, cluster_to_sector(cluster), true);
}

void copy_name_dynamic(char **name_field, const char *src) {
  int len = 0;
  while (len < 256) {
    if (src[len] == 0)
      break;
    len++;
  }

  char *buf = (char *)alloc_pages((len + 1 + PAGE_SIZE - 1) / PAGE_SIZE);
  for (int i = 0; i <= len; i++) {
    buf[i] = src[i];
  }
  *name_field = buf;
}
