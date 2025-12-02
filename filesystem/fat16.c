#include "./fat16.h"
#include "../drivers/virtio.h"
#include "../kernel.h"
#include <stdint.h>

uint16_t fat[FAT_ENTRY_NUM];
struct dir_entry root_dir[16];

// FATボリュームの各領域を初期化
void init_fat16_disk() {
  uint8_t buf[SECTOR_SIZE];
  for (int i = 0; i < SECTOR_SIZE; i++)
    buf[i] = 0;

  // FATエントリを0埋め
  for (unsigned s = FAT1_START_SECTOR;
       s < FAT1_START_SECTOR + BPB_FATSz16 * BPB_NumFATs; s++) {
    read_write_disk(buf, s, true);
  }

  // ルートディレクトリ領域を0埋め
  for (unsigned s = ROOT_DIR_START_SECTOR;
       s < ROOT_DIR_START_SECTOR + ROOT_DIR_SECTORS; s++) {
    read_write_disk(buf, s, true);
  }

  // データ領域は必要に応じて初期化
}

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

int create_file(const char *name, uint32_t size) {
  // 1. 空きディレクトリエントリを探す
  int dir_index = -1;
  for (int i = 0; i < BPB_RootEntCnt; i++) {
    if (root_dir[i].name[0] == 0) { // 空き
      dir_index = i;
      break;
    }
  }
  if (dir_index == -1)
    return -1; // 空きなし

  // 2. 空きクラスタを探す
  int cluster = -1;
  for (int i = 2; i < FAT_ENTRY_NUM; i++) {
    if (fat[i] == 0) { // 未使用クラスタ
      cluster = i;
      break;
    }
  }
  if (cluster == -1)
    return -1; // 空きクラスタなし

  // 3. ルートディレクトリエントリに設定
  struct dir_entry *de = &root_dir[dir_index];
  // ディレクトリエントリにファイル名コピー（簡易版）
  int len = 0;
  while (len < 11 && name[len]) {
    de->name[len] = name[len];
    len++;
  }
  for (; len < 11; len++)
    de->name[len] = ' '; // パディング
  de->start_cluster = cluster;
  de->size = size;

  // 4. FAT 更新
  fat[cluster] = 0xFFFF; // 終端マーク

  // 5. ファイル内容をゼロ初期化
  uint8_t buf[BPB_BytsPerSec * BPB_SecPerClus];
  for (int i = 0; i < sizeof(buf); i++)
    buf[i] = 0;
  write_cluster(cluster, buf);

  return 0;
}
void list_root_dir() {
  for (int i = 0; i < BPB_RootEntCnt; i++) {
    struct dir_entry *de = &root_dir[i];
    if (de->name[0] == 0)
      continue; // 空きエントリスキップ

    // ファイル名文字列を作る
    char name[12];
    for (int j = 0; j < 11; j++)
      name[j] = de->name[j];
    name[11] = '\0';

    printf("%s\t%d bytes\n", name, de->size);
  }
}
