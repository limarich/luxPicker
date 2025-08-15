#include "gy33.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

static inline int _wr(i2c_inst_t *i2c, uint8_t addr, const uint8_t *buf, size_t len, bool nostop)
{
    return i2c_write_blocking(i2c, addr, buf, len, nostop);
}

static inline int _rd(i2c_inst_t *i2c, uint8_t addr, uint8_t *buf, size_t len, bool nostop)
{
    (void)nostop; // param mantido por simetria; o pico-sdk ignora em read
    return i2c_read_blocking(i2c, addr, buf, len, false);
}

bool gy33_write8(gy33_t *dev, uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = {reg, value};
    return (_wr(dev->i2c, dev->addr, tx, 2, false) == 2);
}

bool gy33_read16(gy33_t *dev, uint8_t reg, uint16_t *value)
{
    if (_wr(dev->i2c, dev->addr, &reg, 1, true) != 1)
        return false;
    uint8_t rx[2] = {0};
    if (_rd(dev->i2c, dev->addr, rx, 2, false) != 2)
        return false;
    *value = (uint16_t)((rx[1] << 8) | rx[0]); // LE -> 16 bits
    return true;
}

bool gy33_enable(gy33_t *dev, bool on)
{
    if (on)
    {
        if (!gy33_write8(dev, GY33_ENABLE_REG, GY33_ENABLE_PON))
            return false;
        sleep_ms(3);
        if (!gy33_write8(dev, GY33_ENABLE_REG, GY33_ENABLE_PON | GY33_ENABLE_AEN))
            return false;
        sleep_ms(3);
    }
    else
    {
        if (!gy33_write8(dev, GY33_ENABLE_REG, 0x00))
            return false;
    }
    return true;
}

bool gy33_set_integration(gy33_t *dev, uint8_t atime)
{
    return gy33_write8(dev, GY33_ATIME_REG, atime);
}

bool gy33_set_gain(gy33_t *dev, gy33_gain_t gain)
{
    return gy33_write8(dev, GY33_CONTROL_REG, (uint8_t)gain);
}

bool gy33_read_id(gy33_t *dev, uint8_t *out_id)
{
    uint16_t v;
    if (!gy33_read16(dev, GY33_ID_REG, &v))
        return false;
    if (out_id)
        *out_id = (uint8_t)(v & 0xFF);
    return true;
}

bool gy33_data_ready(gy33_t *dev, bool *ready)
{
    uint16_t st;
    if (!gy33_read16(dev, GY33_STATUS_REG, &st))
        return false;
    if (ready)
        *ready = ((st & GY33_STATUS_AVALID) != 0);
    return true;
}

bool gy33_read_color(gy33_t *dev, uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c)
{
    uint16_t _c, _r, _g, _b;
    if (!gy33_read16(dev, GY33_CDATA_REG, &_c))
        return false;
    if (!gy33_read16(dev, GY33_RDATA_REG, &_r))
        return false;
    if (!gy33_read16(dev, GY33_GDATA_REG, &_g))
        return false;
    if (!gy33_read16(dev, GY33_BDATA_REG, &_b))
        return false;
    if (c)
        *c = _c;
    if (r)
        *r = _r;
    if (g)
        *g = _g;
    if (b)
        *b = _b;
    return true;
}

bool gy33_begin_default(gy33_t *dev)
{
    if (!gy33_set_integration(dev, 0xF5))
        return false;
    if (!gy33_set_gain(dev, GY33_GAIN_1X))
        return false;
    if (!gy33_enable(dev, true))
        return false;
    return true;
}
