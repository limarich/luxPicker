#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "hardware/i2c.h"

// Endereços possíveis do BH1750
#define BH1750_ADDR_L 0x23
#define BH1750_ADDR_H 0x5C

// Comandos básicos
#define BH1750_CMD_POWER_DOWN 0x00
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_RESET 0x07 // requer POWER_ON antes

// Modos de medição
typedef enum
{
    BH1750_CONT_HIRES_1 = 0x10, // ~1 lx/bit, ~120 ms (MTreg=69)
    BH1750_CONT_HIRES_2 = 0x11, // ~0,5 lx/bit
    BH1750_CONT_LORES = 0x13,   // ~4 lx/bit, ~16 ms
    BH1750_OT_HIRES_1 = 0x20,
    BH1750_OT_HIRES_2 = 0x21,
    BH1750_OT_LORES = 0x23
} bh1750_mode_t;

// Faixa válida de MTreg (sensibilidade)
#define BH1750_MTREG_MIN 31
#define BH1750_MTREG_DEF 69
#define BH1750_MTREG_MAX 254

typedef struct
{
    i2c_inst_t *i2c;
    uint8_t addr;
    uint8_t mtreg; // sensibilidade (default 69)
    bh1750_mode_t mode;
} bh1750_t;

// ---- API de alto nível ----
static inline void bh1750_create(bh1750_t *dev, i2c_inst_t *i2c, uint8_t addr)
{
    dev->i2c = i2c;
    dev->addr = addr;
    dev->mtreg = BH1750_MTREG_DEF;
    dev->mode = BH1750_CONT_HIRES_1;
}

bool bh1750_power_on(bh1750_t *dev);
bool bh1750_power_down(bh1750_t *dev);
bool bh1750_reset(bh1750_t *dev);

bool bh1750_set_mode(bh1750_t *dev, bh1750_mode_t mode);
// Ajusta MTreg (31..254). Retorna false se fora do intervalo.
bool bh1750_set_mtreg(bh1750_t *dev, uint8_t mtreg);

// Lê a conversão crua (16 bits) do BH1750 (Big Endian no fio).
bool bh1750_read_raw(bh1750_t *dev, uint16_t *raw);

// Converte para lux (float). Fórmula válida para qualquer MTreg/modo contínuo.
// lux = (raw / 1.2) * (69 / MTreg)
bool bh1750_read_lux(bh1750_t *dev, float *lux);

// Inicialização “padrão”: POWER_ON, MTreg=69, CONT_HIRES_1 e pequeno delay inicial.
bool bh1750_begin_default(bh1750_t *dev);
