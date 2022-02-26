#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "sound.h"

#define LENGTH_X 320
#define LENGTH_Y 240
#define WIDTH 4
#define STEP 4
#define LEFT 1
#define UP 2
#define RIGHT 3
#define DOWN 4
#define UNDEFINED_DIR 0
#define INITIAL_SNAKE_LENGTH 4
#define MAX_SNAKE_LENGTH     100
#define INITIAL_SNAKE_POS_X  156
#define INITIAL_SNAKE_POS_Y  120
#define INITIAL_SNAKE_SPEED  10
#define SPEED_SCORE_COEFF    4

#define ALKIS_BUF_SIZE 62464				
#define ALKIS_COEFFICIENT 1		

//TIMER_HIGH + TIME_LOW = 1700000.
//Our timer speed is 100MHz so we wanted to have 60 FPS resolution at least.
//Theoritically we should set it to 1 666 000 but we take CPU calculation time
//into account and decreased the counter value.
#define TIMER_LOW 0b1111000010100000
#define TIME_HIGH 0b0000000000011001

int high1 = 0;
int high2 = 0;
int high3 = 0;
char highest_score1[42];
char highest_score2[42];
char highest_score3[42];
char current_player_name[30];
int player_name_index = 0;
char h1_name[31];
char h2_name[31];
char h3_name[31];
int highests[3]={0};
volatile uint8_t state_player_name_taking = 1;

void draw_background();
void set_start_point();
void print_highest_score();
void reset_parameters();
void print_game_name();
void set_highest_table();
void print_name_request();
void take_player_name();

void config_GIC(void);
void config_interrupt (int N, int CPU_target);
void disable_A9_interrupts (void);
void set_A9_IRQ_stack (void);
void config_KEYs (void);
void config_PS2();
void config_timer();
void enable_A9_interrupts (void);
void pushbutton_ISR (void);
void keyboard_ISR();
void timer_ISR();
void write_pixel(int x, int y, short colour);
void clear_screen();
void clear_charbuf();
void write_char(int x, int y, char c);
void print_score(int score);
void draw_snake(int x,int y);
void delete_snake(int x, int y);
void snake_display(uint16_t snake[LENGTH_X][LENGTH_Y]);
void set_coordinates_tick(uint16_t coordinates[LENGTH_X][LENGTH_Y], uint8_t direction,uint16_t start_point[2],uint16_t end_point[2]);
void gameover_display();
void draw_borders();
void eat_apple();
void set_apple();
void draw_apple(int x, int y);
void reset_game();


void sound_function(int *sound_buff,int sound_buff_size,int sound_coeff);
volatile int * audio_ptr = (int *) 0xFF203040; // audio port addres
volatile int * left_ptr = (int *) 0xFF203048; 
volatile int * right_ptr = (int *) 0xFF20304C; 


volatile unsigned int * timer_status = (unsigned int *) 0xFF202000;
volatile unsigned int * timer_control = (unsigned int *) 0xFF202004;
volatile unsigned int * timer_low = (unsigned int *) 0xFF202008;
volatile unsigned int * timer_high = (unsigned int *) 0xFF20200C;
volatile unsigned int * LEDs = (unsigned int *) (0xFF200000);

uint16_t snake_coordinates[LENGTH_X][LENGTH_Y]={0};
uint8_t next_direction=RIGHT;
uint8_t current_direction=RIGHT;
uint16_t start_point[2];
uint16_t end_point[2];
uint16_t apple_coordinates[2];
uint8_t gameover_flag=0;
uint32_t score_global= 0;
int player_score;
char player_name[30];

volatile uint32_t highest_score = 0;
uint8_t end_direction[100]={UNDEFINED_DIR};
uint8_t length=5;
volatile uint8_t sound_flag = 0; // calismiyor suan
uint16_t speed_coeff=INITIAL_SNAKE_SPEED;
uint8_t end_point_direction_index = 0;
uint8_t start_point_direction_index = 0;
uint8_t sleep_flag = 0;

int main (){

    disable_A9_interrupts ();
    set_A9_IRQ_stack ();
    config_GIC ();
    config_KEYs ();
    config_PS2();
    enable_A9_interrupts ();
    
    clear_screen();
    draw_background();

    end_direction[start_point_direction_index++]=RIGHT;
    end_direction[start_point_direction_index++]=RIGHT;
    end_direction[start_point_direction_index++]=RIGHT;
    end_direction[start_point_direction_index]=RIGHT;
   
    start_point[0] = INITIAL_SNAKE_POS_X; 
    start_point[1] = INITIAL_SNAKE_POS_Y; 
    end_point[0]=start_point[0]-16;
    end_point[1]=start_point[1];
    snake_coordinates[start_point[0]][start_point[1]]=2;
    snake_coordinates[start_point[0]-4][start_point[1]]=2;
    snake_coordinates[start_point[0]-8][start_point[1]]=2;
    snake_coordinates[start_point[0]-12][start_point[1]]=2;
    snake_coordinates[start_point[0]-16][start_point[1]]=2;
    
    print_game_name();
    print_name_request();
    take_player_name();
    while(state_player_name_taking){}
    clear_screen();
    print_game_name();
    snake_display(snake_coordinates);
    print_score(score_global);
    print_highest_score();
    //set_apple();
    draw_borders();
    config_timer();
    while (1){}

}

void print_game_name(){
    char *company_name = "SNAKE TRAPPED BEHIND WALLS BY BOGAZICI GAMES";
    int i = 18;
    while(*company_name){
        write_char(i,2,*company_name);
        company_name++;
        i++;
    }
}
void take_player_name(){
    print_name_request();
    //while(state_player_name_taking){}
}
void set_highest_table(){

    if(player_score > highests[2]){
        highests[2] = player_score;
        memset(h3_name,0,sizeof(h3_name));
        strcpy(h3_name,player_name);
    }
    if(player_score > highests[1]){
        int temp = highests[1];
        highests[1]=player_score;
        highests[2]=temp;
        char temp_name[30]={0};
        strcpy(temp_name,h2_name);
        memset(h2_name,0,sizeof(h2_name));
        strcpy(h2_name,player_name);
        memset(h3_name,0,sizeof(h3_name));
        strcpy(h3_name,temp_name);
    }

    if(player_score> highests[0]){
        int temp = highests[0];
        highests[0]=player_score;
        highests[1]=temp;
        char temp_name[30]={0};
        strcpy(temp_name,h1_name);
        memset(h1_name,0,sizeof(h1_name));
        strcpy(h1_name,player_name);
        memset(h2_name,0,sizeof(h2_name));
        strcpy(h2_name,temp_name);
    }
    
}
void print_name_request(){
    char* name_request="ENTER YOUR PLAYER NAME";
    int i=29;
    while(*name_request){
        write_char(i,23,*name_request);
        name_request++;
        i++;
    }
}
void print_highest_score(){
    char score_int[4]={0,0,0,0};
    char bosluk[2] = {": "};
    memset(highest_score1,0,sizeof(highest_score1));
    strcpy(highest_score1,"1. ");
    strcat(highest_score1,h1_name);
    memset(score_int,0,sizeof(score_int));
    sprintf(score_int,"%d",highests[0]);
    strcat(highest_score1,bosluk);
    strcat(highest_score1,score_int);
    
    memset(highest_score2,0,sizeof(highest_score2));
    strcpy(highest_score2,"2. ");
    strcat(highest_score2,h2_name);
    memset(score_int,0,sizeof(score_int));
    sprintf(score_int,"%d",highests[1]);
    
    strcat(highest_score2,bosluk);
    strcat(highest_score2,score_int);
    
    memset(highest_score3,0,sizeof(highest_score3));
    strcpy(highest_score3,"3. ");
    strcat(highest_score3,h3_name);
    memset(score_int,0,sizeof(score_int));
    sprintf(score_int,"%d",highests[2]);
    
    strcat(highest_score3,bosluk);
    strcat(highest_score3,score_int);
    
    int i=5;
    char *temp_ptr = highest_score1;
    while(*temp_ptr){
        write_char(i,47,*temp_ptr);
        temp_ptr++;
        i++;
    }
    
    i = 5;
    temp_ptr = highest_score2;
    while(*temp_ptr){
        write_char(i,49,*temp_ptr);
        temp_ptr++;
        i++;
    }
    i = 5;
    temp_ptr = highest_score3;
    while(*temp_ptr){
        write_char(i,51,*temp_ptr);
        temp_ptr++;
        i++;
    }

}


/**
 * @brief Play sound data from array.
 * 
 * @param sound_buff 
 * @param sound_buff_size 
 * @param sound_coeff 
 */
void sound_function(int *sound_buff,int sound_buff_size,int sound_coeff){
	int fifospace;
	int buffer_index = 0;

	fifospace = *(audio_ptr + 1); 					// read the audio port fifospace register (gereksiz)
	
	int i;  
    // check RARC, for >75% full  ,0x00000060= 96
    sound_flag = 0;
    for (i = 0; i < sound_coeff ; i++){     
        if ( (fifospace & 0x000000FF) > 0x00000060){	
            while((buffer_index < sound_buff_size) && (0 == sound_flag)){	
                if ( ((fifospace & 0x00FF0000)> 0x00600000) || ((fifospace & 0xFF000000)> 0x60000000) ){	
                    *left_ptr = sound_buff[buffer_index];
                    *right_ptr = sound_buff[buffer_index];
                    ++buffer_index;		
                }
                fifospace = *(audio_ptr + 1); // read the audio port fifospace register BITWISE ISLEMDEN SONRA ESKI HALINE GETIRMEK ICIN	
            }
            buffer_index = 0;     //yeniden alkisi duymak icin uncommentle
        }

        //If the sound is playing and we push space button. We have to stop 
        //playing sound.
        if(1 == sound_flag){
            sound_flag = 0;
            break;
        }
    }
}


/**
 * @brief background will be gray
 * 
 */
void draw_background(){
    int i;
    int j;
    for(i=0;i<=239;i++){
        for (j=0; j <= 319; j++){
            write_pixel(j,i,0);
        }
    }
}

/**
 * @brief This function is called when we want to restart the game parameters.
 * It will be called if the space button is pressed.
 */
void reset_parameters(){
    int i,j;
    for ( i = 0; i < LENGTH_X; i++){
        for ( j = 0;j < LENGTH_Y; j++){
            snake_coordinates[i][j]=0;
        }
    }
    next_direction=RIGHT;
    current_direction=RIGHT;
    gameover_flag=0;
    player_score = score_global;
    memset(player_name,0,sizeof(player_name));
    strcpy(player_name,current_player_name);
    memset(current_player_name,0,sizeof(current_player_name));
    player_name_index = 0;
    state_player_name_taking = 1;
    score_global=0;
    for(i = 0;i<MAX_SNAKE_LENGTH;i++){
        end_direction[i]=UNDEFINED_DIR;
    }
    length=5;
    speed_coeff=INITIAL_SNAKE_SPEED;
    end_point_direction_index = 0;
    start_point_direction_index = 0;

    end_direction[start_point_direction_index++]=RIGHT;
    end_direction[start_point_direction_index++]=RIGHT;
    end_direction[start_point_direction_index++]=RIGHT;
    end_direction[start_point_direction_index]=RIGHT;
   
    start_point[0] = INITIAL_SNAKE_POS_X; 
    start_point[1] = INITIAL_SNAKE_POS_Y; 
    end_point[0]=start_point[0]-16;
    end_point[1]=start_point[1];
    snake_coordinates[start_point[0]][start_point[1]]=2;
    snake_coordinates[start_point[0]-4][start_point[1]]=2;
    snake_coordinates[start_point[0]-8][start_point[1]]=2;
    snake_coordinates[start_point[0]-12][start_point[1]]=2;
    snake_coordinates[start_point[0]-16][start_point[1]]=2;

}


void __attribute__ ((interrupt)) __cs3_isr_irq (void)
{
    // Read the ICCIAR from the CPU Interface in the GIC
    int interrupt_ID = *((int *) 0xFFFEC10C);

    if (interrupt_ID == 73){    //Pushbutton interrupt. (will be removed)
        pushbutton_ISR ();
    }
    else if (interrupt_ID == 79){   //Keyboard interrupt.
        keyboard_ISR();
    }
    else if(interrupt_ID == 72){    //Interval timer interrupt.
        timer_ISR();
    }
    else{
        while(1);
    }

    *((int *) 0xFFFEC110) = interrupt_ID;
}

void __attribute__ ((interrupt)) __cs3_reset (void)
{
    while(1);
}

void __attribute__ ((interrupt)) __cs3_isr_undef (void)
{
    while(1);
}

void __attribute__ ((interrupt)) __cs3_isr_swi (void)
{
    while(1);
}

void __attribute__ ((interrupt)) __cs3_isr_pabort (void)
{
    while(1);
}

void __attribute__ ((interrupt)) __cs3_isr_dabort (void)
{
    while(1);
}

void __attribute__ ((interrupt)) __cs3_isr_fiq (void)
{
    while(1);
}

void reset_game(){
    //stop timer and reset its status
    *timer_control = 0;
    *timer_status  = 0;
    reset_parameters();
    set_highest_table(); 
    clear_screen();
    print_game_name();
    take_player_name();
    
    //timer and timer interrupt started again.
}

void set_coordinates_tick(uint16_t coordinates[LENGTH_X][LENGTH_Y], uint8_t direction,uint16_t start_point[2],uint16_t end_point[2]){

    //If the next direction is set by player in -180 degree reverse direction
    if(current_direction == RIGHT && direction==LEFT){
        direction=current_direction;
    }else if(current_direction == LEFT && direction==RIGHT){
        direction=current_direction;
    }else if(current_direction == DOWN && direction==UP){
        
        direction=current_direction;
    }else if(current_direction == UP && direction==DOWN){
        direction=current_direction;
    }

    //Game over condition 1 : If snake hits the wall
    if( (start_point[0]-STEP)<16 || (start_point[1]-STEP)<16 || (start_point[0]+STEP)>300 || (start_point[1]+STEP)>180){
        gameover_flag=1;
        return;
    }

    if(start_point_direction_index<99){
        start_point_direction_index++;
    }else{
        start_point_direction_index = 0;
    }
    end_direction[start_point_direction_index]=direction;


    //Case : Apple is eaten
    if(start_point[0]==apple_coordinates[0] && start_point[1]==apple_coordinates[1]){
        score_global++;
        coordinates[apple_coordinates[0]][apple_coordinates[1]] = 2;
        if(length<MAX_SNAKE_LENGTH){
            length++;
            switch(direction){
                case LEFT:
                    start_point[0]=start_point[0]-STEP;
                    coordinates[start_point[0]][start_point[1]]=2;
                    current_direction=LEFT;
                    if(start_point_direction_index<99){
                        start_point_direction_index++;
                    }else{
                        start_point_direction_index = 0;
                    }
                    end_direction[start_point_direction_index]=LEFT;
                    break;
                case UP:
                    start_point[1]=start_point[1]-STEP;
                    coordinates[start_point[0]][start_point[1]]=2;
                    current_direction=UP;
                    if(start_point_direction_index<99){
                        start_point_direction_index++;
                    }else{
                        start_point_direction_index = 0;
                    }
                    end_direction[start_point_direction_index]=UP;
                    break;
                case RIGHT:
                    start_point[0]=start_point[0]+STEP;
                    coordinates[start_point[0]][start_point[1]]=2;
                    current_direction=RIGHT;
                    if(start_point_direction_index<99){
                        start_point_direction_index++;
                    }else{
                        start_point_direction_index = 0;
                    }
                    end_direction[start_point_direction_index]=RIGHT;
                    break;
                case DOWN:
                    start_point[1]=start_point[1]+STEP;
                    coordinates[start_point[0]][start_point[1]]=2;
                    current_direction=DOWN;
                    if(start_point_direction_index<99){
                        start_point_direction_index++;
                    }else{
                        start_point_direction_index = 0;
                    }
                    end_direction[start_point_direction_index]=DOWN;
                    break;
                default:
                    start_point[0]=start_point[0]+STEP;
                    coordinates[start_point[0]][start_point[1]]=2;
                    break;
            }
        }
        set_apple();
        print_score(score_global);
    }

    if(LEFT == direction){
        //Check whether snake eats itself or not
        if(snake_coordinates[start_point[0]-STEP][start_point[1]]==2){
            gameover_flag=1;
            return;
        }
        coordinates[start_point[0]-STEP][start_point[1]]=2;
        start_point[0]=start_point[0]-STEP;
        current_direction=LEFT;
    }else if(UP==direction){
        if(snake_coordinates[start_point[0]][start_point[1]-STEP]==2){
            gameover_flag=1;
            return;
        }
        coordinates[start_point[0]][start_point[1]-STEP]=2;
        start_point[1]=start_point[1]-STEP;
        current_direction=UP;
        
    }else if(RIGHT == direction){
        if(snake_coordinates[start_point[0]+STEP][start_point[1]]==2){
            gameover_flag=1;
            return;
        }
        coordinates[start_point[0]+STEP][start_point[1]]=2;
        start_point[0]=start_point[0]+STEP;
        current_direction=RIGHT;
    }else if(DOWN == direction){
        if(snake_coordinates[start_point[0]][start_point[1]+STEP]==2){
            gameover_flag=1;
            return;
        }
        coordinates[start_point[0]][start_point[1]+STEP]=2;
        start_point[1]=start_point[1]+STEP;
        current_direction=DOWN;    
    }else{
        start_point[0]=start_point[0]+STEP;
        coordinates[start_point[0]][start_point[1]]=2;
    }


    if(end_direction[end_point_direction_index]==LEFT){
        coordinates[end_point[0]][end_point[1]]=3;
        end_point[0]=end_point[0]-STEP;
    }else if(end_direction[end_point_direction_index]==RIGHT){
        coordinates[end_point[0]][end_point[1]]=3;
        end_point[0]=end_point[0]+STEP;
    }else if(end_direction[end_point_direction_index]==UP){
        coordinates[end_point[0]][end_point[1]]=3;
        end_point[1]=end_point[1]-STEP;
    }else if(end_direction[end_point_direction_index]==DOWN){
        coordinates[end_point[0]][end_point[1]]=3;
        end_point[1]=end_point[1]+STEP;
    }
    end_direction[end_point_direction_index]=UNDEFINED_DIR;
    if(end_point_direction_index<99){
        end_point_direction_index++;
    }else{
        end_point_direction_index=0;
    }
}

void set_apple(){
    do{ 
        apple_coordinates[0]=(rand()%70)*4+20;
        apple_coordinates[1]=(rand()%40)*4+20;
    }while(snake_coordinates[apple_coordinates[0]][apple_coordinates[1]] == 1);

    snake_coordinates[apple_coordinates[0]][apple_coordinates[1]]=1;
}

void draw_borders(){
    int i,k;
    //Snake can move on x = [20,299] and y = [20,179] pixels
    //Down side
    for(i=18;i<=300;i++){
        write_pixel(i,179,0x1212);
        write_pixel(i,180,0x1212);
    }
    // Right side
    for(i=18;i<=180;i++){
        write_pixel(299,i,0x1212);
        write_pixel(300,i,0x1212);
    }
    //Up side
    for(i=18;i<=300;i++){
        write_pixel(i,18,0x1212);
        write_pixel(i,19,0x1212);
    }
    //left side
    for(i=18;i<=180;i++){
        write_pixel(18,i,0x1212);
        write_pixel(19,i,0x1212);
    }
}

void gameover_display(){
    int i;
    clear_screen();
    if(score_global > highest_score){
        highest_score = score_global;
        char* highest_text = "NEW RECORD!";
        i=35;
        while(*highest_text){
            write_char(i,23,*highest_text);
            i++;
            highest_text++;
        }
    }
    char *text="GAME OVER !";
    i=35;
    while(*text){
        write_char(i,27,*text);
        i++;
        text++;
    }
    char *score_text="SCORE:";
    char score_int[4];
    sprintf(score_int,"%d",score_global);
    i=37;
    while(*score_text){
        write_char(i,30,*score_text);
        i++;
        score_text++;
    }
    write_char(44,30,score_int[0]);
    write_char(45,30,score_int[1]);
    write_char(46,30,score_int[2]);
    write_char(47,30,score_int[3]);

    char *text2="PRESS SPACE TO PLAY AGAIN";
    i=28;
    while(*text2){
        write_char(i,33,*text2);
        i++;
        text2++;
    }
    sound_function(alkis_buffer,ALKIS_BUF_SIZE,ALKIS_COEFFICIENT);
    //gameover_flag = 0;

}

void snake_display(uint16_t snake[LENGTH_X][LENGTH_Y]){
    int i,k;
    for(i=16;i<LENGTH_X;i=i+WIDTH){
        for(k=16;k<LENGTH_Y;k=k+WIDTH){
            if(snake[i][k]==2){
                draw_snake(i,k);
            }else if(snake[i][k]==1){
                draw_apple(i,k);
            }else if(snake[i][k]==3){
                delete_snake(i,k);
                snake[i][k]=0;
            }
        }
    }
}

void draw_snake(int x, int y){
    int i,k;
    for(i=0; i<3;i++){
        for(k=0;k<3;k++){
            write_pixel(x+i,y+k,0x2CC2);
        }
    }
}

void draw_apple(int x, int y){
    int i,k;
    for(i=0; i<3;i++){
        for(k=0;k<3;k++){
            write_pixel(x+i,y+k,0xF020);
        }
    }
}

void delete_snake(int x, int y){
    int i,k;
    for(i=0;i<3;i++){
        for(k=0;k<3;k++){
            write_pixel(x+i,y+k,0);
        }
    }
}

void print_score(int score){
        char *score_text="PLAYER SCORE:";
        char score_int[4]={0,0,0,0};
        sprintf(score_int,"%d",score);
        int i=60;
        while(*score_text){
            write_char(i,47,*score_text);
            i++;
            score_text++;
        }
        write_char(73,47,score_int[0]);
        write_char(74,47,score_int[1]);
        write_char(75,47,score_int[2]);
        write_char(76,47,score_int[3]);
}

//Bu kisimda 200x200 kare oyun alani 0,0 dan baslatilmis bunun yerine biz 
//20,20 den baslatip 200x300 bir oyun alani yapabiliriz.
void write_pixel(int x, int y, short colour) {
    //VGA in base adresi zaten 0xc8000000 oluyor. Biz bunun uzerine 
    //x ve posizyonunu ekliyoruz.Manueldeki gibi hesaplama yapiyoruz
    //oncelikle. Burada x i bir bit sola kaydirmamizin sebebi manueldeki
    //pixel position hesaplama metodundan dolayi. y 11. bitten itibaren 8
    //bit ile ifade ediliyor. x ten 2. bitten itibaren 9 bitle ifade ediliyor.
    volatile short *vga_addr=(volatile short*)(0xc8000000 + (y<<10) + (x<<1));
    *vga_addr=colour;
}

void write_char(int x, int y, char c) {
    // VGA character buffer
    volatile char * character_buffer = (char *) (0xc9000000 + (y<<7) + x);
    *character_buffer = c;
}

//clear edildikten sonra siyah basmak yerine baska bir sey basariz.
void clear_screen() {
  int x, y;
  for (x = 0; x < 320; x++) {
    for (y = 0; y < 240; y++) {
      write_pixel(x,y,0);
    }
  }
  clear_charbuf();
}

void clear_charbuf() {
    int x, y;
    for (x = 0; x < 80; x++) {
        for (y = 0; y < 60; y++) {
            write_char(x,y,' ');
        }
    }
}

void config_KEYs(){
    volatile int * KEY_ptr = (int *) 0xFF200050;
    *(KEY_ptr + 2) = 0xF;    // enable interrupts for all four KEYs
}

void config_PS2(){
    volatile int * PS2_address = (int *) 0xFF200100;	// PS/2 port address
    *(PS2_address) = 0xFF;				                /* reset */
    *(PS2_address + 1) = 0x1; 			                /* write to the PS/2 Control register to enable interrupts */
}

void config_timer(){

    *timer_status=0x00;    
    *timer_low =TIMER_LOW;       
    *timer_high=TIME_HIGH;   
    /**
     * timer control was set to 5 which means that we do not want it
     * to continue counting after counter reaches 0.So, we should start
     * it after each timer interrupt request.
    */
    *timer_control=0x05;   
}

void disable_A9_interrupts(void){
    int status = 0b11010011;
    asm("msr cpsr, %[ps]" : : [ps]"r"(status));
}

void set_A9_IRQ_stack(void){
    int stack, mode;
    stack = 0xFFFFFFFF - 7;      // top of A9 onchip memory, aligned to 8 bytes
    /* change processor to IRQ mode with interrupts disabled */
    mode = 0b11010010;
    asm("msr cpsr, %[ps]" : : [ps] "r" (mode));
    /* set banked stack pointer */
    asm("mov sp, %[ps]" : : [ps] "r" (stack));
    /* go back to SVC mode before executing subroutine return! */
    mode = 0b11010011;
    asm("msr cpsr, %[ps]" : : [ps] "r" (mode));
}

void enable_A9_interrupts(void){
    int status = 0b01010011;
    asm("msr cpsr, %[ps]" : : [ps]"r"(status));
}

void config_GIC(void){
    config_interrupt (73, 1);    // configure the FPGA KEYs interrupt (73)
    config_interrupt (79, 1);    // configure the FPGA PS/2 interrupt (79)
    config_interrupt (72, 1);    // configure the FPGA timer interrupt (72)
    // Set Interrupt Priority Mask Register (ICCPMR). Enable all priorities
    *((int *) 0xFFFEC104) = 0xFFFF;
    // Set the enable in the CPU Interface Control Register (ICCICR)
    *((int *) 0xFFFEC100) = 1;
    // Set the enable in the Distributor Control Register (ICDDCR)
    *((int *) 0xFFFED000) = 1;
}

void config_interrupt (int N, int CPU_target) {
    int reg_offset, index, value, address;
    /* Configure the Interrupt Set-Enable Registers (ICDISERn).
     * reg_offset = (integer_div(N / 32) * 4; value = 1 << (N mod 32) */
    reg_offset = (N >> 3) & 0xFFFFFFFC;
    index = N & 0x1F;
    value = 0x1 << index;
    address = 0xFFFED100 + reg_offset;
    /* Using the address and value, set the appropriate bit */
    *(int *) address |= value;
    /* Configure the Interrupt Processor Targets Register (ICDIPTRn)
     * reg_offset = integer_div(N / 4) * 4; index = N mod 4 */
    reg_offset = (N & 0xFFFFFFFC);
    index = N & 0x3;
    address = 0xFFFED800 + reg_offset + index;
    /* Using the address and value, write to (only) the appropriate byte */
    *(char *) address = (char) CPU_target;
}

void pushbutton_ISR( void ){
    /* KEY base address */
    volatile int *KEY_ptr = (int *) 0xFF200050;
    /* HEX display base address */
    volatile int *HEX3_HEX0_ptr = (int *) 0xFF200020;
    int press, HEX_bits;
    press = *(KEY_ptr + 3);
    *(KEY_ptr + 3) = press;
    if (press & 0x1)
        HEX_bits = 0b00111111;
    else if (press & 0x2)
        HEX_bits = 0b00000110;
    else if (press & 0x4)
        HEX_bits = 0b01011011;
    else
        HEX_bits = 0b01001111;
    *HEX3_HEX0_ptr = HEX_bits;
    return; 
}

void keyboard_ISR(){
    volatile int * PS2_address = (int *) 0xFF200100;
    uint8_t key1;
    int PS2_data;
    PS2_data = *(PS2_address);	// Read the PS/2 register. Clears interrupt bit.
   
    key1 = (PS2_data & 0xFF);
    if(0 == state_player_name_taking){
    
        switch(key1){
            case 0x6B:
                next_direction=LEFT;
                break;
            case 0x75:
                next_direction=UP;
                break;
            case 0x74:
                next_direction=RIGHT;
                break;
            case 0x72:
                next_direction=DOWN;
                break;
            case 0x29:
                sound_flag = 1;
                reset_game();
                break;
            default:
                break;
        }
    }else{
        if(0x29 == key1){
            sound_flag =1;
            reset_game();
        }else{
            if(player_name_index<30){
                if((key1==0x1C)){
                    current_player_name[player_name_index] ='A';  
                    write_char(29+player_name_index,25,'A');
                    player_name_index++;
                    
                }else if(key1==0x32){
                    current_player_name[player_name_index] ='B';  
                    write_char(29+player_name_index,25,'B');
                    player_name_index++;
                    
                }else if (key1==0x21){
                    current_player_name[player_name_index] ='C';  
                    write_char(29+player_name_index,25,'C');
                    player_name_index++;
                    
                }else if(key1==0x23){
                    current_player_name[player_name_index] ='D';  
                    write_char(29+player_name_index,25,'D');
                    player_name_index++;
                    
                }else if(key1==0x24){
                    current_player_name[player_name_index] ='E';  
                    write_char(29+player_name_index,25,'E');
                    player_name_index++;
                    
                }else if (key1==0x2B){
                    current_player_name[player_name_index] ='F';  
                    write_char(29+player_name_index,25,'F');
                    player_name_index++;
                    
                }else if (key1==0x34){
                    
                    current_player_name[player_name_index] ='G';  
                    write_char(29+player_name_index,25,'G');
                    player_name_index++;
                    
                } else if(key1==0x33){
                    current_player_name[player_name_index] ='H';  
                    write_char(29+player_name_index,25,'H');
                    player_name_index++;
                    
                }else if(key1==0x43){
                    
                    current_player_name[player_name_index] ='I';  
                    write_char(29+player_name_index,25,'I');
                    player_name_index++;
                    
                }else if(key1==0x3B){
                    
                    current_player_name[player_name_index] ='J';  
                    write_char(29+player_name_index,25,'J');
                    player_name_index++;
                    
                }else if(key1==0x42){
                    current_player_name[player_name_index] ='K';  
                    write_char(29+player_name_index,25,'K');
                    player_name_index++;
                    
                }else if(key1==0x4B){
                    
                    current_player_name[player_name_index] ='L';  
                    write_char(29+player_name_index,25,'L');
                    player_name_index++;
                }else if(key1==0x3A){
                    current_player_name[player_name_index] ='M';  
                    write_char(29+player_name_index,25,'M');
                    player_name_index++;
                } else if(key1==0x31){
                    
                    current_player_name[player_name_index] ='N';  
                    write_char(29+player_name_index,25,'N');
                    player_name_index++;
                } else if(key1==0x44){
                    current_player_name[player_name_index] ='O';  
                    write_char(29+player_name_index,25,'O');
                    player_name_index++;
                }else if(key1==0x4D){
                    current_player_name[player_name_index] ='P';  
                    write_char(29+player_name_index,25,'P');
                    player_name_index++;
                    
                } else if(key1==0x15){
                    
                    current_player_name[player_name_index] ='Q';  
                    write_char(29+player_name_index,25,'Q');
                    player_name_index++;
                }else if(key1==0x2D){
                    
                    current_player_name[player_name_index] ='R';  
                    write_char(29+player_name_index,25,'R');
                    player_name_index++;
                }else if(key1==0x1B){

                    current_player_name[player_name_index] ='S';  
                    write_char(29+player_name_index,25,'S');
                    player_name_index++;
                }else if(key1==0x2C){
                    
                    current_player_name[player_name_index] ='T';  
                    write_char(29+player_name_index,25,'T');
                    player_name_index++;
                }else if(key1==0x3C){
                    
                    current_player_name[player_name_index] ='U';  
                    write_char(29+player_name_index,25,'U');
                    player_name_index++;
                }else if(key1==0x2A){
                    
                    current_player_name[player_name_index] ='V';  
                    write_char(29+player_name_index,25,'V');
                    player_name_index++;

                }else if(key1==0x1D){
                    
                    current_player_name[player_name_index] ='W';  
                    write_char(29+player_name_index,25,'W');
                    player_name_index++;

                }else if(key1==0x22){

                    current_player_name[player_name_index] ='X';  
                    write_char(29+player_name_index,25,'X');
                    player_name_index++;
                }else if(key1==0x35){
                    current_player_name[player_name_index] ='Y';  
                    write_char(29+player_name_index,25,'Y');
                    player_name_index++;

                }else if(key1==0x1A){
                    current_player_name[player_name_index] ='Z';  
                    write_char(29+player_name_index,25,'Z');
                    player_name_index++;
                }else if(0x66 == key1){
                    if(player_name_index>0){
                        player_name_index--;
                        current_player_name[player_name_index] = ' ';
                        write_char(29+player_name_index,25,' ');
                    }else{
                        current_player_name[player_name_index] = ' ';
                        write_char(29+player_name_index,25,' ');
                    }
                }else if(0x5A == key1){
                    state_player_name_taking = 0;
                    *timer_status = 0;                    
                    clear_screen();
                    print_game_name();
                    draw_borders();
                    set_apple();
                    print_score(score_global);
                    print_highest_score();

                    *timer_control = 0x05;
                }else{

                }
                sleep_flag =1;
            }else{
                if(0x66 == key1){
                    player_name_index--;
                    current_player_name[player_name_index] = 0;
                    write_char(29+player_name_index,25,' ');
                }else if( 0x5A == key1){
                    state_player_name_taking = 0;
                    *timer_status = 0;
                    clear_screen();
                    print_game_name();
                    draw_borders();
                    set_apple();
                    print_score(score_global);
                    print_highest_score();
                    *timer_control = 0x05;
                }else{}
                sleep_flag =1;
            }
        }
    
    }
}

void timer_ISR(){
    //with speed coeff, we control the speed of the speed.
    //we display snake on screen for each 18 timer interrupt.

    if(0 == --speed_coeff){
        set_coordinates_tick(snake_coordinates,next_direction,start_point,end_point);
        if(score_global < SPEED_SCORE_COEFF){
            speed_coeff = INITIAL_SNAKE_SPEED;
        }else if(score_global < SPEED_SCORE_COEFF*2){
            speed_coeff = 7;
        }else if(score_global < SPEED_SCORE_COEFF*3){
            speed_coeff = 4;
        }else{
            speed_coeff = 2;
        }
    }
    
    if(gameover_flag==0){
        *timer_status=0x00;
        snake_display(snake_coordinates);
        *timer_control=0x05;
    }else{
        //stopping timer and interrupt
        *timer_control = 0;
        *timer_status = 0;
        set_highest_table();
        gameover_display();
    }

    
    return;
}


