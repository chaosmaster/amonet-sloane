#ifndef GCPU_H
#define GCPU_H

#include <stdint.h>

#define GCPU_BASE   0x10210000

#ifdef sdr_read32
#define GCPU_READ_REG(reg)          sdr_read32(reg)
#else
#define GCPU_READ_REG(reg)          (*(volatile uint32_t *)(reg))
#endif

#ifdef sdr_write32
#define GCPU_WRITE_REG(reg, val)    sdr_write(reg, wal)
#else
#define GCPU_WRITE_REG(reg, val)    ((*(volatile uint32_t *)(reg)) = (val))
#endif

void gcpu_init(void);
void gcpu_uninit(void);

int gcpu_cmd(uint32_t cmd);

void gcpu_memptr_set(uint32_t offset, uint8_t *data_in);
void gcpu_memptr_get(uint32_t offset, uint32_t len, uint8_t *data_out);

int gcpu_load_hw_key(uint32_t offset);
int gcpu_aes_decrypt(uint32_t key_offset, uint32_t data_offset, uint32_t out_offset);

#endif
