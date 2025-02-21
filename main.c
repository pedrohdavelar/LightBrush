/**
Projeto Final Embarcatech
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/binary_info.h"
#include "pico/bootrom.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h" 
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "hardware/adc.h"
#include "ws2818b.pio.h"

#pragma region  //##### Inicio - Código Display OLED 1306
//Adaptado de: https://github.com/BitDogLab/BitDogLab-C/tree/main/display_oled
#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 15

//variaveis para guardar a posição atual do cursor
int currentLine = 0;       
int currentColumn = 0;

uint8_t ssd[ssd1306_buffer_length];

struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};

//Função para verificar e resetar, se for o caso, a posição do cursor
void checkCursorPosition(){
    if (currentLine >= 64){
        currentLine = 0;
        memset(ssd, 0, ssd1306_buffer_length);
    }
    if (currentColumn >= 128){
        currentColumn = 0;
        memset(ssd, 0, ssd1306_buffer_length);
    }
}
//
#pragma endregion //###########################Fim - Código Display OLED 1306####################

#pragma region //##### Inicio - Código Neopixel
//######################Matriz 5x5 de LEDs RGB#################################
//Código para uso da matriz de LEDs 5x5
//Adaptado de: https://github.com/BitDogLab/BitDogLab-C/tree/main/neopixel_pio

//Defines para a matriz 5x5 de LEDs
#define LED_COUNT 25
#define LED_PIN 7

//Definição das cores para os leds
#define npBLACK   { 0,   0,   0 }
#define npRED     { 255, 0,   0 }
#define npGREEN   { 0,   255, 0 }
#define npBLUE    { 0,   0,   255 }
#define npYELLOW  { 255, 255, 0 }
#define npCYAN    { 0,   255, 255 }
#define npMAGENTA { 255, 0,   255 }
#define npWHITE   { 255, 255, 255 }



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
//https://discord.com/channels/1306286396063088640/1307105239236739134/1339395917286998121


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

//Escreve os dados do buffer nos LEDs
void npWrite(){
    //Escreve cada dado de 8 bits dos pixels em sequencia no buffer da máquina PIO
    for (uint i = 0; i < LED_COUNT; ++i){
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
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

#pragma region //Figuras para a matriz de LEDs

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

//deixar aqui o # de figuras usadas para o pincel de luz -1. Por hora, 5-1 =4:
//figHeart, figSmiley, figStar, figRainbow, figBrazilFlag
#define figCOUNT 5

//com o define acima e o array abaixo, será possível iterar facilmente entre as figuras
npLED_t (*figArray[])[5][5] = {
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

//Quadrado Verde
npLED_t figGreen[5][5] = {
    {npGREEN, npGREEN, npGREEN, npGREEN, npGREEN},
    {npGREEN, npGREEN, npGREEN, npGREEN, npGREEN},
    {npGREEN, npGREEN, npGREEN, npGREEN, npGREEN},
    {npGREEN, npGREEN, npGREEN, npGREEN, npGREEN},
    {npGREEN, npGREEN, npGREEN, npGREEN, npGREEN}
};

//Quadrado Azul
npLED_t figBlue[5][5] = {
    {npBLUE, npBLUE, npBLUE, npBLUE, npBLUE},
    {npBLUE, npBLUE, npBLUE, npBLUE, npBLUE},
    {npBLUE, npBLUE, npBLUE, npBLUE, npBLUE},
    {npBLUE, npBLUE, npBLUE, npBLUE, npBLUE},
    {npBLUE, npBLUE, npBLUE, npBLUE, npBLUE}
};

//Quadrado Amarelo
npLED_t figYellow[5][5] = {
    {npYELLOW, npYELLOW, npYELLOW, npYELLOW, npYELLOW},
    {npYELLOW, npYELLOW, npYELLOW, npYELLOW, npYELLOW},
    {npYELLOW, npYELLOW, npYELLOW, npYELLOW, npYELLOW},
    {npYELLOW, npYELLOW, npYELLOW, npYELLOW, npYELLOW},
    {npYELLOW, npYELLOW, npYELLOW, npYELLOW, npYELLOW}
};

//Quadrado Ciano
npLED_t figCyan[5][5] = {
    {npCYAN, npCYAN, npCYAN, npCYAN, npCYAN},
    {npCYAN, npCYAN, npCYAN, npCYAN, npCYAN},
    {npCYAN, npCYAN, npCYAN, npCYAN, npCYAN},
    {npCYAN, npCYAN, npCYAN, npCYAN, npCYAN},
    {npCYAN, npCYAN, npCYAN, npCYAN, npCYAN}
};

//Quadrado Magenta
npLED_t figMagenta[5][5] = {
    {npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA},
    {npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA},
    {npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA},
    {npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA},
    {npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA, npMAGENTA}
};


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

//Array com as cores para facilitar a oscilação entre elas
npLED_t (*figColors[])[5][5] = {
    &figRed, &figGreen, &figBlue, &figYellow, &figCyan, &figMagenta, &figWhite
};

//Array com as setas para facilitar a oscilação entre elas
npLED_t (*figArrows[])[5][5] = {
    &figCircle, &figArrowUp, &figArrowDown, &figArrowLeft, &figArrowRight
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

//tempo constante para o sistema fazer o debouncing - calibrar depois
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
        printf("Status do botão A\n");
        checkCursorPosition();
        //ssd1306_draw_string(ssd, currentColumn, currentLine, "A! ");
        //render_on_display(ssd, &frame_area);
        currentLine += 8;
        currentColumn += 24;
        button = &buttonA;
    } else if (pin == B_BUTTON_PIN){
        printf("Status do botão B\n");
        checkCursorPosition();
        //ssd1306_draw_string(ssd, currentColumn, currentLine, "B! ");
        //render_on_display(ssd, &frame_area);
        currentLine += 8;
        currentColumn += 24;
        button = &buttonB;
    } else if (pin == J_BUTTON_PIN){
        printf("Status do botão do Joystick\n");
        checkCursorPosition();
        //ssd1306_draw_string(ssd, currentColumn, currentLine, "J! ");
        //render_on_display(ssd, &frame_area);
        currentLine += 8;
        currentColumn += 24;
        button = &buttonJ;
    } else {
        printf("Botao invalido!\n");
        return;
    }
    printf("Pino do botao: %d\n", button->gpioPin);
    printf("# de acionamentos: %d\n", button->pressCount);
    printf("Está acionado? %s\n", button->isPressed ? "sim" : "nao");
    printf("Ultimo acionamento: %d\n", button->lastPressed);
}


//função de callback para as interrupções do GPIO
void gpio_callback (uint8_t gpio, uint32_t events){
    printf("Interrupcao detectada\n");
    buttonState* button;
    if (gpio == A_BUTTON_PIN){ 
        button = &buttonA;  //passagem por referencia do botão
    } else if (gpio == B_BUTTON_PIN){
        button = &buttonB;
    } else if (gpio == J_BUTTON_PIN){
        button = &buttonJ;
    } else {
        printf("Erro! Botao invalido detectado!\n");
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

//como os valores de leitura do joystick variam de um valor negativo para positivo,
//é interessante usar um signed int para guardar os valores
#define joyCENTER 0
#define joyUP 1
#define joyDOWN 2
#define joyLEFT 3
#define joyRIGHT 4


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
    printf("Cord X: %+d Cord Y: %+d\n", joystick.x, joystick.y);
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

    #pragma endregion //Fim das funções de inicialização
    char charFigSelection[1];
    int figSelection = 0;
    
    while (true) {
        //imprime no display os comandos
        ssd1306_draw_string(ssd, 0,7, "<Selecao Figura>");
        ssd1306_draw_string(ssd, 0,15, "^Selecao  Tempo#");
        ssd1306_draw_string(ssd, 0,60,"A RUN   B Efeito");

        /*switch (figSelection){
             case 0: ssd1306_draw_string(ssd, 0,31,"Coracao"); break;
             case 1: ssd1306_draw_string(ssd, 0,31,"Smiley"); break;
             case 2: ssd1306_draw_string(ssd, 0,31,"Estrela"); break;
             case 3: ssd1306_draw_string(ssd, 0,31,"Arco-Iris"); break;
             case 4: ssd1306_draw_string(ssd, 0,31,"Brasil"); break;
        }*/
        ssd1306_draw_string(ssd, 0,31, figNames[figSelection]);
        render_on_display(ssd, &frame_area);
        sleep_ms(50);

        joystick.direction = readJoystick();
        /*switch (joystick.direction){
            case joyCENTER: ssd1306_draw_string(ssd,0,39,"Center"); break;
            case joyUP    : ssd1306_draw_string(ssd,0,39,"Up"); break;
            case joyDOWN  : ssd1306_draw_string(ssd,0,39,"Down"); break;
            case joyLEFT: ssd1306_draw_string(ssd,0,39,"Left"); break;
            case joyRIGHT: ssd1306_draw_string(ssd,0,39,"Right"); break;
        }*/

        //figSelection++;
        printf("Joystick direction: %d", joystick.direction);
        if (joystick.direction == joyLEFT) {--figSelection;}
        if (joystick.direction == joyRIGHT) {++figSelection;}
        if (figSelection > (figCOUNT - 1)) {figSelection = 0;}
        if (figSelection < 0){figSelection = (figCOUNT - 1);}
        npSetFigure(figArray[figSelection]);        
        npWrite();
        sleep_us(2);
        
        if (buttonA.isPressed && buttonB.isPressed){
            memset(ssd, 0, ssd1306_buffer_length);
            ssd1306_draw_string(ssd, 0, 0,  "A + B!");
            ssd1306_draw_string(ssd, 0, 16, "          RESET!");
            render_on_display(ssd, &frame_area);
            sleep_ms(1500);
            npSetFigure(figOff);
            npWrite();
            reset_usb_boot(0, 0);
        }
    }
}
