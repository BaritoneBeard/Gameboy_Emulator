#include <iostream>
#include "gameboy.h"
#include <stdio.h>
#include <fstream>
using namespace std;
//bad alloc = file not found
//if bad alloc try root

//unsigned char rom[]={0x06,0x06,0x3e,0x00,0x80,0x05,0xc2,0x04,0x00,0x76};
//rom = new char[];

extern QApplication* app;
unsigned char graphicsRAM[8192];
int palette[4];
int tileset, tilemap, scrollx, scrolly;
char* rom;
int romSize;
Z80* z80;
int HBLANK=0, VBLANK=1, SPRITE=2, VRAM=3;
unsigned char workingRAM[0x2000];
unsigned char page0RAM[0x80];
int line=0, cmpline=0, videostate=0, keyboardColumn=0, horizontal=0;
int gpuMode=HBLANK;
int romOffset = 0x4000;
int keys1 = 0xf;
int keys0 = 0xf;
long totalInstructions=0;
int rombank;
int cartridgetype;
int romsizemask;
unsigned char spriteRAM[0x100];
int objpalette0[4];
int objpalette1[4];

void dma(int address);

void setControlByte(unsigned char b) { 
        tilemap=(b&8)!=0?1:0;
        tileset=(b&16)!=0?1:0;
 }
void setPalette(unsigned char b) { 
	palette[0]=b&3;
	palette[1]=(b>>2)&3;
	palette[2]=(b>>4)&3;
	palette[3]=(b>>6)&3;
}
void setObjpalette0(unsigned char b) { 
	objpalette0[0]=b&3;
	objpalette0[1]=(b>>2)&3;
	objpalette0[2]=(b>>4)&3;
	objpalette0[3]=(b>>6)&3;
}
void setObjpalette1(unsigned char b) { 
	objpalette1[0]=b&3;
	objpalette1[1]=(b>>2)&3;
	objpalette1[2]=(b>>4)&3;
	objpalette1[3]=(b>>6)&3;
}

unsigned char getVideoState() {
        int by=0;
        if(line==cmpline) by|=4;
        if(gpuMode==VBLANK) by|=1;
        if(gpuMode==SPRITE) by|=2;
        if(gpuMode==VRAM) by|=3;
        return (unsigned char)((by|(videostate&0xf8))&0xff);
}

unsigned char memoryRead(int address)
{
	if(address < 0x4000)
	{
		return rom[address]; //Returns code stored in memory address of rom
	}
	else if(address >= 0x4000 && address <= 0x7fff)
	{
		return rom[romOffset + address%0x4000];
	}
	else if(address >= 0x8000 && address <= 0x9fff)
	{
		return graphicsRAM[address%0x2000];
	}
	else if(address >= 0xc000 && address <= 0xdfff)
	{
		return workingRAM[address%0x2000];
	}
	else if(address>=0xfe00 && address <=0xfe9f)
	{
		return spriteRAM[address%0xff];//
	}
	else if(address >= 0xff80 && address <= 0xffff)
	{
		return page0RAM[address%0x80];
	}
	else if(address == 0xff00)
	{
		if((keyboardColumn&0x30)==0x10)
			return keys0;
		else
			return keys1;
	}
	else if(address == 0xff41)
	{
		return getVideoState();
	}
	else if(address == 0xff42)
	{
		return scrolly;
	}
	else if(address == 0xff43)
	{
		return scrollx;
	}
	else if(address == 0xff44)
	{
		return line;
	}
	else if (address == 0xff45)
	{
		return cmpline;
	}
	else
	{
		return 0;
	}

}

void memoryWrite(int address, unsigned char value)
{
	//printf("address = %x\n",address);

	if(cartridgetype == 1 || cartridgetype ==2 || cartridgetype ==3)
	{
		if(address >= 0x2000 && address <= 0x3fff)
		{
			value = value& 0x1f;
			if(value ==0)
				value =1;

			rombank = rombank & 0x60;
			rombank += value;
			romOffset = (rombank*0x4000)&romsizemask;
		}
		else if(address >= 0x4000 && address <=0x5fff)
		{
			value = value&3;

			rombank = rombank&0x1f;
			rombank |= value <<5;

			romOffset =(rombank*0x4000)&romsizemask;
		}
	}

	if(address >= 0x8000 && address <= 0x9fff)
	{
		graphicsRAM[address%0x2000] = value;
	}
	else if(address >= 0xc000 && address <= 0xdfff)
	{
		workingRAM[address%0x2000] = value;
	}
	else if(address >=0xfe00 && address <= 0xfe9f)
	{
		spriteRAM[address&0xff] = value;//
		//printf("spriting: %x\n",spriteRAM[address%0x2000]);
	}
	else if(address >= 0xff80 && address <= 0xffff)
	{
		page0RAM[address%0x80] = value;
	}
	else if(address == 0xff00)
	{
		keyboardColumn = value;
	}
	else if(address == 0xff40)
	{
		setControlByte(value);
	}
	else if(address == 0xff41)
	{
		videostate = value;
	}
	else if(address == 0xff42)
	{
		scrolly = value;
	}
	else if(address == 0xff43)
	{
		scrollx = value;
	}
	else if(address == 0xff44)
	{
		line = value;
	}
	else if(address == 0xff45)
	{
		cmpline = value;
	}
	else if(address == 0xff46)
	{
		//printf("calling\n");
		dma(value);
	}
	else if(address == 0xff47)
	{
		setPalette(value);
	}
	else if(address == 0xff48)
	{
		setObjpalette0(value);
	}
	else if(address == 0xff49)
	{
		setObjpalette1(value);
	}
	else
	{
	}
}

void dma(int address) {
	//printf("called\n");
	address=address<<8;
	for(int i=0; i<0xa0; i++){
		memoryWrite(0xfe00+i,memoryRead(address+i));
	}
}


void keydown(int value)
{
	cout << value <<endl;
	if(value == 40 || value == 114)		//right, D
		keys1 &= 0xe;
	else if(value == 38 || value == 113)	//left, A
		keys1 &= 0xd;
	else if(value == 25 || value == 111)	//Up, W or arrow
		keys1 &= 0xb;
	else if(value == 39 || value == 116)	//Down, S or arrow
		keys1 &= 0x7;
	else if(value == 52 || value == 45)	//A, Z or K
		keys0 &= 0xe;
	else if(value == 53 || value == 46)	//B, X or L
		keys0 &= 0xd;
	else if(value == 9 || value == 119)	//Start, ESC or DEL
		keys0 &= 0xb;
	else if(value == 50 || value == 62)	//Select, LSHIFT or RSHIFT
		keys0 &= 0x7;


	z80->throwInterrupt(0x10);
}
void keyup(int value)
{
	cout << value << endl;
	if(value == 40|| value == 114)		//right, D or arrow
		keys1 |= 1;
	else if(value == 38 || value == 113)	//left, A or arrow
		keys1 |= 2;
	else if(value == 25 || value == 111)	//Up, W or arrow
		keys1 |= 4;
	else if(value == 39 || value == 116)	//Down, S or arrow
		keys1 |= 8;
	else if(value == 52 || value == 45)	//A, Z or K
		keys0 |= 1;
	else if(value == 53 || value == 46)	//B, X or L
		keys0 |= 2;
	else if(value == 9 || value == 119)	//Start, esc or del
		keys0 |= 4;
	else if(value == 50 || value == 62)	//Select, Lshift or Rshift
		keys0 |= 8;

	z80->throwInterrupt(0x10);
}

void render(int row)
{
	for(int column = 0; column<160; column++)
	{
		int x = column;
		int y = row;

		x = (x+scrollx)&255;
		y = (y+scrolly)&255;

		int tilex = x/8;
		int tiley = y/8;	//might need to be a float

		int tileposition = tiley*32 + tilex;

		int tileindex=0;//make compiler happy
		int tileaddress=0;//make compiler happy
		if(tilemap == 0)
			tileindex = graphicsRAM[0x1800 + tileposition];
		else if(tilemap == 1)
			tileindex = graphicsRAM[0x1c00 + tileposition];


		if(tileset ==0)
		{
			if(tileindex>= 128)
				tileindex=tileindex-256;
			tileaddress = tileindex*16 + 0x1000;
		}
		else if(tileset == 1)
			tileaddress = tileindex*16;

		int xoffset = x%8;
		int yoffset = y%8;

		int row0 = graphicsRAM[tileaddress + yoffset*2];
		int row1 = graphicsRAM[tileaddress + yoffset*2 +1];

		int row0shifted = row0>>(7-xoffset);
		int row1shifted = row1>>(7-xoffset);

		int row0capture = row0shifted&1;
		int row1capture = row1shifted&1;

		int pixel = row1capture*2 + row0capture;
		int color = palette[pixel];
		//part 6
		for(int spnum=0;spnum<40;spnum++)
		{
			int spritey = spriteRAM[spnum*4+0]-16;
			int spritex = spriteRAM[spnum*4+1]-8;
			int tilenumber = spriteRAM[spnum*4+2];
			int options = spriteRAM[spnum*4+3];

			//indexes found on imrannazer.com link

			if(spritex +8 <= column || spritex > column)
				continue;
			else if(spritey +8 <= row || spritey > row)
				continue;
			else if((options&0x80)!=0)
				continue;
			else
			{

				tileaddress = tilenumber*16;
				xoffset = column-spritex;
				yoffset = row-spritey;
				if((options&0x40)!=0)
					yoffset=7-yoffset;
				if((options&0x20)!=0)
					xoffset=7-xoffset;

				row0 = graphicsRAM[tileaddress + yoffset*2];//try spriteRAM
				row1 = graphicsRAM[tileaddress + yoffset*2 +1];

				row0shifted = row0>>(7-xoffset);
				row1shifted = row1>>(7-xoffset);

				row0capture = row0shifted&1;
				row1capture = row1shifted&1;

				int palindex = row1capture*2 + row0capture;

				if(palindex == 0){}
				else
				{
					if((options&0x10)==0)
						color = objpalette0[palindex];
					else
						color = objpalette1[palindex];
				}
			}
		}
		updateSquare(column,row,color);
	}
}

int main(int argc, char* argv[])
{
        setup(argc,argv);

 	ifstream romfile("mario.gb", ios::in|ios::binary|ios::ate);
        streampos size=romfile.tellg();

        rom=new char[size];

        romSize=size;

        romfile.seekg(0,ios::beg);

        romfile.read(rom,size);

        romfile.close();

	z80 = new Z80(memoryRead, memoryWrite); //new z80 object
	z80->reset();	//z80 private function
	//z80->PC=0;	//set global PC in z80 object to 0
/*
	int n;
	ifstream vidfile("opus5.gb",ios::in);
	for(int i=0; i<8192; i++){
		int n;

		vidfile>>n;

		graphicsRAM[i]=(unsigned char)n;
 	}
	vidfile >> tileset;
        vidfile >> tilemap;
        vidfile >> scrollx;
        vidfile >> scrolly;


        vidfile >> palette[0];
        vidfile >> palette[1];
        vidfile >> palette[2];
        vidfile >> palette[3];
*/
	//int totalInstructions=0;


	rombank = 0;
	cartridgetype = rom[0x147]&3;
	int buff[] = {0x7fff,0xffff,0x1ffff,0x3ffff,0x7ffff,0xfffff,0x1fffff,0x3fffff,0x7fffff};
	int adr = rom[0x148];
	romsizemask =buff[adr];
//	printf("romsizemask: %x\n",romsizemask);

	while(1)
	{
		if(z80->halted == false)
		{
			z80->doInstruction();
		}
		//don instruction until halted
		//printf("PC: %d, instruction %s, A %d, B%d\n",z80->PC,z80->instruction,z80->A,z80->B);

  		if(z80->interrupt_deferred>0)
                {
                        z80->interrupt_deferred--;
                        if(z80->interrupt_deferred==1)
                        {
                                z80->interrupt_deferred=0;
                                z80->FLAG_I=1;
                        }
                }
                z80->checkForInterrupts();

		horizontal = (int) ((totalInstructions+1)%61);
		if(line >= 145)
			gpuMode = VBLANK;
		else if(horizontal <= 30)
			gpuMode = HBLANK;
		else if(horizontal >30 && horizontal <41)//
			gpuMode = SPRITE;
		else
			gpuMode = VRAM;

		if(horizontal == 0)
		{
			line++;
			if(line == 144)
				z80->throwInterrupt(1);
			if(line>=0&&line<144)
				render(line);
			if(line%153 == cmpline && (videostate&0x40)!=0)
			{
				z80->throwInterrupt(2);
			}
			if(line==153)
			{
				line = 0;
				onFrame();
			}//

		}
		totalInstructions++;

	}

//	app->exec();

}
