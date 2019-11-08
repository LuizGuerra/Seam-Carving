#include <stdio.h>
#include <stdlib.h>
#include <string.h>        // Para usar strings

#ifdef WIN32
#include <windows.h>    // Apenas para Windows
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>     // Funções da OpenGL
#include <GL/glu.h>    // Funções da GLU
#include <GL/glut.h>   // Funções da FreeGLUT
#endif

// SOIL é a biblioteca para leitura das imagens
#include "SOIL.h"

// Um pixel RGB (24 bits)
typedef struct {
    unsigned char r, g, b;
} RGB;

// Uma imagem RGB
typedef struct {
    int width, height;
    RGB* img;
} Img;

// Protótipos
void load(char* name, Img* pic);
void uploadTexture();

// Funções da interface gráfica e OpenGL
void init();
void draw();
void keyboard(unsigned char key, int x, int y);

// Largura e altura da janela
int width, height;

// Identificadores de textura
GLuint tex[3];

// As 3 imagens
Img pic[3];

// Imagem selecionada (0,1,2)
int sel;

// Carrega uma imagem para a struct Img
void load(char* name, Img* pic)
{
    int chan;
    pic->img = (RGB*) SOIL_load_image(name, &pic->width, &pic->height, &chan, SOIL_LOAD_RGB);
    if(!pic->img)
    {
        printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
        exit(1);
    }
    printf("Load: %d x %d x %d\n", pic->width, pic->height, chan);
}

int calculateEnergy (RGB* rgb1, RGB* rgb2);
int calculateEnergy (RGB* rgb1, RGB* rgb2) {
    int red = rgb2->r - rgb1->r;
    red *= red;
    int green = rgb2->g - rgb1->g;
    green *= green;
    int blue = rgb2->b - rgb1->b;
    blue *= blue;
    return red + green + blue;
}

int isRightBorder (int index, int widthA, int heightA);
int isRightBorder (int index, int widthA, int heightA) {
    for (int i = widthA-1; i < widthA*heightA; i += widthA) {
        if (i == index) { return 1; }
    }
    return 0;
}

int calculatePixelEnergy (Img* picture, int currentLineIndex);
int calculatePixelEnergy (Img* picture, int currentLineIndex) {

    int previousLineIndex = currentLineIndex - picture->width;
    int nextLineIndex = currentLineIndex + picture->width;

    RGB left, right, top, bot;

    int isTop = currentLineIndex < picture->width;
    int isBot = currentLineIndex >= (picture->width * picture->height-picture->width);
    int isLeft = currentLineIndex % picture->width == 0;
    int isRight = isRightBorder(currentLineIndex, picture->width, picture->height);

    /* Pra fazer testes: */

//    printf("  %i\n", isTop);
//    printf("%i %i %i\n", isLeft,!(isLeft || isRight || isBot || isTop) , isRight);
//    printf("  %i\n\n", isBot);

    if (isTop) {
        top.r = top.g = top.b = 0;
        bot = picture->img[nextLineIndex];
        if (isLeft) {
            left.r = left.g = left.b = 0;
            right = picture->img[currentLineIndex+1];
        } else if (isRight) {
            left = picture->img[currentLineIndex-1];
            right.r = right.g = right.b = 0;
        } else {
            left = picture->img[currentLineIndex-1];
            right = picture->img[currentLineIndex+1];
        }
    } else if (isBot) {
        bot.r = bot.g = bot.b = 0;
        top = picture->img[previousLineIndex];
        if (isLeft) {
            left.r = left.g = left.b = 0;
            right = picture->img[currentLineIndex+1];
        } else if (isRight) {
            left = picture->img[currentLineIndex-1];
            right.r = right.g = right.b = 0;
        } else {
            left = picture->img[currentLineIndex-1];
            right = picture->img[currentLineIndex+1];
        }
    } else if (isLeft) {
        top = picture->img[previousLineIndex];
        bot = picture->img[nextLineIndex];
        left.r = left.g = left.b = 0;
        right = picture->img[currentLineIndex+1];
    } else if (isRight) {
        top = picture->img[previousLineIndex];
        bot = picture->img[nextLineIndex];
        left = picture->img[currentLineIndex-1];
        right.r = right.g = right.b = 0;
    } else { /* Is not in a border */
        top = picture->img[previousLineIndex];
        bot = picture->img[nextLineIndex];
        left = picture->img[currentLineIndex-1];
        right = picture->img[currentLineIndex+1];
    }

    int horizontalValue = calculateEnergy(&right, &left);
    int verticalValue = calculateEnergy(&bot, &top);

    return horizontalValue + verticalValue;
}

int * energyMap (Img* img);
int * energyMap (Img* img) {
    int size = img->height*img->width;
    int * array = malloc(size);
    for (int index = 0; index < size; index++) {
        if (pic[1].img[index].r > 100) {
            array[index] = -10000000;
            continue;
        }
        array[index] = calculatePixelEnergy(&img[0], index);
        if (pic[1].img[index].g > 100) {
            array[index] += 10000000; // 10 000 000
        }
    }
    printf("retornando array");
    return array;
}

int * acumulatedEnergyMap (Img* picture, const int* energyMapArray);
int * acumulatedEnergyMap (Img* picture, const int* energyMapArray) {

    int size = picture->height * picture->width;

    int * acumuledEnergyMapArray = malloc(size);

    for (int index = 0; index < size; index++) {
        int leftFather, rightFather;
        int isTop = index < picture->width;
        int isBot = index >= (picture->width * picture->height-picture->width);
        int isLeft = index % picture->width == 0;
        int isRight = isRightBorder(index, picture->width, picture->height);

        if (isTop) {
            acumuledEnergyMapArray[index] = energyMapArray[index];
        } else {
            acumuledEnergyMapArray[index] =  energyMapArray[index] + energyMapArray [index - picture->width];
            if (isLeft) {
                rightFather =  energyMapArray[index] + energyMapArray [index - picture->width + 1];
                if (acumuledEnergyMapArray[index] > rightFather) { acumuledEnergyMapArray[index] = rightFather; }
            } else if (isRight) {
                leftFather =  energyMapArray[index] + energyMapArray [index - picture->width - 1];
                if (acumuledEnergyMapArray[index] > leftFather) { acumuledEnergyMapArray[index] = leftFather; }
            } else {
                rightFather =  energyMapArray[index] + energyMapArray [index - picture->width + 1];
                leftFather =  energyMapArray[index] + energyMapArray [index - picture->width - 1];
                if (acumuledEnergyMapArray[index] > rightFather) { acumuledEnergyMapArray[index] = rightFather; }
                if (acumuledEnergyMapArray[index] > leftFather) { acumuledEnergyMapArray[index] = leftFather; }
            }
        }
    }

}

int returnLowerIndex (Img* picture, const int* acumuledEnergyMapArray);
int returnLowerIndex (Img* picture, const int* acumuledEnergyMapArray) {
    int index = -1;
    int value = -1;
    int size = picture->width*picture->height;
    for (int i = size - picture->width - 1; i < size; i++) {
        if (index == -1 || acumuledEnergyMapArray[i] < value) {
            value = acumuledEnergyMapArray[i];
            index = i;
        }
    }
    return index;
}

int smallerNumber (int value01, int value02, int value03);
int smallerNumber (int value01, int value02, int value03) {
    if (value01 < value02 && value01 < value03) { return value01; }
    if (value02 < value03) { return value02; }
    return value03;
}

int main(int argc, char** argv)
{
    if(argc < 2) {
        printf("seamcarving [origem] [mascara]\n");
        printf("Origem é a imagem original, mascara é a máscara desejada\n");
        exit(1);
    }
    glutInit(&argc,argv);

    // Define do modo de operacao da GLUT
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    // pic[0] -> imagem original
    // pic[1] -> máscara desejada
    // pic[2] -> resultado do algoritmo

    // Carrega as duas imagens
    load(argv[1], &pic[0]);
    load(argv[2], &pic[1]);

    if(pic[0].width != pic[1].width || pic[0].height != pic[1].height) {
        printf("Imagem e máscara com dimensões diferentes!\n");
        exit(1);
    }

    // A largura e altura da janela são calculadas de acordo com a maior
    // dimensão de cada imagem
    width = pic[0].width;
    height = pic[0].height;

    // A largura e altura da imagem de saída são iguais às da imagem original (1)
    pic[2].width  = pic[1].width;
    pic[2].height = pic[1].height;

    // Especifica o tamanho inicial em pixels da janela GLUT
    glutInitWindowSize(width, height);

    // Cria a janela passando como argumento o titulo da mesma
    glutCreateWindow("Seam Carving");

    // Registra a funcao callback de redesenho da janela de visualizacao
    glutDisplayFunc(draw);

    // Registra a funcao callback para tratamento das teclas ASCII
    glutKeyboardFunc (keyboard);

    // Cria texturas em memória a partir dos pixels das imagens
    tex[0] = SOIL_create_OGL_texture((unsigned char*) pic[0].img, pic[0].width, pic[0].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
    tex[1] = SOIL_create_OGL_texture((unsigned char*) pic[1].img, pic[1].width, pic[1].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    // Exibe as dimensões na tela, para conferência
    printf("Origem  : %s %d x %d\n", argv[1], pic[0].width, pic[0].height);
    printf("Destino : %s %d x %d\n", argv[2], pic[1].width, pic[0].height);
    sel = 0; // pic1

    // Define a janela de visualizacao 2D
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0.0,width,height,0.0);
    glMatrixMode(GL_MODELVIEW);

    // Aloca memória para a imagem de saída
    pic[2].img = malloc(pic[1].width * pic[1].height * 3); // W x H x 3 bytes (RGB)
    // Pinta a imagem resultante de preto!
    memset(pic[2].img, 0, width*height*3);

    // Cria textura para a imagem de saída
    tex[2] = SOIL_create_OGL_texture((unsigned char*) pic[2].img, pic[2].width, pic[2].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    // Entra no loop de eventos, não retorna
    glutMainLoop();
}


// Gerencia eventos de teclado
void keyboard(unsigned char key, int x, int y)
{
    if(key==27) {
      // ESC: libera memória e finaliza
      free(pic[0].img);
      free(pic[1].img);
      free(pic[2].img);
      exit(1);
    }
    if(key >= '1' && key <= '3')
        // 1-3: seleciona a imagem correspondente (origem, máscara e resultado)
        sel = key - '1';
    if(key == 's') {
        // Aplica o algoritmo e gera a saida em pic[2].img...
        // ...
        // ... (crie uma função para isso!)

        //for (int i = 0; i < pic[2].width * pic[2].height; i++) {
        //pic[2].img[i].r = 200;
        //pic[2].img[i].g = pic[2].img[i].b = 100;
        //}

        int * energyMapArray = energyMap(pic);
        int * acumuledEnergyMapArray = acumulatedEnergyMap(&pic[2], energyMapArray);
        int lowerIndex = returnLowerIndex(&pic[2], acumuledEnergyMapArray);

        int size = pic[2].width * pic[2].height;
        while (lowerIndex > 0) {

            acumuledEnergyMapArray[lowerIndex] = -100; // marca pixel pra deletar ele

            int isTop = lowerIndex < pic[2].width;
            int isBot = lowerIndex >= (pic[2].width * pic[2].height-pic[2].width);
            int isLeft = lowerIndex % pic[2].width == 0;
            int isRight = isRightBorder(lowerIndex, pic[2].width, pic[2].height);

            if (isTop) {
                break;
            } else {
                if (isLeft) {
                    lowerIndex = acumuledEnergyMapArray[lowerIndex-pic[2].width] <
                            acumuledEnergyMapArray[lowerIndex-pic[2].width + 1] ?
                            acumuledEnergyMapArray[lowerIndex-pic[2].width] :
                            acumuledEnergyMapArray[lowerIndex-pic[2].width + 1];
                } else if (isRight) {
                    lowerIndex = acumuledEnergyMapArray[lowerIndex-pic[2].width] <
                                 acumuledEnergyMapArray[lowerIndex-pic[2].width - 1] ?
                                 acumuledEnergyMapArray[lowerIndex-pic[2].width] :
                                 acumuledEnergyMapArray[lowerIndex-pic[2].width - 1];
                } else {

                    if (acumuledEnergyMapArray[lowerIndex-pic[2].width-1] < acumuledEnergyMapArray[lowerIndex-pic[2].width] &&
                            acumuledEnergyMapArray[lowerIndex-pic[2].width-1] < acumuledEnergyMapArray[lowerIndex-pic[2].width+1]) {
                        lowerIndex = lowerIndex-pic[2].width-1;
                    } else if (acumuledEnergyMapArray[lowerIndex-pic[2].width] < acumuledEnergyMapArray[lowerIndex-pic[2].width+1]) {
                        lowerIndex = lowerIndex - pic[2].width;
                    } else {
                        lowerIndex = lowerIndex-pic[2].width+1;
                    }
                }
            }
        }

        int aux = 0;
        for (int i = 0; i < size; i++) {
            if (acumuledEnergyMapArray[i] == -100) { continue; }
            pic[2].img[aux++] = pic[2].img[i];
        }

        pic[2].width--;


        // Chame uploadTexture a cada vez que mudar
        // a imagem (pic[2])
        uploadTexture();
    }
    glutPostRedisplay();
}

// Faz upload da imagem para a textura,
// de forma a exibi-la na tela
void uploadTexture()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
        pic[2].width, pic[2].height, 0,
        GL_RGB, GL_UNSIGNED_BYTE, pic[2].img);
    glDisable(GL_TEXTURE_2D);
}

// Callback de redesenho da tela
void draw()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Preto
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // Para outras cores, veja exemplos em /etc/X11/rgb.txt

    glColor3ub(255, 255, 255);  // branco

    // Ativa a textura corresponde à imagem desejada
    glBindTexture(GL_TEXTURE_2D, tex[sel]);
    // E desenha um retângulo que ocupa toda a tela
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);

    glTexCoord2f(0,0);
    glVertex2f(0,0);

    glTexCoord2f(1,0);
    glVertex2f(pic[sel].width,0);

    glTexCoord2f(1,1);
    glVertex2f(pic[sel].width, pic[sel].height);

    glTexCoord2f(0,1);
    glVertex2f(0,pic[sel].height);

    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Exibe a imagem
    glutSwapBuffers();
}
