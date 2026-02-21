#include "sd_card.h"
#include "pin_config.h"
#include <SD_MMC.h>
#include <FS.h>
#include "HWCDC.h"

extern HWCDC USBSerial;

static void *sd_open_cb(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode) {
  (void)drv;
  const char *flags = "r";
  if (mode == LV_FS_MODE_WR) flags = "w";
  else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) flags = "rw";

  // Use stack allocation instead of String to avoid heap fragmentation
  char full_path[256];
  snprintf(full_path, sizeof(full_path), "/%s", path);

  File f = SD_MMC.open(full_path, flags);
  if (!f) {
    return NULL;
  }

  File *fp = new File(f);
  return (void *)fp;
}

static lv_fs_res_t sd_close_cb(lv_fs_drv_t *drv, void *file_p) {
  (void)drv;
  File *fp = (File *)file_p;
  fp->close();
  delete fp;
  return LV_FS_RES_OK;
}

static lv_fs_res_t sd_read_cb(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br) {
  (void)drv;
  File *fp = (File *)file_p;
  *br = fp->read((uint8_t *)buf, btr);
  return LV_FS_RES_OK;
}

static lv_fs_res_t sd_seek_cb(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence) {
  (void)drv;
  File *fp = (File *)file_p;
  SeekMode sm = SeekSet;
  if (whence == LV_FS_SEEK_CUR) sm = SeekCur;
  else if (whence == LV_FS_SEEK_END) sm = SeekEnd;
  fp->seek(pos, sm);
  return LV_FS_RES_OK;
}

static lv_fs_res_t sd_tell_cb(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p) {
  (void)drv;
  File *fp = (File *)file_p;
  *pos_p = fp->position();
  return LV_FS_RES_OK;
}

static lv_fs_drv_t sd_drv;

bool sd_card_init() {
  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA);
  if (!SD_MMC.begin("/sdcard", true)) {
    USBSerial.println("SD card mount failed");
    return false;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    USBSerial.println("No SD card attached");
    return false;
  }

  USBSerial.print("SD card size: ");
  USBSerial.print((uint32_t)(SD_MMC.cardSize() / (1024 * 1024)));
  USBSerial.println(" MB");

  lv_fs_drv_init(&sd_drv);
  sd_drv.letter = 'S';
  sd_drv.open_cb = sd_open_cb;
  sd_drv.close_cb = sd_close_cb;
  sd_drv.read_cb = sd_read_cb;
  sd_drv.seek_cb = sd_seek_cb;
  sd_drv.tell_cb = sd_tell_cb;
  lv_fs_drv_register(&sd_drv);

  USBSerial.println("SD card + LVGL FS driver initialized");
  return true;
}

void sd_card_sleep() {
  SD_MMC.end();
}

bool sd_card_wake() {
  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA);
  if (!SD_MMC.begin("/sdcard", true)) {
    USBSerial.println("SD card remount failed");
    return false;
  }
  return true;
}
