#pragma once

#include "../kernel.h"

// ブートセクタ
#define BPB_BytsPerSec 512
#define BPB_SecPerClus 1
#define BPB_RsvdSecCnt 1
#define BPB_NumFATs 2
#define BPB_RootEntCnt 512
#define BPB_FATSz16 32
#define BPB_TotSec16 32768

// FAT領域
#define FAT1_START_SECTOR BPB_RsvdSecCnt
#define FAT2_START_SECTOR (FAT1_START_SECTOR + BPB_FATSz16)

// ルートディレクトリ
#define ROOT_DIR_START_SECTOR (BPB_RsvdSecCnt + BPB_NumFATs * BPB_FATSz16)
#define ROOT_DIR_SECTORS                                                       \
  ((BPB_RootEntCnt * 32 + BPB_BytsPerSec - 1) / BPB_BytsPerSec)

// データ領域
#define DATA_START_SECTOR (ROOT_DIR_START_SECTOR + ROOT_DIR_SECTORS)

void read_cluster(uint16_t cluster, void *buf);
void write_cluster(uint16_t cluster, void *buf);
void copy_name_dynamic(char **name_field, const char *src);
int create_file(const char *name, uint32_t size);
void list_root_dir();