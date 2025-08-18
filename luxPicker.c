#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "font.h"
#include "pico/bootrom.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "leds.h"
#include "pio_matrix.pio.h"
#include <math.h>

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
#define BUZZER_PIN 21

const uint RED_PIN = 13;
const uint GREEN_PIN = 11;
const uint BLUE_PIN = 12;
uint buzzer_slice;

//  Botão para BOOTSEL
#define botaoB 6
static void gpio_irq_handler(uint gpio, uint32_t events)
{
    (void)gpio;
    (void)events;
    reset_usb_boot(0, 0);
}

// Função para tocar o beep
void beep(uint ms) {
    pwm_set_gpio_level(BUZZER_PIN, 2500); // 50% duty
    pwm_set_enabled(buzzer_slice, true);
    sleep_ms(ms);
    pwm_set_enabled(buzzer_slice, false);
}

static inline float clampf(float x, float lo, float hi)
{
    return x < lo ? lo : (x > hi ? hi : x);
}

// Converte R/G/B brutos + C em [0..1] usando normalização por C
static inline void rgb_from_clear(uint16_t r, uint16_t g, uint16_t b, uint16_t c,
                                  float *rf, float *gf, float *bf)
{
    float cc = (c == 0) ? 1.0f : (float)c;
    float rN = (float)r / cc, gN = (float)g / cc, bN = (float)b / cc;

    // normaliza pela componente máxima para preservar saturação visual da cor
    float m = fmaxf(rN, fmaxf(gN, bN));
    if (m > 0.0f)
    {
        rN /= m;
        gN /= m;
        bN /= m;
    }

    // gamma perceptual
    const float inv_gamma = 1.0f / 2.2f;
    *rf = powf(clampf(rN, 0.f, 1.f), inv_gamma);
    *gf = powf(clampf(gN, 0.f, 1.f), inv_gamma);
    *bf = powf(clampf(bN, 0.f, 1.f), inv_gamma);
}

// MME (exponential moving average) para suavizar brilho
static inline float ema(float prev, float now, float alpha)
{
    return prev * (1.0f - alpha) + now * alpha;
}

int main(void)
{
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    buzzer_slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_clkdiv(buzzer_slice, 4.0);      // Frequência ~10kHz audível
    pwm_set_wrap(buzzer_slice, 5000);
    pwm_set_enabled(buzzer_slice, false);   // Começa desligado

    PIO pio;
    uint sm;
    // configurações da PIO
    pio = pio0;
    uint offset = pio_add_program(pio, &pio_matrix_program);
    sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, LED_PIN);

    pixel draw[PIXELS];
    test_matrix(pio, sm);

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

    // Configuração dos pinos GPIO do LED RGB
    gpio_init(RED_PIN);
    gpio_init(GREEN_PIN);
    gpio_init(BLUE_PIN);

    gpio_set_dir(RED_PIN, GPIO_OUT);
    gpio_set_dir(GREEN_PIN, GPIO_OUT);
    gpio_set_dir(BLUE_PIN, GPIO_OUT);

    gpio_put(RED_PIN, 1);

    float smooth_brightness = 0.3f; // ponto inicial

    while (true)
    {
        //  Leitura GY-33
        uint16_t r = 0, g = 0, b = 0, c = 0;
        gy33_read_color(&gy, &r, &g, &b, &c);

        //  Leitura BH1750
        float lux = 0.0f;
        bh1750_read_lux(&luxm, &lux);

        float rf, gf, bf;
        rgb_from_clear(r, g, b, c, &rf, &gf, &bf);

        // referências de brilho e lux
        const float LUX_REF = 300.0f;
        const float C_REF = 12000.0f;

        // Normalizações individuais
        float bright_lux = clampf(lux / LUX_REF, 0.05f, 1.0f);
        float bright_c = clampf((float)c / C_REF, 0.05f, 1.0f);

        //  média geométrica do clear com o lux
        float brightness = sqrtf(bright_lux * bright_c);

        // Suavização
        smooth_brightness = ema(smooth_brightness, brightness, 0.2f);

        // Aplica brilho global à cor
        rf *= smooth_brightness;
        gf *= smooth_brightness;
        bf *= smooth_brightness;

        pixel displayed = {(int)(rf * 255), (int)(gf * 255), (int)(bf * 255), smooth_brightness};
        draw_color(pio0, sm, displayed);

        printf("rf=%.3f, gf=%.3f, bf=%.3f, C=%u, Lux=%.1f, Bright=%.3f\n",
               rf, gf, bf, c, lux, smooth_brightness);

        if(lux < 100) // Emite bips intermitentes enquanto lux estiver abaixo de 100
            beep(100);

        //  Formata strings
        snprintf(buf_r, sizeof(buf_r), "%u R", r);
        snprintf(buf_g, sizeof(buf_g), "%u G", g);
        snprintf(buf_b, sizeof(buf_b), "%u B", b);
        snprintf(buf_c, sizeof(buf_c), "%u C", c);
        snprintf(buf_lux, sizeof(buf_lux), "%.0f Lux", lux);

        //  OLED
        ssd1306_fill(&ssd, !cor);
        ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);
        ssd1306_draw_string(&ssd, "GY-33 & BH1750", 8, 6);
        ssd1306_line(&ssd, 3, 25, 123, 25, cor);
        ssd1306_draw_string(&ssd, buf_r, 5, 28);
        ssd1306_draw_string(&ssd, buf_g, 5, 40);
        ssd1306_draw_string(&ssd, buf_b, 5, 52);
        ssd1306_draw_string(&ssd, buf_c, 65, 28);
        ssd1306_draw_string(&ssd, buf_lux, 65, 40);
        ssd1306_send_data(&ssd);

        cor = !cor;
        sleep_ms(500);
    }
}
