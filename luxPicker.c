#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "font.h"
#include "pico/bootrom.h"

//  GY-33
#include "gy33.h"
#define I2C_GY33_PORT i2c0
#define GY33_SDA 0
#define GY33_SCL 1
#define GY33_ADDR GY33_I2C_ADDR_DEFAULT // 0x29

//  BH1750
#include "bh1750.h"
#define I2C_BH1750_PORT i2c0
#define BH1750_ADDR BH1750_ADDR_L // 0x23

// OLED
#define I2C_OLED_PORT i2c1
#define OLED_SDA 14
#define OLED_SCL 15
#define OLED_ADDR 0x3C

//  Botão para BOOTSEL
#define botaoB 6
static void gpio_irq_handler(uint gpio, uint32_t events)
{
    (void)gpio;
    (void)events;
    reset_usb_boot(0, 0);
}

int main(void)
{
    // Configura BOOTSEL no botão B
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    stdio_init_all();

    //  OLED na I2C1
    i2c_init(I2C_OLED_PORT, 400 * 1000);
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);

    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, OLED_ADDR, I2C_OLED_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    //  I2C0 para sensores
    i2c_init(I2C_GY33_PORT, 100 * 1000);
    gpio_set_function(GY33_SDA, GPIO_FUNC_I2C);
    gpio_set_function(GY33_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(GY33_SDA);
    gpio_pull_up(GY33_SCL);

    //  Inicializa GY-33
    gy33_t gy;
    gy33_create(&gy, I2C_GY33_PORT, GY33_ADDR);
    if (!gy33_begin_default(&gy))
    {
        printf("Falha ao iniciar GY-33!\n");
    }

    //  Inicializa BH1750
    bh1750_t luxm;
    bh1750_create(&luxm, I2C_BH1750_PORT, BH1750_ADDR);
    if (!bh1750_begin_default(&luxm))
    {
        printf("Falha ao iniciar BH1750!\n");
    }

    char buf_r[12], buf_g[12], buf_b[12], buf_c[12], buf_lux[16];
    bool cor = true;

    while (true)
    {
        //  Leitura GY-33
        uint16_t r = 0, g = 0, b = 0, c = 0;
        gy33_read_color(&gy, &r, &g, &b, &c);

        //  Leitura BH1750
        float lux = 0.0f;
        bh1750_read_lux(&luxm, &lux);

        //  Log serial
        printf("R=%u, G=%u, B=%u, C=%u, Lux=%.1f\n", r, g, b, c, lux);

        //  Formata strings
        snprintf(buf_r, sizeof(buf_r), "%u R", r);
        snprintf(buf_g, sizeof(buf_g), "%u G", g);
        snprintf(buf_b, sizeof(buf_b), "%u B", b);
        snprintf(buf_c, sizeof(buf_c), "%u C", c);
        snprintf(buf_lux, sizeof(buf_lux), "%.1f Lux", lux);

        //  OLED
        ssd1306_fill(&ssd, !cor);
        ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);
        ssd1306_draw_string(&ssd, "GY-33 & BH1750", 8, 6);
        ssd1306_line(&ssd, 3, 25, 123, 25, cor);
        ssd1306_draw_string(&ssd, buf_r, 5, 28);
        ssd1306_draw_string(&ssd, buf_g, 5, 40);
        ssd1306_draw_string(&ssd, buf_b, 5, 52);
        ssd1306_draw_string(&ssd, buf_c, 70, 28);
        ssd1306_draw_string(&ssd, buf_lux, 70, 40);
        ssd1306_send_data(&ssd);

        cor = !cor;
        sleep_ms(500);
    }
}
