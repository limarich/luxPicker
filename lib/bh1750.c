// bh1750.c - Implementação do driver BH1750
#include "bh1750.h"
#include "pico/stdlib.h"
#include <stddef.h>

static inline int _wr(i2c_inst_t *i2c, uint8_t addr, const uint8_t *buf, size_t len)
{
    return i2c_write_blocking(i2c, addr, buf, len, false);
}

static bool _write_cmd(bh1750_t *dev, uint8_t cmd)
{
    return (_wr(dev->i2c, dev->addr, &cmd, 1) == 1);
}

bool bh1750_power_on(bh1750_t *dev) { return _write_cmd(dev, BH1750_CMD_POWER_ON); }
bool bh1750_power_down(bh1750_t *dev) { return _write_cmd(dev, BH1750_CMD_POWER_DOWN); }
bool bh1750_reset(bh1750_t *dev) { return _write_cmd(dev, BH1750_CMD_RESET); }

bool bh1750_set_mode(bh1750_t *dev, bh1750_mode_t mode)
{
    if (!_write_cmd(dev, (uint8_t)mode))
        return false;
    dev->mode = mode;
    return true;
}

bool bh1750_set_mtreg(bh1750_t *dev, uint8_t mtreg)
{
    if (mtreg < BH1750_MTREG_MIN || mtreg > BH1750_MTREG_MAX)
        return false;
    uint8_t high = 0x40 | (mtreg >> 5);  // 01000_MT[7:5]
    uint8_t low = 0x60 | (mtreg & 0x1F); // 011_MT[4:0]
    if (!_write_cmd(dev, high))
        return false;
    if (!_write_cmd(dev, low))
        return false;
    dev->mtreg = mtreg;
    return true;
}

bool bh1750_read_raw(bh1750_t *dev, uint16_t *raw)
{
    uint8_t rx[2] = {0};
    int n = i2c_read_blocking(dev->i2c, dev->addr, rx, 2, false);
    if (n != 2)
        return false;
    // BH1750 envia MSB primeiro (Big Endian)
    uint16_t val = ((uint16_t)rx[0] << 8) | rx[1];
    if (raw)
        *raw = val;
    return true;
}

bool bh1750_read_lux(bh1750_t *dev, float *lux)
{
    uint16_t raw = 0;
    if (!bh1750_read_raw(dev, &raw))
        return false;

    // Conversão: lux = (raw / 1.2) * (69 / MTreg)
    // 1.2 é o fator do datasheet para Hi-Res1 com MTreg=69.
    float f = (float)raw / 1.2f * ((float)BH1750_MTREG_DEF / (float)dev->mtreg);

    if (lux)
        *lux = f;
    return true;
}

uint32_t bh1750_integration_time_ms(const bh1750_t *dev)
{
    // Aproximação: Hi-Res ~120 ms @ MT=69; Lo-Res ~16 ms @ MT=69
    // Escala aproximadamente ~proporcional a (MTreg / 69).
    float scale = (float)dev->mtreg / (float)BH1750_MTREG_DEF;
    uint32_t base = 120; // Hi-Res
    if (dev->mode == BH1750_CONT_LORES || dev->mode == BH1750_OT_LORES)
        base = 16;
    return (uint32_t)(base * scale + 0.5f);
}

bool bh1750_begin_default(bh1750_t *dev)
{
    if (!bh1750_power_on(dev))
        return false;
    // MTreg default = 69 (já em dev->mtreg, mas reenvia p/ garantir)
    if (!bh1750_set_mtreg(dev, BH1750_MTREG_DEF))
        return false;
    if (!bh1750_set_mode(dev, BH1750_CONT_HIRES_1))
        return false;

    // Aguarda primeira conversão
    sleep_ms(bh1750_integration_time_ms(dev));
    return true;
}
