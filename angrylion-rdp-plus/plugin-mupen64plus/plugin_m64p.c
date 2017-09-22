#include "gfx_m64p.h"

#include "core/rdram.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DP_INTERRUPT    0x20

static uint32_t rdram_size;
static uint8_t* rdram_hidden_bits;

static char filter_char(char c)
{
    if (isalnum(c) || c == '_' || c == '-' || c == '.') {
        return c;
    } else {
        return ' ';
    }
}

static void plugin_init(void)
{
    rdram_size = 0x800000;

    // Zilmar plugins also can't access the hidden bits, so allocate it on our own
    rdram_hidden_bits = malloc(rdram_size);
    memset(rdram_hidden_bits, 3, rdram_size);
}

static void plugin_sync_dp(void)
{
    *gfx.MI_INTR_REG |= DP_INTERRUPT;
    gfx.CheckInterrupts();
}

static uint32_t** plugin_get_dp_registers(void)
{
    // HACK: this only works because the ordering of registers in GFX_INFO is
    // the same as in dp_register
    return (uint32_t**)&gfx.DPC_START_REG;
}

static uint32_t** plugin_get_vi_registers(void)
{
    // HACK: this only works because the ordering of registers in GFX_INFO is
    // the same as in vi_register
    return (uint32_t**)&gfx.VI_STATUS_REG;
}

static uint8_t* plugin_get_rdram(void)
{
    return gfx.RDRAM;
}

static uint8_t* plugin_get_rdram_hidden(void)
{
    return rdram_hidden_bits;
}

static uint32_t plugin_get_rdram_size(void)
{
    return rdram_size;
}

static uint8_t* plugin_get_dmem(void)
{
    return gfx.DMEM;
}

static uint32_t plugin_get_rom_name(char* name, uint32_t name_size)
{
    if (name_size < 21) {
        // buffer too small
        return 0;
    }

    // copy game name from ROM header, which is encoded in Shift_JIS.
    // most games just use the ASCII subset, so filter out the rest.
    int i = 0;
    for (; i < 20; i++) {
        name[i] = filter_char(gfx.HEADER[(32 + i) ^ BYTE_ADDR_XOR]);
    }

    // make sure there's at least one whitespace that will terminate the string
    // below
    name[i] = ' ';

    // trim trailing whitespaces
    for (; i > 0; i--) {
        if (name[i] != ' ') {
            break;
        }
        name[i] = 0;
    }

    // game title is empty or invalid, use safe fallback using the four-character
    // game ID
    if (i == 0) {
        for (; i < 4; i++) {
            name[i] = filter_char(gfx.HEADER[(59 + i) ^ BYTE_ADDR_XOR]);
        }
        name[i] = 0;
    }

    return i;
}

static void plugin_close(void)
{
    if (rdram_hidden_bits) {
        free(rdram_hidden_bits);
        rdram_hidden_bits = NULL;
    }
}

void plugin_mupen64plus(struct plugin_api* api)
{
    api->init = plugin_init;
    api->sync_dp = plugin_sync_dp;
    api->get_dp_registers = plugin_get_dp_registers;
    api->get_vi_registers = plugin_get_vi_registers;
    api->get_rdram = plugin_get_rdram;
    api->get_rdram_hidden = plugin_get_rdram_hidden;
    api->get_rdram_size = plugin_get_rdram_size;
    api->get_dmem = plugin_get_dmem;
    api->get_rom_name = plugin_get_rom_name;
    api->close = plugin_close;
}
