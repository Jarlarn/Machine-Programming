/*
 * 	Easy Pong program for Assembly programming done in C.
 *
 */
 #include <string.h>
__attribute__((naked)) __attribute__((section (".start_section")) )
void startup ( void )
{
__asm__ volatile(" LDR R0,=0x2001C000\n");		/* set stack */
__asm__ volatile(" MOV SP,R0\n");
__asm__ volatile(" BL main\n");					/* call main */
__asm__ volatile(".L1: B .L1\n");				/* never return */
}



#define GPIO_D 0x40020C00
#define GPIO_MODER  ((volatile unsigned int *) (GPIO_D))  
#define GPIO_OTYPER  ((volatile unsigned short *) (GPIO_D+0x4))  
#define GPIO_PUPDR ((volatile unsigned int *) (GPIO_D+0xC))
  
#define GPIO_IDR_LOW ((volatile unsigned char *) (GPIO_D+0x10))  
#define GPIO_IDR_HIGH  ((volatile unsigned char *) (GPIO_D+0x11))  
#define GPIO_ODR_LOW ((volatile unsigned char *) (GPIO_D+0x14))  
#define GPIO_ODR_HIGH ((volatile unsigned char *) (GPIO_D+0x15)) 

#define PORT_BASE 0x40021000 // Port E 
#define portModer ((volatile unsigned int *)  PORT_BASE)
#define portOtyper ((volatile unsigned short *) (PORT_BASE+0x4))
#define portOspeedr ((volatile unsigned int *) (PORT_BASE+0x8))
#define portPupdr ((volatile unsigned int *) (PORT_BASE+0xC))
#define portIdrLow ((volatile unsigned char *) (PORT_BASE+0x10))
#define portIdrHigh ((volatile unsigned char *) (PORT_BASE+0x11))
#define portOdrLow ((volatile unsigned char *) (PORT_BASE+0x14))	
#define portOdrHigh ((volatile unsigned char *) (PORT_BASE+0x15))

__attribute__((naked))
void graphic_initialize(void){
    __asm volatile (" .HWORD 0xDFF0\n");
    __asm volatile ("BX LR\n");
}
__attribute__((naked))
void graphic_clear_screen(void){
    __asm volatile (".HWORD 0xDFF1 \n");
    __asm volatile ("BX LR \n");
    
}
__attribute__((naked))
void graphic_pixel_set(int x, int y){
    __asm volatile (".HWORD 0xDFF2 \n");
    __asm volatile ("BX LR \n");
}
__attribute__((naked))
void graphic_pixel_clear(int x, int y){
    __asm volatile (".HWORD 0xDFF3 \n");
    __asm volatile ("BX LR \n");
}

void swap(unsigned char* a, unsigned char* b){
	unsigned char temp = *a;
	*a = *b;
	*b = temp;
	
}

#define STK_CTRL ((volatile unsigned int *)(0xE000E010))  
#define STK_LOAD ((volatile unsigned int *)(0xE000E014))  
#define STK_VAL  ((volatile unsigned int *)(0xE000E018))


#define B_E 0x40
#define B_SELECT 4
#define B_RW 2
#define B_RS 1

void app_init(){
	*((unsigned long *) 0x40023830) = 0x18;

    *((volatile unsigned int *)0x40020C08) = 0x55555555; // MEDIUM SPEED
    * ( (volatile unsigned int *) 0x40020C00) &= 0x00000000; // MODER CONFIG
    * ( (volatile unsigned int *) 0x40020C00) |= 0x55005555; // MODER CONFIG
    * ( (volatile unsigned short *) 0x40020C04) &= 0x0000; // TYPER CONFIG
    * ( (volatile unsigned int *) 0x40020C0C) &= 0x00000000; // PUPDR CONFIG
    * ( (volatile unsigned int *) 0x40020C0C) |= 0x0000AAAA; // PUPDR CONFIG
	
}

// ------------------------------------------------------- keypad ------------------------------------------------------------------------------- //
void kbdActivate( unsigned int row ){ /* Aktivera angiven rad hos tangentbordet, eller
* deaktivera samtliga */
	switch( row )
	{
		case 1: *GPIO_ODR_HIGH = 0x10; break;
		case 2: *GPIO_ODR_HIGH = 0x20; break;
		case 3: *GPIO_ODR_HIGH = 0x40; break;
		case 4: *GPIO_ODR_HIGH = 0x80; break;
		case 0: *GPIO_ODR_HIGH = 0x00; break;
	}
}

int kbdGetCol ( void ){ /* Om någon tangent (i aktiverad rad)
* är nedtryckt, returnera dess kolumnnummer,
* annars, returnera 0 */
	unsigned char c;
	c = *GPIO_IDR_HIGH;
	if ( c & 0x8 ) return 4;
	if ( c & 0x4 ) return 3;
	if ( c & 0x2 ) return 2;
	if ( c & 0x1 ) return 1;
	return 0;
}

unsigned char keyb(void){
	unsigned char key[]={1,2,3,0xA,4,5,6,0xB,7,8,9,0xC,0xE,0,0xF,0xD};
	int row, col;
	for(row=1; row <=4 ; row++ ) {
		kbdActivate( row );
		if( (col = kbdGetCol () ) )
		{
		kbdActivate( 0 );
		return key [4*(row-1)+(col-1) ];
		}
	}
	kbdActivate( 0 );
	return 0xFF;
}

// ------------------------------------------------DELAY------------------------------------------------------------------//
void delay_250ns(void){
	// page 98
	*STK_CTRL = 0; // Resets SysTick
	*STK_LOAD = (168 / 4) - 1; // Minus 1 comes from how to processor counts 
	*STK_VAL = 0; // Resets counter register 
	*STK_CTRL = 5; // Starts the count down 
	while(*STK_CTRL & 0x00010000){} // while the countflag is 1 
	*STK_CTRL = 0; // resets Systick 
}

void delay_micro(unsigned int ms){
	// When SIMULATOR is defined for some reason it gets stuck here. 	
	#ifdef SIMULATOR
		ms = ms / 1000;
		ms++;
	#endif
	// 4 * 250 ns = 1 µs
	for (int i = 0; i < ms; i++)
	{
		delay_250ns();
		delay_250ns();
		delay_250ns();
		delay_250ns();
	}

}

void delay_milli(unsigned int ms){
	#ifdef SIMULATOR
		ms = ms / 1000;
		ms++;
	#endif
	// 1000 µs = 1 ms 
	
	delay_micro(ms * 1000);
}

// ------------------------------------------------DELAY------------------------------------------------------------------//

typedef struct {
    unsigned char x,y;
} POINT, *PPOINT;

typedef struct{
	POINT p0;
	POINT p1;
}LINE, *PLINE;

typedef struct rect {
	POINT p;
	unsigned char x,y;
}RECT, *PRECT;

typedef struct polygonpoint {
	char x,y;
	struct polygonpoint *next;
}POLYPOINT, *PPOLYPOINT;

#define MAX_POINTS 30
typedef struct {
	int numpoints;
	int sizex;
	int sizey;
	POINT px[MAX_POINTS];
}GEOMETRY, *PGEOMETRY;

GEOMETRY ball_geometry = {
	12, //numpoints
	4,4, //sizex,y
	{
		{0,1},{0,2},{1,0},{1,1},{1,2},{1,3},{2,0},{2,1},{2,2},{2,3},{3,1},{3,2}
	}
};

typedef struct tObj{
	PGEOMETRY geo;
	int dirx,diry;
	int posx,posy;
	void (* draw) (struct tObj *);
	void (* clear) (struct tObj *);
	void (* move) (struct tObj *);
	void (* set_speed) (struct tObj *,int,int);
}OBJECT,*POBJECT;

void draw_ballobject(POBJECT o) {
	int pixels = o->geo->numpoints;
	for (int i = 0; i < pixels; i++) {
		graphic_pixel_set(o->posx + (o->geo->px+i)->x, o->posy + (o->geo->px+i)->y);
	}
} 
void clear_ballobject(POBJECT o){
	int pixels = o->geo->numpoints;
	for (int i = 0; i < pixels; i++) {
		graphic_pixel_clear(o->posx + (o->geo->px+i)->x, o->posy + (o->geo->px+i)->y);
	}
}

void move_ballobject (POBJECT o){
	clear_ballobject(o);
	signed int new_pos_x = (o->posx)+(o->dirx);
	signed int new_pos_y = (o->posy)+(o->diry);
	if (new_pos_x < 1  ){
		int dir = -o->dirx;
		
		o->set_speed(o,dir,o->diry);
	}
	if (new_pos_y < 1 || new_pos_y > 60 ){
		int dir = -o->diry;
		o->set_speed(o,o->dirx,dir);
	}
	o->posx = new_pos_x;
	o->posy = new_pos_y;
	draw_ballobject(o);
}

void set_ballobject_speed(POBJECT o, int speedx, int speedy){
	o->dirx = speedx;
	o->diry = speedy;
}

GEOMETRY racket_geometry = {
	27, //numpoints
	5,9, //sizex,y
	{
		{0,0},{1,0},{2,0},{3,0},{4,0},{0,1},{4,1},{0,2},{4,2},{0,3},{2,3},{4,3},{0,4},
		{2,4},{4,4},{0,5},{2,5},{4,5},{0,6},{4,6},{0,7},{4,7},{0,8},{1,8},{2,8},{3,8},{4,8}
	}
};

void move_paddleobject (POBJECT p){
	clear_ballobject(p);
	signed int new_pos_y = (p->posy)+(p->diry);
	
	if (new_pos_y < 1 || new_pos_y > 55 ){
		new_pos_y = (p->posy)+(p->diry) * -1;
	}
	p->posy = new_pos_y;
	draw_ballobject(p);
	
	
	/*
	
	clear_ballobject(p);
	int new_pos_y = (p->posy)+(p->diry);
	
	if (new_pos_y > -1 && new_pos_y < 58){
		p->posy = new_pos_y;
	}
	draw_ballobject(p);
	 */
}
static OBJECT ball = {
	&ball_geometry,
	4,1,
	1,1,
	draw_ballobject,
	clear_ballobject,
	move_ballobject,
	set_ballobject_speed
};

static OBJECT PADDLE = {
	&racket_geometry,
	10,0,
	10,25,
	draw_ballobject,
	clear_ballobject,
	move_paddleobject,
	set_ballobject_speed
};
static OBJECT PADDLE2 = {
	&racket_geometry,
	0,0,
	115,25,
	draw_ballobject,
	clear_ballobject,
	move_paddleobject,
	set_ballobject_speed
};
//----------------------------------SCORE AND BOUNCE-----------------------

int score1,score2 = 0;



void reset (POBJECT o, POBJECT p, POBJECT p1){
	
	graphic_clear_screen();
	p -> posx = 10;
	p -> posy = 25;
    p1 -> posx = 115;
    p1->posy = 25;
	o -> posx = 1;
	o -> posy = 1;
	p -> draw(p);
    p1->draw(p1);
	o -> draw(o);
}

void hard_reset (POBJECT o, POBJECT p, POBJECT p1){
	
	graphic_clear_screen();
	p -> posx = 10;
	p -> posy = 25;
    p1 -> posx = 115;
    p1->posy = 25;
	o -> posx = 1;
	o -> posy = 1;
	p -> draw(p);
    p1->draw(p1);
	o -> draw(o);
	score1 =0;
    score2 = 0;
}

int pixel_overlap(POBJECT o1, POBJECT o2) {
  int offset1x = o1->posx;
  int offset1y = o1->posy;
  int offset2x = o2->posx;
  int offset2y = o2->posy;
  for (int i = 0; i < o1->geo->numpoints; i++) {
    for (int j = 0; j < o2-> geo->numpoints; j++)
      if ((offset1x + o1->geo->px[i].x == offset2x + o2->geo->px[j].x) &&
        (offset1y + o1->geo->px[i].y == offset2y + o2->geo->px[j].y)) return 1;
  }
  return 0;
}


//---------------------------------- ASCII-----------------------------
void ascii_ctrl_bit_set(char x)
{
	char c;
	c = *portOdrLow;
	*portOdrLow = B_SELECT | x | c;
}

void ascii_ctrl_bit_clear(char x)
{
	char c;
	c = *portOdrLow;
	c = c & ~x;
	*portOdrLow = B_SELECT | c;
}

void ascii_write_controller(unsigned char byte)
{
	// These delays are need for the processor to execute the respective functions.
	delay_250ns(); // 40ns
	ascii_ctrl_bit_set(B_E);
	*portOdrHigh = byte;
	delay_250ns(); //230ns
	ascii_ctrl_bit_clear(B_E);
	delay_250ns(); // 10ns 
}

void ascii_write_cmd(unsigned char command)
{
	ascii_ctrl_bit_clear(B_RS);
	ascii_ctrl_bit_clear(B_RW);
	ascii_write_controller(command);
}

void ascii_write_data(unsigned char data)
{
	ascii_ctrl_bit_set(B_RS);
	ascii_ctrl_bit_clear(B_RW);
	ascii_write_controller(data);
}

unsigned char ascii_read_controller(void)
{
	ascii_ctrl_bit_set(B_E);
	delay_250ns();
	delay_250ns(); // 360ns
	unsigned char rv = *portIdrHigh;
	ascii_ctrl_bit_clear(B_E);
	return rv;
}

unsigned char ascii_read_status(void)
{
	*portModer = 0x00005555; // Set bit15-8 as input 
	ascii_ctrl_bit_clear(B_RS);
	ascii_ctrl_bit_set(B_RW);
	unsigned char rv = ascii_read_controller();
	*portModer = 0x55555555; // Set bit15-8 as output
	return rv;
}

unsigned char ascii_read_data(void)
{
	*portModer = 0x00005555; // Set bit15-8 as input 
	ascii_ctrl_bit_set(B_RS);
	ascii_ctrl_bit_set(B_RW);
	unsigned char rv = ascii_read_controller();
	*portModer = 0x55555555; // Set bit15-8 as output 
	return rv;
}

void ascii_command(unsigned char command)
{
	while( (ascii_read_status() & 0x80) == 0x80) // Wait for the display to be ready for instructions
	{}
	delay_micro(8);
	ascii_write_cmd(command);
	delay_micro(45);
}

void ascii_init(void)
{
	ascii_command(0x38); // 2 rows, 5x8 
        delay_micro(39);

	ascii_command(0x0E);
    delay_micro(39);     // Activate display, activate cursor and set it as constant 
	ascii_command(0x01); // Clear Display
    delay_milli(1.53);
	ascii_command(0x06); // Increment, No shift
    delay_micro(39);
}

void ascii_write_char(unsigned char c)
{
	while( (ascii_read_status() & 0x80) == 0x80) // Wait for the display to be ready for instructions
	{}
	delay_micro(8);
	ascii_write_data(c);
	delay_micro(45);
}

void ascii_gotoxy(int x, int y)
{
	unsigned char adress = x - 1;
	if ( y == 2 )
	{
		adress = adress + 0x40;
	}
	ascii_write_cmd(0x80 | adress);
}

int print_score(int score1, int score2) {

    char *s;
    char score_1 = 48 + score1; 
    char score_2 = 48 + score2;
    
    char test1[20] = "Player 1 score: ";
    char test2[20] = "Player 2 score: ";
    int i = 0;
    
    if (score1 == 0) {
        test1[16] = '0';
        i = 1;
    }
    else {
        while (score1 > 0) {
            test1[16-i] = score1 % 10 + '0';
            score1 /= 10;
            i++;
        }
    }
    
    i = 0;
    if (score2 == 0) {
        test2[16] = '0';
        i = 1;
    }
    else {
        while (score2 > 0) {
            test2[16-i] = score2 % 10 + '0';
            score2 /= 10;
            i++;
        }
    }
    
    ascii_init();
    ascii_gotoxy(1,1);
    s = test1;
    while (*s)
        ascii_write_char (*s++);
    ascii_gotoxy(1,2);
    s = test2;
    while (*s)
        ascii_write_char(*s++);
    return 0;
}


void print_score0_0(void) {
    char *s;
    char test1[] = "Player 1 score: 0";
    char test2[] = "Player 2 score: 0";
    ascii_init();
    ascii_gotoxy(1,1);
    s = test1;
    while (*s)
        ascii_write_char (*s++);
    ascii_gotoxy(1,2);
    s = test2;
    while (*s)
        ascii_write_char(*s++);
}
void print_score1_0(void) {
    char *s;
    char test1[] = "Player 1 score: 1";
    char test2[] = "Player 2 score: 0";
    ascii_init();
    ascii_gotoxy(1,1);
    s = test1;
    while (*s)
        ascii_write_char (*s++);
    ascii_gotoxy(1,2);
    s = test2;
    while (*s)
        ascii_write_char(*s++);
}
void print_score2_0(void) {
    char *s;
    char test1[] = "Player 1 score: 2";
    char test2[] = "Player 2 score: 0";
    ascii_init();
    ascii_gotoxy(1,1);
    s = test1;
    while (*s)
        ascii_write_char (*s++);
    ascii_gotoxy(1,2);
    s = test2;
    while (*s)
        ascii_write_char(*s++);
}
void print_score0_1(void) {
    char *s;
    char test1[] = "Player 1 score: 0";
    char test2[] = "Player 2 score: 1";
    ascii_init();
    ascii_gotoxy(1,1);
    s = test1;
    while (*s)
        ascii_write_char (*s++);
    ascii_gotoxy(1,2);
    s = test2;
    while (*s)
        ascii_write_char(*s++);
}
void print_score0_2(void) {
    char *s;
    char test1[] = "Player 1 score: 0";
    char test2[] = "Player 2 score: 2";
    ascii_init();
    ascii_gotoxy(1,1);
    s = test1;
    while (*s)
        ascii_write_char (*s++);
    ascii_gotoxy(1,2);
    s = test2;
    while (*s)
        ascii_write_char(*s++);
}
void print_score1_1(void) {
    char *s;
    char test1[] = "Player 1 score: 1";
    char test2[] = "Player 2 score: 1";
    ascii_init();
    ascii_gotoxy(1,1);
    s = test1;
    while (*s)
        ascii_write_char (*s++);
    ascii_gotoxy(1,2);
    s = test2;
    while (*s)
        ascii_write_char(*s++);
}
void print_score2_1(void) {
    char *s;
    char test1[] = "Player 1 score: 2";
    char test2[] = "Player 2 score: 1";
    ascii_init();
    ascii_gotoxy(1,1);
    s = test1;
    while (*s)
        ascii_write_char (*s++);
    ascii_gotoxy(1,2);
    s = test2;
    while (*s)
        ascii_write_char(*s++);
}
void print_score1_2(void) {
    char *s;
    char test1[] = "Player 1 score: 1";
    char test2[] = "Player 2 score: 2";
    ascii_init();
    ascii_gotoxy(1,1);
    s = test1;
    while (*s)
        ascii_write_char (*s++);
    ascii_gotoxy(1,2);
    s = test2;
    while (*s)
        ascii_write_char(*s++);
}



int print_end1(void) {
    ascii_command(0x1);
    delay_milli(1.53);
    char *s;
    char test1[] = "Player 1 won! Congra";
    char test2[] = "tulations!";
    ascii_init();
    ascii_gotoxy(1,1);
    s = test1;
    while (*s)
        ascii_write_char (*s++);
    ascii_gotoxy(1,2);
    s = test2;
    while(*s) 
        ascii_write_char(*s++);
    return 0;
}

int print_end2(void) {
    ascii_command(0x1);
    delay_milli(1.53);
    char *s;
    char test1[] = "Player 2 won! Congra";
  char test2[] = "tulations!";
    ascii_init();
    ascii_gotoxy(1,1);
    s = test1;
    while (*s)
        ascii_write_char (*s++);
    ascii_gotoxy(1,2);
    s = test2;
    while(*s) 
        ascii_write_char(*s++);
    return 0;
}



void main(void)
{
    int count = 0;
    char c;
    POBJECT o = &ball;
    POBJECT p = &PADDLE;
    POBJECT p1 = &PADDLE2;
    app_init();
    ascii_init();
    graphic_initialize();
    graphic_clear_screen();
    print_score0_0();
    o->set_speed(o,4,1);
    
    while(1){

		p->move(p);
        p1->move(p1);
		o->move(o);
        c = keyb();
        
        switch(c){
            case 9: p1->set_speed(p1,0,3); break;
            case 3: p1->set_speed(p1,0,-3); break;
            case 1: p->set_speed(p,0,-3);break;
            case 7: p->set_speed(p,0,3);break;
            case 6: hard_reset(o,p,p1); break;
			default: p->set_speed(p, 0, 0); p1->set_speed(p1,0,0); break;
        }
        if (pixel_overlap(p,o) || pixel_overlap(p1,o)) { // Om bollen prickar paddeln, byta x riktning.
            o->dirx = -(o->dirx);
        }
        
        if (o->posx <1) { // Om bollen flyger ut till vänster, uppdatera score1, samma score 2.
            score2++;
            print_score(score1,score2);
            reset(o,p,p1);
            
        } else if (o->posx >127) {
            score1++;
            print_score(score1,score2);
            reset(o,p,p1);
            
        }
        
        
        /*
        if (score1 ==1 && score2 ==0 && count ==0) {
            print_score1_0();
            count++;
        } 
        if (score1 ==2 && score2 ==0 && count ==1) {
            print_score2_0();
            count++;
        } 
        if (score1 ==2 && score2 ==1 && count == 2) {
            print_score2_1();
            count++;
        } 
        if (score1 ==0 && score2 ==1 &&count ==0) {
            print_score0_1();
            count++;
        } 
        if (score1 ==0 && score2 ==2 && count ==1) {
            print_score0_2();
            count++;
        } 
        if (score1 ==1 && score2 ==1 && count ==1) {
            print_score1_1();
            count++;
        } 
        if (score1 ==1 && score2 ==2 && count ==2) {
            print_score1_2();
            count++;
        } 
        */
         if (score1 > 5) {
        print_end1();
        break;

    } else if (score2 >5) {
        print_end2();
        break;
    }
    
    
    
	} 
}
