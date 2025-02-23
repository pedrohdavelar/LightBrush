/**
Projeto Final Embarcatech
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/binary_info.h"
#include "pico/bootrom.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/adc.h"
#include "hardware/clocks.h" 
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "inc/ssd1306.h"
#include "ws2818b.pio.h"

//essa variável será responsável por ativar os prints no terminal
bool debugMode = 0; 

#pragma region //##### Inicio - Código Display OLED 1306
//Adaptado de: https://github.com/BitDogLab/BitDogLab-C/tree/main/display_oled
#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 15

uint8_t ssd[ssd1306_buffer_length];

struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};


//
#pragma endregion //###########################Fim - Código Display OLED 1306####################

#pragma region //##### Inicio - Código Neopixel
//Adaptado de: https://github.com/BitDogLab/BitDogLab-C/tree/main/neopixel_pio

//Defines para a matriz 5x5 de LEDs
#define LED_COUNT 25
#define LED_PIN 7

//Struct para controle do LED GRB
struct pixel_t{
    uint8_t R, G, B;
};

//typedef para renomear as structs de modo a facilitar a leitura
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t;

//Buffer de pixels que formam a matriz
npLED_t leds[LED_COUNT];

//Variaveis para uso da máquina PIO
PIO np_pio;
uint sm;

//Função para inicializar a máquina PIO para controle da matriz de leds
void npInit(uint pin){
    //cria programa PIO
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    //toma posse de uma máquina PIO
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm <0){
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio,true); //se nenhuma maquina estiver livre, panic!
    }
    //Inicia programa na máquina PIO obtida
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
    //Limpa buffer de pixels
    for (uint i = 0; i < LED_COUNT; ++i){
        leds[i].G = 0;
        leds[i].R = 0;
        leds[i].B = 0;
    }
}

//Funções de operação da matriz de LEDs

//Atribui uma cor RGB a um LED
void npSetLED(const uint index, uint8_t r, uint8_t g, uint8_t b){
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

//Limpa o buffer de pixels
void npClear(){
    for (uint i = 0; i < LED_COUNT; ++i){
        npSetLED(i, 0, 0, 0);
    }
}

//A matriz led recebe os dados de brilho de forma invertida. Por isso, para ajustar corretamente,
//é necessário inverter os bytes de cada cor.
//fiz via uma tabela com os valores já invertidos em ordem
//fonte: https://stackoverflow.com/questions/2602823/in-c-c-whats-the-simplest-way-to-reverse-the-order-of-bits-in-a-byte
uint8_t npReverseByte(uint8_t value){
    static const uint8_t reverseByte[] = {
        0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
        0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
        0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
        0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
        0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
        0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
        0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
        0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
        0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
        0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
        0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
        0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
        0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
        0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
        0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
        0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
        0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
        0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
        0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
        0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
        0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
        0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
        0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
        0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
        0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
        0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
        0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
        0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
        0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
        0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
        0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
        0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
    };
    return reverseByte[value];
}

//Escreve os dados do buffer nos LEDs
void npWrite(){
    //Escreve cada dado de 8 bits dos pixels em sequencia no buffer da máquina PIO
    for (uint i = 0; i < LED_COUNT; ++i){
        pio_sm_put_blocking(np_pio, sm, npReverseByte(leds[i].G));
        pio_sm_put_blocking(np_pio, sm, npReverseByte(leds[i].R));
        pio_sm_put_blocking(np_pio, sm, npReverseByte(leds[i].B));
    }
}

//Para a posição ficar ordenada, ao inves de fazer a conta do exemplo da bitdiglab, apenas gravei um array com as posicoes 
const int npLEDPositions[5][5] = {
    {24, 23, 22, 21, 20},
    {15, 16, 17, 18, 19},
    {14, 13, 12, 11, 10},
    {5, 6, 7, 8, 9},
    {4, 3, 2, 1, 0}
};

//converte uma coordenada x,y para o indice em fila do LED
int getIndex(int x, int  y){
    return npLEDPositions[y][x];
}

//A partir de um array 5x5, define uma figura para a matriz de LEDs
void npSetFigure(npLED_t figure[5][5]){
    for (uint y = 0; y < 5; ++y){
        for (uint x = 0; x < 5; ++x){
            npSetLED(getIndex(x, y), figure[y][x].R, figure[y][x].G, figure[y][x].B);
        }
    }
}

//definições para facilitar o ajuste de brilho
#define npMIN  1
#define npLOW  16
#define npMED  32
#define npHIGH 128
#define npMAX  255

//Ajusta o brilho da matriz SE o led estiver acesso E apenas para a cor que estiver acesa
void npSetBrightness(uint8_t brightness){
    for (uint i = 0; i < LED_COUNT; ++i){
        if(leds[i].R > 0) {leds[i].R =  brightness;}
        if(leds[i].G > 0) {leds[i].G =  brightness;}
        if(leds[i].B > 0) {leds[i].B =  brightness;}
    }
}

#pragma region //Figuras para a matriz de LEDs

//Definição das cores para os leds
#define npBLACK   { 0,   0,   0 }
#define npRED     { 255, 0,   0 }
#define npGREEN   { 0,   255, 0 }
#define npBLUE    { 0,   0,   255 }
#define npYELLOW  { 255, 255, 0 }
#define npCYAN    { 0,   255, 255 }
#define npMAGENTA { 255, 0,   255 }
#define npWHITE   { 255, 255, 255 }
#define npORANGE  { 255, 128, 0 }

//matriz desligada
npLED_t figOff[5][5] = {
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK}
};


//figura de um coração
npLED_t figHeart[5][5] = {
    {npBLACK, npRED, npBLACK, npRED, npBLACK},
    {npRED, npBLACK, npRED, npBLACK, npRED},
    {npRED, npBLACK, npBLACK, npBLACK, npRED},
    {npBLACK, npRED, npBLACK, npRED, npBLACK},
    {npBLACK, npBLACK, npRED, npBLACK, npBLACK}
};

//figura de um smiley
npLED_t figSmiley [5][5] = {
    {npBLACK, npYELLOW, npYELLOW, npYELLOW, npBLACK},
    {npYELLOW, npBLUE, npYELLOW, npBLUE, npYELLOW},
    {npYELLOW, npYELLOW, npYELLOW, npYELLOW, npYELLOW},
    {npYELLOW, npRED, npRED, npRED, npYELLOW},
    {npBLACK, npYELLOW, npYELLOW, npYELLOW, npBLACK}
};

//figura de uma estrela
npLED_t figStar [5][5] = {
    {npBLACK, npBLACK, npYELLOW, npBLACK, npBLACK},
    {npYELLOW, npYELLOW, npYELLOW, npYELLOW, npYELLOW},
    {npBLACK, npYELLOW, npYELLOW, npYELLOW, npBLACK},
    {npBLACK, npYELLOW, npBLACK, npYELLOW, npBLACK},
    {npYELLOW, npBLACK, npBLACK, npBLACK, npYELLOW},
};

//figura de um arco-íris (faltou ajustar o npSetLED pra valores intermediarios pra ter todas as
//cores mas são só 5x5 leds também então de qualquer forma não caberia tudo =/
npLED_t figRainbow [5][5] = {
    {npRED, npRED, npRED, npRED, npRED},
    {npBLUE, npBLUE, npBLUE, npBLUE, npBLUE},
    {npGREEN, npGREEN, npGREEN, npGREEN, npGREEN},
    {npYELLOW, npYELLOW, npYELLOW, npYELLOW, npYELLOW},
    {npCYAN, npCYAN, npCYAN, npCYAN, npCYAN}
};

//figura da bandeira do brasil (o que deu pra fazer né?)
npLED_t figBrazilFlag [5][5] = {
    {npGREEN,npGREEN,npGREEN,npGREEN,npGREEN},
    {npGREEN,npGREEN,npYELLOW,npGREEN,npGREEN},
    {npGREEN,npYELLOW,npBLUE,npYELLOW,npGREEN},
    {npGREEN,npGREEN,npYELLOW,npGREEN,npGREEN},
    {npGREEN,npGREEN,npGREEN,npGREEN,npGREEN}
};

//deixar aqui o # de figuras usadas para o pincel de luz. Isso facilita se for necessário incluir novas figuras 
#define figCOUNT 5

//com o define acima e o array abaixo, será possível iterar entre as figuras usando esse array com endereço das figuras
npLED_t (*figArray[figCOUNT])[5][5] = {
    &figHeart, &figSmiley, &figStar, &figRainbow, &figBrazilFlag
};

//array para guardar o nome das figuras. isso será usado no menu de seleção.
//tomar cuidado para não passar de 16 caracteres por limitação do display OLED!
const char* figNames[figCOUNT] = {
    "     Coracao    ", 
    "     Smiley     ", 
    "     Estrela    ", 
    "     Arco-Iris  ", 
    "     Brasil     "
};

//essas figuras de dos botões foram inicialmente usadas para teste. No futuro, seria interessante
//montar todo o alfabeto para poder pintar com strings dinâmicas

//figura A
npLED_t figA[5][5] = {
    {npBLACK, npWHITE, npWHITE, npWHITE, npBLACK},
    {npWHITE, npBLACK, npBLACK, npBLACK, npWHITE},
    {npWHITE, npWHITE, npWHITE, npWHITE, npWHITE},
    {npWHITE, npBLACK, npBLACK, npBLACK, npWHITE},
    {npWHITE, npBLACK, npBLACK, npBLACK, npWHITE}
};

//figura B
npLED_t figB[5][5] = {
    {npWHITE, npWHITE, npWHITE, npBLACK, npBLACK},
    {npWHITE, npBLACK, npBLACK, npWHITE, npBLACK},
    {npWHITE, npWHITE, npWHITE, npBLACK, npBLACK},
    {npWHITE, npBLACK, npBLACK, npWHITE, npBLACK},
    {npWHITE, npWHITE, npWHITE, npBLACK, npBLACK}
};

//figura J
npLED_t figJ[5][5] = {
    {npWHITE, npWHITE, npWHITE, npWHITE, npWHITE},
    {npBLACK, npBLACK, npWHITE, npBLACK, npBLACK},
    {npBLACK, npBLACK, npWHITE, npBLACK, npBLACK},
    {npWHITE, npBLACK, npWHITE, npBLACK, npBLACK},
    {npBLACK, npWHITE, npBLACK, npBLACK, npBLACK}
};     

//Figuras de cores

//Quadrado Branco
npLED_t figWhite[5][5] = {
    {npWHITE, npWHITE, npWHITE, npWHITE, npWHITE},
    {npWHITE, npWHITE, npWHITE, npWHITE, npWHITE},
    {npWHITE, npWHITE, npWHITE, npWHITE, npWHITE},
    {npWHITE, npWHITE, npWHITE, npWHITE, npWHITE},
    {npWHITE, npWHITE, npWHITE, npWHITE, npWHITE}
};

//Quadrado Vermelho
npLED_t figRed[5][5] = {
    {npRED, npRED, npRED, npRED, npRED},
    {npRED, npRED, npRED, npRED, npRED},
    {npRED, npRED, npRED, npRED, npRED},
    {npRED, npRED, npRED, npRED, npRED},
    {npRED, npRED, npRED, npRED, npRED}
};

//Quadrado Amarelo
npLED_t figYellow[5][5] = {
    {npYELLOW, npYELLOW, npYELLOW, npYELLOW, npYELLOW},
    {npYELLOW, npYELLOW, npYELLOW, npYELLOW, npYELLOW},
    {npYELLOW, npYELLOW, npYELLOW, npYELLOW, npYELLOW},
    {npYELLOW, npYELLOW, npYELLOW, npYELLOW, npYELLOW},
    {npYELLOW, npYELLOW, npYELLOW, npYELLOW, npYELLOW}
};

//Quadrado Verde
npLED_t figGreen[5][5] = {
    {npGREEN, npGREEN, npGREEN, npGREEN, npGREEN},
    {npGREEN, npGREEN, npGREEN, npGREEN, npGREEN},
    {npGREEN, npGREEN, npGREEN, npGREEN, npGREEN},
    {npGREEN, npGREEN, npGREEN, npGREEN, npGREEN},
    {npGREEN, npGREEN, npGREEN, npGREEN, npGREEN}
};

//Quadrado Ciano
npLED_t figCyan[5][5] = {
    {npCYAN, npCYAN, npCYAN, npCYAN, npCYAN},
    {npCYAN, npCYAN, npCYAN, npCYAN, npCYAN},
    {npCYAN, npCYAN, npCYAN, npCYAN, npCYAN},
    {npCYAN, npCYAN, npCYAN, npCYAN, npCYAN},
    {npCYAN, npCYAN, npCYAN, npCYAN, npCYAN}
};

//Quadrado Azul
npLED_t figBlue[5][5] = {
    {npBLUE, npBLUE, npBLUE, npBLUE, npBLUE},
    {npBLUE, npBLUE, npBLUE, npBLUE, npBLUE},
    {npBLUE, npBLUE, npBLUE, npBLUE, npBLUE},
    {npBLUE, npBLUE, npBLUE, npBLUE, npBLUE},
    {npBLUE, npBLUE, npBLUE, npBLUE, npBLUE}
};

//Quadrado Magenta
npLED_t figMagenta[5][5] = {
    {npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA},
    {npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA},
    {npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA},
    {npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA},
    {npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA}
};

//quantidade de cores disponiveis
#define colorCOUNT 7

//Array com as cores para facilitar a oscilação entre elas
npLED_t (*figColors[colorCOUNT])[5][5] = {
    &figWhite, &figRed, &figYellow, &figGreen, &figCyan, &figBlue, &figMagenta 
};
//Array com o nome das cores
const char *colorNames[colorCOUNT] = {
    "     Branco     ", 
    "     Vermelho   ", 
    "     Amarelo    ", 
    "     Verde      ", 
    "     Ciano      ", 
    "     Azul       ", 
    "     Magenta    "
};

//Essas figuras foram usadas para testar o joystick. Convém mantelas pois podem voltar a ser uteis no futuro

//Circulo (posição central do joystick)
npLED_t figCircle [5][5] = {
    {npBLACK, npMAGENTA, npMAGENTA, npMAGENTA, npBLACK},
    {npMAGENTA, npBLACK, npBLACK, npBLACK, npMAGENTA},
    {npMAGENTA, npBLACK, npCYAN, npBLACK, npMAGENTA},
    {npMAGENTA, npBLACK, npBLACK, npBLACK, npMAGENTA},
    {npBLACK, npMAGENTA, npMAGENTA, npMAGENTA, npBLACK}
};

//Seta para cima
npLED_t figArrowUp[5][5] = {
    {npBLACK, npBLACK, npBLUE, npBLACK, npBLACK},
    {npBLACK, npBLUE, npBLUE, npBLUE, npBLACK},
    {npBLUE, npBLUE, npBLUE, npBLUE, npBLUE},
    {npBLACK, npBLACK, npBLUE, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLUE, npBLACK, npBLACK}
};

//Seta para baixo
npLED_t figArrowDown[5][5] = {
    {npBLACK, npBLACK, npYELLOW, npBLACK, npBLACK},
    {npBLACK, npBLACK, npYELLOW, npBLACK, npBLACK},
    {npYELLOW, npYELLOW, npYELLOW, npYELLOW, npYELLOW},
    {npBLACK, npYELLOW, npYELLOW, npYELLOW, npBLACK},
    {npBLACK, npBLACK, npYELLOW, npBLACK, npBLACK}
};

//Seta para esquerda
npLED_t figArrowLeft[5][5] = {
    {npBLACK, npBLACK, npGREEN, npBLACK, npBLACK},
    {npBLACK,npGREEN, npGREEN, npBLACK, npBLACK},
    {npGREEN, npGREEN, npGREEN, npGREEN, npGREEN},
    {npBLACK, npGREEN, npGREEN, npBLACK, npBLACK},
    {npBLACK, npBLACK, npGREEN, npBLACK, npBLACK}
};

npLED_t figArrowRight [5][5] = {
    {npBLACK, npBLACK, npRED, npBLACK, npBLACK},
    {npBLACK, npBLACK, npRED, npRED, npBLACK},
    {npRED, npRED, npRED, npRED, npRED},
    {npBLACK, npBLACK, npRED, npRED, npBLACK},
    {npBLACK, npBLACK, npRED, npBLACK, npBLACK}
};

//Array com as setas para facilitar a oscilação entre elas
npLED_t (*figArrows[5])[5][5] = {
    &figCircle, &figArrowUp, &figArrowDown, &figArrowLeft, &figArrowRight
};

//Numeros de 0 a 3 para auxiliar com o contador prévio a execução do efeito.
//Inicialmente o contador mostrava o zero mas mesmo com brilho minimo ele ainda atrapalhava
//na captura da foto. Mas melhor deixar, no futuro seria interessante incluir os numeros de 4 a 9.

npLED_t figZERO[5][5] = {
    {npBLACK, npWHITE, npWHITE, npWHITE, npBLACK},
    {npWHITE, npBLACK, npBLACK, npBLACK, npWHITE},
    {npWHITE, npBLACK, npBLACK, npBLACK, npWHITE},
    {npWHITE, npBLACK, npBLACK, npBLACK, npWHITE},
    {npBLACK, npWHITE, npWHITE, npWHITE, npBLACK}
};

npLED_t figONE[5][5] = {
    {npBLACK, npBLACK, npWHITE, npBLACK, npBLACK},
    {npBLACK, npWHITE, npWHITE, npBLACK, npBLACK},
    {npBLACK, npBLACK, npWHITE, npBLACK, npBLACK},
    {npBLACK, npBLACK, npWHITE, npBLACK, npBLACK},
    {npBLACK, npWHITE, npWHITE, npWHITE, npBLACK}
};

npLED_t figTWO[5][5] = {
    {npBLACK, npWHITE, npWHITE, npWHITE, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npWHITE},
    {npBLACK, npBLACK, npWHITE, npWHITE, npBLACK},
    {npBLACK, npWHITE, npBLACK, npBLACK, npBLACK},
    {npBLACK, npWHITE, npWHITE, npWHITE, npWHITE}
};

npLED_t figTHREE[5][5] = {
    {npBLACK, npBLACK, npWHITE, npWHITE, npBLACK},
    {npBLACK, npWHITE, npBLACK, npBLACK, npWHITE},
    {npBLACK, npBLACK, npBLACK, npWHITE, npBLACK},
    {npBLACK, npWHITE, npBLACK, npBLACK, npWHITE},
    {npBLACK, npBLACK, npWHITE, npWHITE, npBLACK}
};

//Array com os numeros
npLED_t (*figNumbers[4])[5][5] = {
    &figZERO, &figONE, &figTWO, &figTHREE
};


//Arrays para o rastro de luz. A ideia do trail não ficou muito legal não mas é mais conveniente manter
npLED_t figTrailWhite[5][5] = {
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npWHITE, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK}
};

npLED_t figTrailRed[5][5] = {
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npRED, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK}
};

npLED_t figTrailYellow[5][5] = {
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npYELLOW, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK}
};

npLED_t figTrailGreen[5][5] = {
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npGREEN, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK}
};

npLED_t figTrailCyan[5][5] = {
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npCYAN, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK}
};

npLED_t figTrailBlue[5][5] = {
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLUE, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK}
};

npLED_t figTrailMagenta[5][5] = {
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npMAGENTA, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLACK, npBLACK, npBLACK, npBLACK, npBLACK}
};

//quantidade de cores disponiveis - por hora 8
#define trailCOUNT 7
//Array com as cores para facilitar a oscilação entre elas
npLED_t (*trailColors[trailCOUNT])[5][5] = {
    &figTrailWhite, &figTrailRed, &figTrailYellow, &figTrailGreen, &figTrailCyan, &figTrailBlue, &figTrailMagenta 
};

//Figuras para as letras do meu nome

npLED_t figP[5][5] = {
    {npBLUE, npBLUE, npBLUE, npBLUE, npBLACK},
    {npBLUE, npBLACK, npBLACK, npBLACK, npBLUE},
    {npBLUE, npBLUE, npBLUE, npBLUE, npBLACK},
    {npBLUE, npBLACK, npBLACK, npBLACK, npBLACK},
    {npBLUE, npBLACK, npBLACK, npBLACK, npBLACK}
};

npLED_t figE[5][5] = {
    {npGREEN, npGREEN, npGREEN, npGREEN, npGREEN},
    {npGREEN, npBLACK, npBLACK, npBLACK, npBLACK},
    {npGREEN, npGREEN, npGREEN, npBLACK, npBLACK},
    {npGREEN, npBLACK, npBLACK, npBLACK, npBLACK},
    {npGREEN, npGREEN, npGREEN, npGREEN, npGREEN}
};

npLED_t figD[5][5] = {
    {npRED, npRED, npRED, npRED, npBLACK},
    {npRED, npBLACK, npBLACK, npBLACK, npRED},
    {npRED, npBLACK, npBLACK, npBLACK, npRED},
    {npRED, npBLACK, npBLACK, npBLACK, npRED},
    {npRED, npRED, npRED, npRED, npBLACK}
};

npLED_t figR[5][5] = {
    {npCYAN, npCYAN, npCYAN, npCYAN, npBLACK},
    {npCYAN, npBLACK, npBLACK, npBLACK, npCYAN},
    {npCYAN, npCYAN, npCYAN, npCYAN, npBLACK},
    {npCYAN, npBLACK, npBLACK, npBLACK, npCYAN},
    {npCYAN, npBLACK, npBLACK, npBLACK, npCYAN}
};

npLED_t figO[5][5] = {
    {npBLACK, npMAGENTA, npMAGENTA, npMAGENTA, npBLACK},
    {npMAGENTA, npBLACK, npBLACK, npBLACK, npMAGENTA},
    {npMAGENTA, npBLACK, npBLACK, npBLACK, npMAGENTA},
    {npMAGENTA, npBLACK, npBLACK, npBLACK, npMAGENTA},
    {npBLACK, npMAGENTA, npMAGENTA, npMAGENTA, npBLACK}
};

#define letterCOUNT 5
//Array com as letras do meu nome =)
npLED_t (*arrayPedro[letterCOUNT])[5][5] = {
    &figP, &figE, &figD, &figR, &figO 
};

#pragma endregion //Fim definição figuras

#pragma endregion //######################Fim Código Neopixel#####################################

#pragma region //##### Inicio - Código para os botões 

//Feito por mim, com base no material do Embarcatech 
//usado originalmente no projeto Wokwi: https://wokwi.com/projects/421772261889153025
//leitura dos botões via interrupt com debouncing via temporizador de hardware

//Pinos dos botões
#define A_BUTTON_PIN 5    //Botao A
#define B_BUTTON_PIN 6    //Botao B
#define J_BUTTON_PIN 22   //Botao do Joystick

//tempo constante para o sistema fazer o debouncing
#define DEBOUNCE_TIME_MS 50

//Struct para definir as variáveis de uso de um botão
typedef struct{ 
uint8_t gpioPin;             //Número do pino do botão
volatile int pressCount;     //Contador do # de vezes que o botão foi pressionado
volatile bool isPressed;     //flag para lidar com o acionamento do botão
absolute_time_t lastPressed; //timestamp do ultimo acionamento do botão
} buttonState;

//configurando os botões como variáveis globais
buttonState buttonA, buttonB, buttonJ; 

//função para imprimir o status de um botão (util para debugar!)
void button_status(uint8_t pin){
    buttonState *button;
    if (pin == A_BUTTON_PIN){
        if (debugMode){printf("Status do botão A\n");}
        button = &buttonA;
    } else if (pin == B_BUTTON_PIN){
        if (debugMode){printf("Status do botão B\n");}
        button = &buttonB;
    } else if (pin == J_BUTTON_PIN){
        if (debugMode){printf("Status do botão do Joystick\n");}
        button = &buttonJ;
    } else {
        if (debugMode){printf("Botao invalido!\n");}
        return;
    }
    if (debugMode){printf("Pino do botao: %d\n", button->gpioPin);}
    if (debugMode){printf("# de acionamentos: %d\n", button->pressCount);}
    if (debugMode){printf("Está acionado? %s\n", button->isPressed ? "sim" : "nao");}
    if (debugMode){printf("Ultimo acionamento: %d\n", button->lastPressed);}
}


//função de callback para as interrupções do GPIO
void gpio_callback (uint8_t gpio, uint32_t events){
    if (debugMode){printf("Interrupcao detectada\n");}
    buttonState* button;
    if (gpio == A_BUTTON_PIN){ 
        button = &buttonA;  //passagem por referencia do botão
    } else if (gpio == B_BUTTON_PIN){
        button = &buttonB;
    } else if (gpio == J_BUTTON_PIN){
        button = &buttonJ;
    } else {
        if (debugMode){printf("Erro! Botao invalido detectado!\n");}
        return;
    }
    
    //checagem do tempo para fazer o debounce
    absolute_time_t now = get_absolute_time(); 
    
    //diferenca de tempo, em microssegundos, entre o ultimo acionamento do botão e o tempo atual
    int64_t elapsedTime  = absolute_time_diff_us(button->lastPressed, now); 
    
    //multiplica-se a constante DEBOUNCE_TIME_MS por mil para converter para microssegundos
    if (elapsedTime >= DEBOUNCE_TIME_MS *1000){ 
        if (events & GPIO_IRQ_EDGE_FALL){         //borda de caída visto que o botão está em pull_up (baixa quando pressionado, alta quando não pressionado)
            button_status(gpio);
            button->pressCount += 1;
            button->isPressed = true;
            button->lastPressed = get_absolute_time();
        }
        if (events & GPIO_IRQ_EDGE_RISE){
            if (button->isPressed){
            button->isPressed = false;  
            }
        }
    }
}

//função para configurar um botão e inicializar seus valores
void init_button(buttonState *button, uint8_t pin){
  button->gpioPin = pin;
  button->pressCount = 0;
  button->isPressed = false;
  button->lastPressed = get_absolute_time();
  gpio_init(button->gpioPin);
  gpio_set_dir(button->gpioPin, GPIO_IN);
  gpio_pull_up(button->gpioPin); //botao em pullup: Le alto quando não acionado, baixo quando acionado
  gpio_set_irq_enabled_with_callback(button->gpioPin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback); //gera interrupcao e chama a função gpio_callback
}

#pragma endregion //FIM - Código dos botões

#pragma region //##### Inicio - Código para o joystick

//Adaptado de: https://github.com/BitDogLab/BitDogLab-C/tree/main/joystick
//Combinando também com meu código para os botões

/**
 * Embarcatech adaptado de: 
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * SPDX-License-Identifier: BSD-3-Clause
 */

//pinos usados para o joystick
#define JOYSTICK_X_PIN 26 //pino 27 está atribuido ao canal ADC 0
#define JOYSTICK_Y_PIN 27 //pino 26 está atribuido ao canal ADC 1

//para facilitar a leitura, ao inves de usar todo o range de valores do ADC,
//será definido um range de -100 a +100 para os valores do joystick
#define JOYSTICK_MIN -100
#define JOYSTICK_MAX  100
#define DEADZONE 20
#define THRESHOLD 75
#define DEBOUNCE_TIME_MS_JOYSTICK 200

//é mais conveniente usar nomes ao invés de números para as direções
#define joyCENTER 0
#define joyUP 1
#define joyDOWN 2
#define joyLEFT 3
#define joyRIGHT 4

//e é também mais conveniente usar um signed int para guardar os valores de -range a +range do que de zero a range
typedef struct{
    int x;
    int y;
    int direction;
    bool directionChanged; 
    absolute_time_t lastUpdate; 
} joystickState;

//configurando o joystick como variável global
joystickState joystick; 

//função de inicialização do joystick
void initJoystick(){
    //inicializa o conversor analógico-digital
    adc_init();
    // Configura os pinos GPIO 26 e 27 como entradas de ADC (alta impedância, sem resistores pull-up)
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);
    joystick.lastUpdate = get_absolute_time();
}

//retorna com base no valor lido um numero entre JOYSTICK_MIN e JOYSTICK_MAX
int mapJoystickValue(uint16_t adcValue){
    return (adcValue * (JOYSTICK_MAX - JOYSTICK_MIN) / 4095) + JOYSTICK_MIN;
}

//função para ler o joystick
int readJoystick(){
    //leitura dos valores do joystick
    adc_select_input(1);
    uint adcX = adc_read();
    joystick.x = mapJoystickValue(adcX);
    sleep_us(2);
    adc_select_input(0);
    uint adcY = adc_read();
    joystick.y = mapJoystickValue(adcY);
    sleep_us(2);
    //aplicação da deadzone para evitar leituras falsas
    //se  -deadzone < valor lido < deadzone, anular a leitura
    if (joystick.x < DEADZONE && joystick.x > -DEADZONE){
        joystick.x = 0;
    }
    if (joystick.y < DEADZONE && joystick.y > -DEADZONE){
        joystick.y = 0;
    }
    if (debugMode){printf("Cord X: %+d Cord Y: %+d\n", joystick.x, joystick.y);}
    absolute_time_t now = get_absolute_time();
    //pega a leitura atual do joystick 
    int64_t elapsedTime  = absolute_time_diff_us(joystick.lastUpdate, now);
    //se o tempo entre a leitura atual e a última leitura for maior que o tempo de debounce, atualiza o estado do joystick 
    memset(ssd, 0, ssd1306_buffer_length);
    if (elapsedTime >= DEBOUNCE_TIME_MS_JOYSTICK *1000){
        //a função so irá retornar uma direção uma única vez.
        //após isso a direção só será atualizada quando o joystick voltar ao centro
        if (joystick.directionChanged == false){
            if (joystick.x > THRESHOLD){
                joystick.directionChanged = true;
                joystick.lastUpdate = now;
                return joyRIGHT;
            } else if (joystick.x < -THRESHOLD){
                joystick.directionChanged = true;
                joystick.lastUpdate = now;
                return joyLEFT;
            } else if (joystick.y > THRESHOLD){
                joystick.directionChanged = true;
                joystick.lastUpdate = now;
                return joyUP;                
            } else if (joystick.y < -THRESHOLD){
                joystick.directionChanged = true;
                joystick.lastUpdate = now;
                return joyDOWN;
            } else {
                joystick.directionChanged = false;
                return joyCENTER;
            }
        }
    }
    joystick.directionChanged = false;
    return joyCENTER;
} 


#pragma endregion //FIM - Código para o joystick

#pragma region //##### Inicio - Código para o buzzer
//Adaptado de: https://github.com/BitDogLab/BitDogLab-C/tree/main/buzzer_pwm1

#define BUZZER_PIN 21

//Inicializacao do PWM no pino do buzzer
void pwm_init_buzzer() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f); // Ajusta divisor de clock
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_PIN, 0); // Desliga o PWM inicialmente
}

// Toca uma nota com a frequência e duração especificadas
void play_tone(uint frequency, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint32_t clock_freq = clock_get_hz(clk_sys); 
    uint32_t top = clock_freq / frequency - 1;
    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(BUZZER_PIN, top / 2); // 50% de duty cycle
    sleep_ms(duration_ms);
    pwm_set_gpio_level(BUZZER_PIN, 0); // Desliga o som após a duração
}

//Criei uma musiquinha aleatória para tocar quando o programa inicia
//Queria fazer algo mais legal mas não deu tempo =/
void playStartupTune(){

    for (int i = 0; i < 3; ++i){
    play_tone(2000 + i*500, 100);
    play_tone(1500 + i*500, 100);
    play_tone(1000 + i*500, 100);
    play_tone(3000 + i*500, 100);
    play_tone(2000 + i*500, 100);
    play_tone(1000 + i*500, 100);
    play_tone(5000 + i*500, 100);
    play_tone(2500 + i*500, 100);
    play_tone(1250 + i*500, 100);
    play_tone(2000 + i*500, 100);
    }
}


//Beep para tocar quando detectar o pressionamento de alguma tecla QUE resulte em alguma alteração de parâmetro
//Assim servindo de feedback sonoro para o usuário
void beep() {
    play_tone(2000, 50);
}

//Beep para tocar na contagem prévia ao inicio do efeito - ficou melhor do que eu esperava
void countDownBeep(){
    for (int i = 11; i > 1; --i){
        play_tone(i*300, 50);
    } 
}

//beep para tocar ao resetar para bootsel
void shutdownBeep(){
    for (int i = 0; i < 3; ++i){
    play_tone(3500,250);
    sleep_ms(150);
    }
}

#pragma endregion

#pragma region //##### Inicio - Código para os efeitos

//quantidade de efeitos 
#define effectCOUNT 5
#define effBLINK    0
#define effTRAIL    1
#define effFIXED    2
#define effRAINBOW  3
#define effPEDRO    4

//lista dos efeitos
const char* effectNames[effectCOUNT] = {
    "     Piscar     ",
    "     Rastro     ",
    "    Cor Fixa    ",
    "  Cor oscilante ",
    "      Pedro     "
};

//tempo que a matriz led ficara ativa na função piscar
#define BLINK_TIME 150

//tempo que a matriz led ficara inativa na função piscar
#define BLINK_INTERVAL 850

//intervalo para mudança de cor na função cor oscilante
#define OSCILLATE_TIME 150

#pragma endregion //FIM - Código para os efeitos

int main() {
    
    #pragma region //Funções de Inicialização
    //#Init do pico e Wi-fi
    stdio_init_all();
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }

    //Inicialização dos botões
    init_button(&buttonA, A_BUTTON_PIN);
    init_button(&buttonB, B_BUTTON_PIN);
    init_button(&buttonJ, J_BUTTON_PIN);
    
    //Inicialização do joystick
    initJoystick();
    
    //Inicialização da matriz 5x5 de LEDs
    npInit(LED_PIN);
    npClear();

    //Inicialização do display OLED
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Processo de inicialização completo do OLED SSD1306
    ssd1306_init(); 
    // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);   

    //Inicialização do buzzer
    pwm_init_buzzer();
    
    #pragma endregion //Fim das funções de inicialização
    
    //Tela de apresentação
    ssd1306_draw_string(ssd, 0,0,  "LightBrush v1.0 ");
    ssd1306_draw_string(ssd, 0,8,  "by pedrohdavelar");
    ssd1306_draw_string(ssd, 0,32, "Projeto Final   ");
    ssd1306_draw_string(ssd, 0,40, "     Embarcatech");
    render_on_display(ssd, &frame_area); 
    playStartupTune();
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area); 


    //variaveis para seleção: figura|cor, tempo e efeito
    int figSelection = 0;
    int colorSelection = 0;
    int secSelection = 3;
    char charSecSelection[2];
    int effectSelection = 0;
    int pedroCount = 0;

    absolute_time_t delayCheck = get_absolute_time();

    while (true) {
        //leitura do joystick para alteração de parametros
        joystick.direction = readJoystick();
        if (debugMode){printf("Joystick direction: %d", joystick.direction);}

        //alteração da figura|cor ao pressionar o joystick para a esquerda ou direita
        if (effectSelection == effBLINK || effectSelection == effTRAIL){
            if (joystick.direction == joyLEFT) {--figSelection;beep();}
            if (joystick.direction == joyRIGHT) {++figSelection;beep();}
        }
        if (effectSelection == effFIXED){
            if (joystick.direction == joyLEFT) {--colorSelection;beep();}
            if (joystick.direction == joyRIGHT) {++colorSelection;beep();}
        }

        //se figSelection for maior que o número de figuras, volta para a primeira;
        //se for menor que zero, volta para a última
        if (figSelection > (figCOUNT - 1)) {figSelection = 0;}
        if (figSelection < 0){figSelection = (figCOUNT - 1);}
        
        //delay de 300ms para atualizar as cores no caso da função cor oscilante
        if (absolute_time_diff_us(delayCheck, get_absolute_time()) > 300000 && effectSelection == effRAINBOW ){
            delayCheck = get_absolute_time();
            ++colorSelection;
        }
        
        //se colorSelection for maior que o número de cores, volta para a primeira;
        //se for menor que zero, volta para a última
        if (colorSelection > (colorCOUNT - 1)) {colorSelection = 0;}
        if (colorSelection < 0){colorSelection = (colorCOUNT - 1);}
        
        
        //alteração do tempo ao pressionar o joystick para cima ou para baixo
        //no caso do tempo não vou circular igual as figuras. Vai ser no mínimo 3 seg e no máximo 10 seg
        if (joystick.direction == joyUP && secSelection < 10) {++secSelection;beep();}
        if (joystick.direction == joyDOWN && secSelection > 3) {--secSelection;beep();}

        //alteração do efeito ao pressionar b
        if (buttonB.isPressed){
            beep();
            ++effectSelection;
            if (effectSelection > (effectCOUNT - 1)) {effectSelection = 0;}
            buttonB.isPressed = false;
        }
        
        //imprime no display oled as configurações selecionadas
        if (effectSelection == effBLINK || effectSelection == effTRAIL){
            ssd1306_draw_string(ssd, 0,7, "<Selecao Figura>");
            ssd1306_draw_string(ssd, 0,15, figNames[figSelection]);
        }
        if (effectSelection == effFIXED || effectSelection == effRAINBOW){
            ssd1306_draw_string(ssd, 0,7, "<Selecao    Cor>");
            if (effectSelection == effFIXED){
            ssd1306_draw_string(ssd, 0,15, colorNames[colorSelection]);
            }
        }
        
        if (effectSelection == effPEDRO){
            ssd1306_draw_string(ssd, 0,7,  "  Imprimir Nome ");
        }
        
        if (effectSelection != effPEDRO){
        ssd1306_draw_string(ssd, 0,23, "^Selecao  Tempo#");
        }
        
        ssd1306_draw_string(ssd, 0,56,"A RUN   B Efeito");

        //imprime no display oled o tempo selecionado
        if (effectSelection != effPEDRO){ 
            sprintf(charSecSelection, "%d", secSelection);
            ssd1306_draw_string(ssd, 24, 31, "Tempo ");
            ssd1306_draw_string(ssd, 72, 31, charSecSelection);
            ssd1306_draw_string(ssd, 90, 31, "sec");
        };

        //imprime no display oled o efeito selecionado
        ssd1306_draw_string(ssd, 0, 39, effectNames[effectSelection]);

        //atualiza o display oled
        render_on_display(ssd, &frame_area);
        
        //imprime na matriz led a figura|cor selecionada
        if (effectSelection == effBLINK || effectSelection == effTRAIL){
            npSetFigure(figArray[figSelection]);
        }
        
        if (effectSelection == effFIXED || effectSelection == effRAINBOW){
            npSetFigure(figColors[colorSelection]);
        }        
        
        if (effectSelection == effPEDRO){
            npSetFigure(arrayPedro[pedroCount]);
            //delay de 300ms para atualizar as letras na matriz de led
            if (absolute_time_diff_us(delayCheck, get_absolute_time()) > 300000){
                delayCheck = get_absolute_time();
                ++pedroCount;
            }
            if (pedroCount > (letterCOUNT - 1)) {pedroCount = 0;}
        }

        //no momento da configuração, deixar o brilho baixo para não cegar o usuário!
        npSetBrightness(npMIN);
        npWrite();
        sleep_us(2);
        
        //se o botão A for pressionado, inicia o efeito
        if (buttonA.isPressed){
            beep();
            //timer de 4 segundos antes de iniciar o efeito
            for (int i = 3; i > 0; --i){
                memset(ssd, 0, ssd1306_buffer_length);
                sprintf(charSecSelection, "%d", i);
                ssd1306_draw_string(ssd, 72, 31, charSecSelection);
                render_on_display(ssd, &frame_area); 
                npSetFigure(figNumbers[i]);
                npSetBrightness(1);
                npWrite();
                countDownBeep();
                sleep_ms(500);
            }
            memset(ssd, 0, ssd1306_buffer_length);
            ssd1306_draw_string(ssd, 72, 31, "0");
            render_on_display(ssd, &frame_area); 
            npClear();
            npWrite();
            sleep_ms(1000);

            //buffer de meio segundo antes de começar; tambem limpar o display oled
            memset(ssd, 0, ssd1306_buffer_length);
            render_on_display(ssd, &frame_area); 
            sleep_ms(500);   
            //tempo de inicio dos efeitos
            absolute_time_t startTime = get_absolute_time();
            absolute_time_t secCountA, secCountB;
            secCountA = get_absolute_time();
            uint8_t secCount = 0;

            //efeito de piscar
            if (effectSelection == effBLINK){
                //como absolute_time_diff_us lida com microssegundos, multiplicamos secSelection por 10^6
                //enquanto o loop não atingir o tempo selecionado, ele irá processar o efeito selecionado
                while (absolute_time_diff_us(startTime, get_absolute_time()) < secSelection * 1000000){
                    
                    //a matriz irá alternar entre a figura selecionado e desligada usando os intervalos definidos
                    //por BLINK_TIME e BLINK_INTERVAL
                    npSetFigure(figArray[figSelection]);
                    npWrite();
                    sleep_ms(BLINK_TIME);
                    npClear();
                    npWrite();
                    sleep_ms(BLINK_INTERVAL);
                }
            }

            //efeito de rastro
            if (effectSelection == effTRAIL){
                //nesse caso, primeiro será feito um rastro durante o tempo selecionado - BLINK_TIME,
                //depois a figura será apresentada durante blink time. A ideia é fazer um rastro finalizando com a 
                //figura como se fosse uma estrela cadente
                int trailSelection = 0;
                while (absolute_time_diff_us(startTime, get_absolute_time()) < (secSelection * 1000000 - (BLINK_TIME * 1000))){
                    npSetFigure(trailColors[trailSelection]);
                    npWrite();
                    sleep_ms(OSCILLATE_TIME);
                    ++trailSelection;
                    if (trailSelection > (trailCOUNT - 1)) {trailSelection = 0;}
                }
                    npSetFigure(figArray[figSelection]);
                    npWrite();
                    sleep_ms(BLINK_TIME);
            }

            //efeito de cor fixa
            if (effectSelection == effFIXED){
                npSetFigure(figColors[colorSelection]);
                npWrite();
                sleep_ms(secSelection * 1000);
            }

            //efeito de arco-íris
            if (effectSelection == effRAINBOW){
                while (absolute_time_diff_us(startTime, get_absolute_time()) < secSelection * 1000000){
                    npSetFigure(figColors[colorSelection]);
                    npWrite();
                    sleep_ms(OSCILLATE_TIME);
                    ++colorSelection;
                    if (colorSelection > (colorCOUNT - 1)) {colorSelection = 0;}
                }
            }
                    
            //efeito do meu nome =)
            if (effectSelection == effPEDRO){
                for (int i = 0; i < letterCOUNT; ++i){
                    npSetFigure(arrayPedro[i]);
                    npWrite();
                    sleep_ms(BLINK_TIME);
                    npClear();
                    npWrite();
                    sleep_ms(BLINK_INTERVAL);
                }
            }
            
            //intervalo de 3 segundos com tudo apagado para dar tempo de finalizar a foto
            npClear();
            npWrite();
            memset(ssd, 0, ssd1306_buffer_length);
            render_on_display(ssd, &frame_area);
            sleep_ms(3000);

            //garantir a limpeza do estado do botão A para evitar reacionamento em falso
            buttonA.isPressed = false;
        }

        //reset para bootsel se j e b forem pressionados. Útil para debugar e não vejo porque remover.
        //Futuramente se o botão do joystick for utilizado.... 
        if (buttonJ.isPressed && buttonB.isPressed){
            memset(ssd, 0, ssd1306_buffer_length);
            render_on_display(ssd, &frame_area);
            npClear();
            npWrite();
            sleep_ms(100);
            shutdownBeep();
            reset_usb_boot(0, 0);
        }
    }
}
