#ifndef __TIMER__H__
#define __TIMER__H__

static void (*mdelay)(uint32_t timeout_ms) = (void*)0x2e8;
static void (*udelay)(uint32_t timeout_us) = (void*)0x2bc;

#endif
