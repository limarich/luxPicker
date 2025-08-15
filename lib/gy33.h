#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "hardware/i2c.h"

#ifndef GY33_I2C_ADDR_DEFAULT
#define GY33_I2C_ADDR_DEFAULT 0x29
#endif

enum
{
    GY33_ENABLE_REG = 0x80,
    GY33_ATIME_REG = 0x81,
    GY33_CONTROL_REG = 0x8F,
    GY33_ID_REG = 0x92,
    GY33_STATUS_REG = 0x93,
    GY33_CDATA_REG = 0x94, // Clear
    GY33_RDATA_REG = 0x96, // Red
    GY33_GDATA_REG = 0x98, // Green
    GY33_BDATA_REG = 0x9A  // Blue
};

// Bits do ENABLE
#define GY33_ENABLE_PON 0x01 // Power ON
#define GY33_ENABLE_AEN 0x02 // Enable RGBC ADC

// STATUS (bit de dado válido em TCS3472 é o AVALID=0x01)
#define GY33_STATUS_AVALID 0x01

// Ganhos típicos do CONTROL (TCS34725)
typedef enum
{
    GY33_GAIN_1X = 0x00,
    GY33_GAIN_4X = 0x01,
    GY33_GAIN_16X = 0x02,
    GY33_GAIN_60X = 0x03
} gy33_gain_t;

typedef struct
{
    i2c_inst_t *i2c;
    uint8_t addr;
} gy33_t;

// Inicializa a estrutura com instância I2C e endereço
static inline void gy33_create(gy33_t *dev, i2c_inst_t *i2c, uint8_t addr)
{
    dev->i2c = i2c;
    dev->addr = addr;
}

// Liga/desliga o sensor (PON/AEN). Se ligar, aguarda um pequeno tempo de start.
bool gy33_enable(gy33_t *dev, bool on);

// Define tempo de integração (ATIME cru). Ex.: 0xF5 ~ 2,4 ms * (256-0xF5)
bool gy33_set_integration(gy33_t *dev, uint8_t atime);

// Define ganho
bool gy33_set_gain(gy33_t *dev, gy33_gain_t gain);

// Lê ID do chip (útil para diagnóstico)
bool gy33_read_id(gy33_t *dev, uint8_t *out_id);

// Verifica se há dado válido (STATUS & AVALID)
bool gy33_data_ready(gy33_t *dev, bool *ready);

// Lê RGBC (16 bits cada, little-endian)
bool gy33_read_color(gy33_t *dev, uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c);

// Helper “setup padrão”: PON+AEN, ATIME ~ 0xF5, GAIN 1x
bool gy33_begin_default(gy33_t *dev);

bool gy33_write8(gy33_t *dev, uint8_t reg, uint8_t value);
bool gy33_read16(gy33_t *dev, uint8_t reg, uint16_t *value);
