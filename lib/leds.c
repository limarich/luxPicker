#include "leds.h"

// Rotina para definição da intensidade de cores do LED
uint32_t matrix_rgb(uint r, uint g, uint b, float intensity)
{
    uint8_t R = (uint8_t)(r * intensity);
    uint8_t G = (uint8_t)(g * intensity);
    uint8_t B = (uint8_t)(b * intensity);
    return (G << 24) | (R << 16) | (B << 8);
}

// Rotina para acionar a matriz de LEDs - WS2812B
void draw_pio(pixel *draw, PIO pio, uint sm)
{
    for (int16_t i = 0; i < PIXELS; i++)
    {
        uint32_t valor_led = matrix_rgb(draw[i].red, draw[i].green, draw[i].blue, draw[i].intensity);
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}

// testa a matriz de leds
void test_matrix(PIO pio, uint sm)
{
    frame test_frame, black_frame;

    pixel red = {255, 0, 0, 1}, black = {0, 0, 0, 1};

    for (int16_t i = 0; i < PIXELS; i++)
    {
        test_frame[i] = red;
        black_frame[i] = black;
        draw_pio(test_frame, pio, sm);
        sleep_ms(50);
    }
    draw_pio(black_frame, pio, sm);
    sleep_ms(50);
}

// determina o padrão do pixel baseado na cor passada
pixel handle_color(color_options color, float intensity)
{
    pixel pixel_color = {0, 0, 0};
    switch (color)
    {
    case BLACK:
        pixel_color.red = 0;
        pixel_color.green = 0;
        pixel_color.blue = 0;
        pixel_color.intensity = intensity;
        break;
    case BROWN:
        pixel_color.red = 165;
        pixel_color.green = 25;
        pixel_color.blue = 0;
        pixel_color.intensity = intensity;
        break;
    case RED:
        pixel_color.red = 255;
        pixel_color.green = 0;
        pixel_color.blue = 0;
        pixel_color.intensity = intensity;
        break;
    case ORANGE:
        pixel_color.red = 205;
        pixel_color.green = 43;
        pixel_color.blue = 0;
        pixel_color.intensity = intensity;
        break;
    case YELLOW:
        pixel_color.red = 255;
        pixel_color.green = 255;
        pixel_color.blue = 0;
        pixel_color.intensity = intensity;
        break;
    case GREEN:
        pixel_color.red = 0;
        pixel_color.green = 255;
        pixel_color.blue = 0;
        pixel_color.intensity = intensity;
        break;
    case BLUE:
        pixel_color.red = 0;
        pixel_color.green = 0;
        pixel_color.blue = 255;
        pixel_color.intensity = intensity;
        break;
    case VIOLET:
        pixel_color.red = 121;
        pixel_color.green = 8;
        pixel_color.blue = 205;
        pixel_color.intensity = intensity;
        break;
    case GRAY:
        pixel_color.red = 55;
        pixel_color.green = 55;
        pixel_color.blue = 55;
        pixel_color.intensity = intensity;
        break;
    case WHITE:
        pixel_color.red = 255;
        pixel_color.green = 255;
        pixel_color.blue = 255;
        pixel_color.intensity = intensity;
        break;
    default:
        break;
    }

    return pixel_color;
}

// colore a matriz com a cor especificada
void draw_color(PIO pio, uint sm, pixel pixel_color)
{

    frame matrix;

    // uint a matriz com a cor preta
    for (int16_t i = 0; i < PIXELS; i++)
    {
        matrix[i] = handle_color(BLACK, 1);
    }

    // Preenche os LEDs para o nível de água
    for (int i = 0; i < PIXELS; i++)
    {
        matrix[i] = pixel_color;
    }

    draw_pio(matrix, pio, sm);
}