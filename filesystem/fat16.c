#include "./fat16.h"
#include "../drivers/virtio.h"
#include "../kernel.h"

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

// RAM上のFATとルートディレクトリ
uint16_t fat[FAT_ENTRY_NUM];
struct dir_entry root_dir[16]; // ここでは16個だけ利用する簡易版

// FAT1/FAT2 の読み書き
static void read_fat_from_disk() {
  for (int i = 0; i < BPB_FATSz16; i++) {
    read_write_disk(&fat[i * (BPB_BytsPerSec / 2)], FAT1_START_SECTOR + i, 0);
  }
}

static void write_fat_to_disk() {
  // FAT1 書き戻し
  for (int i = 0; i < BPB_FATSz16; i++) {
    read_write_disk(&fat[i * (BPB_BytsPerSec / 2)], FAT1_START_SECTOR + i, 1);
  }

  // FAT2 書き戻し（ミラー）
  for (int i = 0; i < BPB_FATSz16; i++) {
    read_write_disk(&fat[i * (BPB_BytsPerSec / 2)], FAT2_START_SECTOR + i, 1);
  }
}

// ルートディレクトリの読み書き
static void read_root_dir_from_disk() {
  for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
    read_write_disk(&root_dir[i * (BPB_BytsPerSec / 32)],
                    ROOT_DIR_START_SECTOR + i, 0);
  }
}

static void write_root_dir_to_disk() {
  for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
    read_write_disk(&root_dir[i * (BPB_BytsPerSec / 32)],
                    ROOT_DIR_START_SECTOR + i, 1);
  }
}
/*
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
*/

int create_file(const char *name, uint32_t size) {
  printf("[FAT16] create_file: %s (%u bytes)\n", name, size);

  // 1. FAT 読み込み
  read_fat_from_disk();

  // 2. root directory 読み込み
  read_root_dir_from_disk();

  // 3. root_dir から空きエントリ探す
  int entry_index = -1;
  for (int i = 0; i < 16; i++) {
    if (root_dir[i].name[0] == 0x00 || root_dir[i].name[0] == 0xE5) {
      entry_index = i;
      break;
    }
  }
  if (entry_index < 0) {
    printf("[FAT16] ERROR: root directory full.\n");
    return -1;
  }

  // 4. FAT から空きクラスタを探す
  int free_cluster = -1;
  for (int i = 2; i < FAT_ENTRY_NUM; i++) {
    if (fat[i] == 0x0000) {
      free_cluster = i;
      break;
    }
  }
  if (free_cluster < 0) {
    printf("[FAT16] ERROR: no free FAT cluster.\n");
    return -1;
  }

  // 5. root_dir エントリの作成
  struct dir_entry *de = &root_dir[entry_index];

  // ファイル名8 + 拡張子3 (簡易実装: '.'無視)
  memset(de->name, ' ', 8);
  memset(de->ext, ' ', 3);

  int n = 0;
  while (n < 8 && name[n] && name[n] != '.') {
    de->name[n] = name[n];
    n++;
  }
  // 拡張子
  if (name[n] == '.') {
    n++;
    for (int e = 0; e < 3 && name[n + e]; e++) {
      de->ext[e] = name[n + e];
    }
  }

  de->start_cluster = free_cluster;
  de->size = size;

  // 6. FAT を更新
  fat[free_cluster] = 0xFFFF; // end of cluster chain

  // 7. FAT をディスクへ書き戻す（FAT1 & FAT2）
  write_fat_to_disk();

  // 8. root_dir を書き戻す
  write_root_dir_to_disk();

  printf("[FAT16] File created: %s at entry %d, cluster %d\n", name,
         entry_index, free_cluster);

  return 0;
}
void list_root_dir() {
  // 1. ディスクから最新の root_dir を読み込む
  read_root_dir_from_disk();

  printf("=== Root Directory ===\n");

  for (int i = 0; i < 16; i++) {
    // 未使用エントリ → ここから先は全部空
    if (root_dir[i].name[0] == 0x00) {
      break;
    }
    // 削除済み
    if (root_dir[i].name[0] == 0xE5) {
      continue;
    }

    // 2. ファイル名（8 + 3）を組み立て
    char name[13];
    int p = 0;

    // name（8文字）
    for (int j = 0; j < 8; j++) {
      if (root_dir[i].name[j] != ' ')
        name[p++] = root_dir[i].name[j];
    }

    // 拡張子
    if (root_dir[i].ext[0] != ' ') {
      name[p++] = '.';
      for (int j = 0; j < 3; j++) {
        if (root_dir[i].ext[j] != ' ')
          name[p++] = root_dir[i].ext[j];
      }
    }

    name[p] = '\0';

    // 3. 表示
    printf("%-12s  size=%u  cluster=%u\n", name, root_dir[i].size,
           root_dir[i].start_cluster);
  }
}
