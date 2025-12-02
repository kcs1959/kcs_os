#include "./fat16.h"
#include "../drivers/virtio.h"
#include "../kernel.h"

// 要修正
uint32_t cluster_to_sector(uint16_t cluster) {
  // データ領域開始セクタ
  uint32_t root_dir_sectors =
      (BPB_RootEntCnt * 32 + BPB_BytsPerSec - 1) / BPB_BytsPerSec;
  uint32_t data_start_sector =
      BPB_RsvdSecCnt + BPB_NumFATs * BPB_FATSz16 + root_dir_sectors;

  // クラスタ番号 → セクタ番号
  return data_start_sector + (cluster - 2) * BPB_SecPerClus;
}

// read/write_clusterはおk。
void read_cluster(uint16_t cluster, void *buf) {
  read_write_disk(buf, cluster_to_sector(cluster), false);
}

void write_cluster(uint16_t cluster, void *buf) {
  read_write_disk(buf, cluster_to_sector(cluster), true);
}

// わざわざファイル名をramに置かなくても良い
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

// 要修正
int create_file(const char *name, uint32_t size) {
  int dir_index = -1;
  for (int i = 0; i < 16; i++) {
    if (root_dir[i].name[0] == 0) {
      dir_index = i;
      break;
    }
  }
  if (dir_index == -1)
    return -1;

  int cluster = -1;
  for (int i = 2; i < FAT_ENTRY_NUM; i++) {
    if (fat[i] == 0) {
      cluster = i;
      break;
    }
  }
  if (cluster == -1)
    return -1;

  root_dir[dir_index].ext[0] = '\0';
  root_dir[dir_index].start_cluster = cluster;
  root_dir[dir_index].size = size;

  // ファイル名
  copy_name_dynamic(&root_dir[dir_index].name, name);

  fat[cluster] = 0xFFFF;

  // ファイルの中身
  paddr_t p = alloc_pages(CLUSTER_SIZE * SECTOR_SIZE);
  write_cluster(cluster, (void *)p);
  return 0;
}
void list_root_dir() {
  for (int i = 0; i < 16; i++) {
    if (root_dir[i].name[0] != '\0') {
      printf("%s\t%d bytes\n", root_dir[i].name, root_dir[i].size);
    } else {
      return;
    }
  }
}
