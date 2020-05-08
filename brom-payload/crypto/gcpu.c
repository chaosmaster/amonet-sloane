#include "gcpu.h"
#include "../printf.h"

void gcpu_init(void) {
    GCPU_WRITE_REG(GCPU_BASE, GCPU_READ_REG(GCPU_BASE) & 0xfffffff0 | 0xf);
    GCPU_WRITE_REG(GCPU_BASE + 0x4, GCPU_READ_REG(GCPU_BASE + 0x4) & 0xFFFFDFF);
}

void gcpu_uninit(void) {
    GCPU_WRITE_REG(GCPU_BASE, GCPU_READ_REG(GCPU_BASE) & 0xfffffff0 | 0xf);
}

int gcpu_cmd(uint32_t cmd) {
    uint32_t ret;

    GCPU_WRITE_REG(GCPU_BASE + 0x0804, 3);
    GCPU_WRITE_REG(GCPU_BASE + 0x0808, 3);
    GCPU_WRITE_REG(GCPU_BASE + 0x0C00, cmd);
    GCPU_WRITE_REG(GCPU_BASE + 0x0400, 0);

    while(GCPU_READ_REG(GCPU_BASE + 0x0800) == 0) {}

    if(GCPU_READ_REG(GCPU_BASE + 0x0800) & 2) {
        if(GCPU_READ_REG(GCPU_BASE + 0x0800) & 1) {
            while(GCPU_READ_REG(GCPU_BASE + 0x0800) == 0) {}
            ret = -1;
            GCPU_WRITE_REG(GCPU_BASE + 0x0804, 3);
        }
    }
    else {
        while(GCPU_READ_REG(GCPU_BASE + 0x0418) & 1 == 0) {}
        ret = 0;
        GCPU_WRITE_REG(GCPU_BASE + 0x0804, 3);
    }

    return ret;
}

void gcpu_memptr_set(uint32_t offset, uint8_t *data_in) {
    int i;
    uint32_t tmp_val;

    for(i = 0; i < 16; i += 4) {
        GCPU_WRITE_REG(GCPU_BASE + 0xC00 + i + (offset * 4), ((uint32_t *)(data_in + i))[0]);
    }
}

void gcpu_memptr_get(uint32_t offset, uint32_t len, uint8_t *data_out) {
    int i;

    for(i = 0; i < len; i += 4) {
      ((uint32_t*)(data_out + i))[0] = GCPU_READ_REG(GCPU_BASE + 0xC00 + i + (offset * 4));
    }

}

int gcpu_load_hw_key(uint32_t offset) {
    GCPU_WRITE_REG(GCPU_BASE + 0xC04, 0x58);
    GCPU_WRITE_REG(GCPU_BASE + 0xC08, offset);
    GCPU_WRITE_REG(GCPU_BASE + 0xC0C, 4);

    return gcpu_cmd(0x70);
}

int gcpu_aes_decrypt(uint32_t key_offset, uint32_t data_offset, uint32_t out_offset){
    GCPU_WRITE_REG(GCPU_BASE + 0xC04, 1);
    GCPU_WRITE_REG(GCPU_BASE + 0xC08, key_offset);
    GCPU_WRITE_REG(GCPU_BASE + 0xC0C, data_offset);
    GCPU_WRITE_REG(GCPU_BASE + 0xC10, out_offset);

    return gcpu_cmd(0x78);
}
