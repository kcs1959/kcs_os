#include "fat16.h"
#include "virtio.h"

static void read_fat_from_disk(void);
static void write_fat_to_disk(void);

static void write_bpb_to_disk(void) {
  uint8_t buf[SECTOR_SIZE];
  for (int i = 0; i < SECTOR_SIZE; i++)
    buf[i] = 0;

  struct bpb_fat16 *bpb = (struct bpb_fat16 *)buf;

  bpb->jmpBoot[0] = 0xEB;
  bpb->jmpBoot[1] = 0x3C;
  bpb->jmpBoot[2] = 0x90;

  memcpy(bpb->OEMName, "KCSOS   ", 8);

  bpb->BytsPerSec = BPB_BytsPerSec;
  bpb->SecPerClus = BPB_SecPerClus;
  bpb->RsvdSecCnt = BPB_RsvdSecCnt;
  bpb->NumFATs = BPB_NumFATs;
  bpb->RootEntCnt = BPB_RootEntCnt;

  bpb->TotSec16 = BPB_TotSec16;
  bpb->Media = 0xF8; // ハードディスク
  bpb->FATSz16 = BPB_FATSz16;
  bpb->SecPerTrk = 32;
  bpb->NumHeads = 64;
  bpb->HiddSec = 0;  // 隠しセクタ数
  bpb->TotSec32 = 0; // TotSec16を参照

  bpb->DrvNum = 0x80;  // 主ディスク
  bpb->Reserved1 = 0;  // 予約領域
  bpb->BootSig = 0x29; // 拡張ブートシグネチャ
  bpb->VolID = rand(); // ボリュームシリアル番号
  memcpy(bpb->VolLab, "KCS_OS     ", 11);
  memcpy(bpb->FilSysType, "FAT16   ", 8);

  // 末尾シグネチャ
  buf[510] = 0x55;
  buf[511] = 0xAA;

  // セクタ0に書き込み
  read_write_disk(buf, 0, true);
}

void init_fat16_disk(void) {
  uint8_t buf[SECTOR_SIZE];

  // ブートセクタを書き込む
  write_bpb_to_disk();

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

  // FAT の予約エントリ (0,1) を埋めておく
  read_fat_from_disk();
  fat[0] = 0xFFF8; // media + reserved bits
  fat[1] = 0xFFFF; // reserved
  write_fat_to_disk();
}

// RAM上のFATとルートディレクトリ
uint16_t fat[FAT_ENTRY_NUM];
struct dir_entry root_dir[BPB_RootEntCnt];

// データ領域の読み書き
static inline uint32_t cluster_to_sector(uint16_t cluster) {
  return DATA_START_SECTOR + (cluster - 2) * BPB_SecPerClus;
}

void read_cluster(uint16_t cluster, void *buf) {
  for (int i = 0; i < BPB_SecPerClus; i++) {
    read_write_disk((uint8_t *)buf + i * BPB_BytsPerSec,
                    cluster_to_sector(cluster) + i, 0);
  }
}

void write_cluster(uint16_t cluster, void *buf) {
  for (int i = 0; i < BPB_SecPerClus; i++) {
    read_write_disk((uint8_t *)buf + i * BPB_BytsPerSec,
                    cluster_to_sector(cluster) + i, 1);
  }
}

// ルートディレクトリの読み書き
static void read_root_dir_from_disk(void) {
  for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
    read_write_disk(&root_dir[i * (BPB_BytsPerSec / 32)],
                    ROOT_DIR_START_SECTOR + i, 0);
  }
}

static void write_root_dir_to_disk(void) {
  for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
    read_write_disk(&root_dir[i * (BPB_BytsPerSec / 32)],
                    ROOT_DIR_START_SECTOR + i, 1);
  }
}

// FAT領域の読み書き
static void read_fat_from_disk(void) {
  for (int i = 0; i < BPB_FATSz16; i++) {
    read_write_disk(&fat[i * (BPB_BytsPerSec / 2)], FAT1_START_SECTOR + i, 0);
  }
}

static void write_fat_to_disk(void) {
  // FAT1 書き戻し
  for (int i = 0; i < BPB_FATSz16; i++) {
    read_write_disk(&fat[i * (BPB_BytsPerSec / 2)], FAT1_START_SECTOR + i, 1);
  }

  // FAT2 書き戻し（ミラー）
  for (int i = 0; i < BPB_FATSz16; i++) {
    read_write_disk(&fat[i * (BPB_BytsPerSec / 2)], FAT2_START_SECTOR + i, 1);
  }
}

int create_file(const char *name, const uint8_t *data, uint32_t size) {
  // FAT / root_dir 読み込み
  read_fat_from_disk();
  read_root_dir_from_disk();

  // root_dir 空きエントリ探索
  int entry_index = -1;
  for (int i = 0; i < BPB_RootEntCnt; i++) {
    if (root_dir[i].name[0] == 0x00 || root_dir[i].name[0] == 0xE5) {
      entry_index = i;
      break;
    }
  }
  if (entry_index < 0) {
    kprintf("[FAT16] ERROR: Root directory is full. Cannot create new file.\n");
    return -1;
  }

  // 最初のクラスタ確保
  int free_cluster = -1;
  for (int i = 2; i < FAT_ENTRY_NUM; i++) {
    if (fat[i] == 0x0000) {
      free_cluster = i;
      break;
    }
  }
  if (free_cluster < 0) {
    kprintf("[FAT16] ERROR: no free FAT cluster.\n");
    return -1;
  }

  struct dir_entry *de = &root_dir[entry_index];
  memset(de->name, ' ', 8);
  memset(de->ext, ' ', 3);

  int n = 0;
  while (n < 8 && name[n] && name[n] != '.') {
    de->name[n] = name[n];
    n++;
  }
  if (name[n] == '.') {
    n++;
    for (int e = 0; e < 3 && name[n + e]; e++) {
      de->ext[e] = name[n + e];
    }
  }

  de->start_cluster = free_cluster;
  de->size = size;

  // データ書き込み
  uint32_t remaining = size;
  uint16_t cluster = free_cluster;
  uint8_t cluster_buf[BPB_BytsPerSec * BPB_SecPerClus];

  while (remaining > 0) {
    uint32_t to_write = remaining;
    if (to_write > BPB_BytsPerSec * BPB_SecPerClus)
      to_write = BPB_BytsPerSec * BPB_SecPerClus;

    if (data) {
      memcpy(cluster_buf, data, to_write);
      data += to_write;
    } else {
      memset(cluster_buf, 0, to_write);
    }
    if (to_write < BPB_BytsPerSec * BPB_SecPerClus)
      memset(cluster_buf + to_write, 0,
             BPB_BytsPerSec * BPB_SecPerClus - to_write);

    write_cluster(cluster, cluster_buf);
    remaining -= to_write;

    if (remaining > 0) {
      // 次クラスタを確保
      uint16_t next_cluster = 0;
      for (uint16_t i = 2; i < FAT_ENTRY_NUM; i++) {
        if (fat[i] == 0x0000) {
          next_cluster = i;
          break;
        }
      }
      if (next_cluster == 0) {
        kprintf("[FAT16] ERROR: not enough clusters.\n");
        return -1;
      }
      fat[cluster] = next_cluster;
      fat[next_cluster] = 0xFFFF;
      cluster = next_cluster;
    } else {
      fat[cluster] = 0xFFFF; // 最後のクラスタ
    }
  }

  // FAT書き戻し
  write_fat_to_disk();
  write_root_dir_to_disk();

  kprintf("[FAT16] File created: %s at entry %d, cluster %d\n", name,
          entry_index, free_cluster);
  return 0;
}

void fat16_list_root_dir(void) {
  // 1. ディスクから最新の root_dir を読み込む
  read_root_dir_from_disk();

  kprintf("=== Root Directory ===\n");

  for (int i = 0; i < BPB_RootEntCnt; i++) {
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
    kprintf("%s  size=", name);
    kprintf("%d", (int)root_dir[i].size);
    kprintf("  cluster=");
    kprintf("%d\n", (int)root_dir[i].start_cluster);
  }
}

// ファイル読み込み
int read_file(uint16_t start_cluster, uint8_t *buf, uint32_t size) {
  read_fat_from_disk();

  if (start_cluster < 2 || start_cluster >= FAT_ENTRY_NUM)
    return -1;

  uint32_t remaining = size;
  uint16_t cluster = start_cluster;
  uint8_t cluster_buf[BPB_BytsPerSec * BPB_SecPerClus];

  while (cluster != 0xFFFF && remaining > 0) {
    read_cluster(cluster, cluster_buf);

    uint32_t to_copy = remaining;
    if (to_copy > BPB_BytsPerSec * BPB_SecPerClus)
      to_copy = BPB_BytsPerSec * BPB_SecPerClus;

    memcpy(buf, cluster_buf, to_copy);
    buf += to_copy;
    remaining -= to_copy;

    cluster = fat[cluster];
  }

  return 0;
}

void fat16_concatenate_first_file(void) {
  // 1. 最新の FAT と root_dir を読み込む（FAT を必ず先に）
  read_fat_from_disk();
  read_root_dir_from_disk();

  // 2. 最初の有効エントリを探す
  struct dir_entry *target = NULL;
  for (int i = 0; i < 16; i++) {
    if (root_dir[i].name[0] == 0x00)
      break; // 以降は空
    if (root_dir[i].name[0] == 0xE5)
      continue; // 削除済み
    target = &root_dir[i];
    break;
  }

  if (!target) {
    kprintf("[cat] no file.\n");
    return;
  }

  // サイズ0なら空ファイル
  if (target->size == 0) {
    kprintf("[cat] (empty file)\n");
    return;
  }

  // 3. ファイルサイズぶんのバッファを確保
  uint32_t size = target->size;
  uint8_t buf[size]; // ※簡易実装としてスタック確保

  // 4. read_file() でデータ領域を読む
  if (read_file(target->start_cluster, buf, size) < 0) {
    kprintf("[cat] read error.\n");
    return;
  }

  // 5. ファイル内容をそのまま表示
  kprintf("===== cat: file content =====\n");
  for (uint32_t i = 0; i < size; i++) {
    kputchar(buf[i]);
  }
  kprintf("\n===== end =====\n");
}
