//#include "gameboy.pro"
#include "screen.h"
#include "Z80.h"

class gameboy
{
public:
unsigned char memoryRead(int adress);
void memoryWrite(int address, unsigned char value);
void render(int row);
void setControlByte(unsigned char b);
void setPalette(unsigned char b);
void setObjpalette0(unsigned char b);
void setObjpalette1(unsigned char b);
void dma(int address);
void keydown(int value);
void keyup(int value);
unsigned char getVideoState();
private:
};
