
/**********************************************************************
关于刷新图层加速的部分没有实现，感觉没有太大的必要去理解学习这一部分，不是重点
 **********************************************************************/
#include<header.h>
#include<fontascii.h>

//需要用到的函数，halt,cli,out8,read_eflags,write_eflags,这些函数在x86.h中
//init _palette, set_palette 这两个函数我想放在screen.c中

#define black 0
#define red   1
#define green 2

extern TIMERCTL *gtimerctl;
void bootmain(void)
{

struct boot_info *bootp=(struct boot_info *)ADDR_BOOT;

init_screen((struct boot_info * )bootp);
init_palette();  //color table from 0 to 15


//draw_window();

int mx,my;//mouse position
//display mouse logo
char mousepic[16*16];     //mouse logo buffer

//display_mouse(bootp->vram,bootp->xsize,16,16,mx,my,mousepic,16);
cli();

//set gdt idt
init_gdtidt();
//remap irq 0-15
//函数中：　irq 1(keyboard)对应设置中断号int0x21,    irq　12(mouse)对应的中断号是int0x2c 要写中断服务程序了。
init_pic();

//设置完了gdt,idt后再enable cpu interrupt才是安全的

unsigned char s[40];		    //sprintf buffer
unsigned char keybuf[32];	    //keyfifo
unsigned char mousebuf[128];	//mousefifo
unsigned char data;		        //temp variable to get fifo data
int count=0;
fifo8_init(&keyfifo ,32,keybuf);//keyfifo是一个global data defined in int.c
fifo8_init(&mousefifo,128,mousebuf);


//enable timer ,keyboard and mouse   //1111 1000 后面的三个0代表 accept interrupt request, irq0=timer interrupt
outb(PIC0_IMR, 0xf8);//1111 1000  irq 1 2打开 因为keyboard是irq 1,irq2 enable 8259B 芯片发生的中断请求                                 // enable pic slave and keyboard interrupt
//enable mouse interrupt 1110 1111  irq 12 打开　mouse是irq 12  所以要把pic 1 pic 2的芯片中断响应位打开。
outb(PIC1_IMR, 0xef);
//初始化　鼠标按键控制电路
init_keyboard();




unsigned int memtotal;
memtotal=memtest(0x400000,0xffffffff);
Memman * memman=(Memman *)0x3c0000;
memman_init(memman);
memman_free(memman,0x400000,memtotal-0x400000);
char *desktop=(unsigned char*)memman_alloc(memman,320*200);
char *win_buf=(unsigned char*)memman_alloc_4K(memman,160*65);

TIMERCTL *timerctl=(TIMERCTL *)memman_alloc_4K(memman,sizeof(TIMERCTL));

gtimerctl=timerctl;
init_pit(timerctl);//init timerctl


draw_win_buf(desktop);
make_window8(win_buf,160,68,"timer");
init_mouse(mousepic,99);	//99　means background color
clear_screen(3);



sprintf(s,"memory:%dMB,free:%dMB,%d",memtotal>>20
,memman_avail(memman)>>20,memman->cellnum);
puts8(desktop ,bootp->xsize,0,150,0,s,Font8x16);

SHTCTL *shtctl;
shtctl=shtctl_init(memman,bootp->vram,bootp->xsize,bootp->ysize);

SHEET *sht_back,*sht_mouse,*sht_win;
//allocate a sheet space from shtctl
sht_back=sheet_alloc(shtctl);
sht_mouse=sheet_alloc(shtctl);
sht_win=sheet_alloc(shtctl);





//printdebug(timerctl,0);//0x412a00

//write something inside the window
//puts8(win_buf ,160,24,28,0,"hello ,easy os");
//puts8(win_buf ,160,24,44,0,"second line");//y=28+16=44



//hoop the buffer with sheet
sheet_setbuf(sht_back,desktop,320,200,-1);
sheet_setbuf(sht_mouse,mousepic,16,16,99);
sheet_setbuf(sht_win,win_buf,160,65,-1);




mx=0;my=0;//set mouse initial position
sheet_move(sht_back,0,0);
sheet_move(sht_mouse,mx,my);
sheet_move(sht_win,120,100);

//set sht_back to layer0 ;set sht_mouse to layer1
sheet_updown(sht_back,0);
sheet_updown(sht_win,1);
sheet_updown(sht_mouse,2);
//refresh a specific rectangle
sheet_refresh(sht_back,0,0,bootp->xsize,bootp->ysize);



struct FIFO8 timerfifo,timerfifo2,timerfifo3;
char timerbuf[8],timerbuf2[8],timerbuf3[8];
TIMER *timer,*timer2,*timer3;

fifo8_init(&timerfifo,8,timerbuf);
fifo8_init(&timerfifo2,8,timerbuf2);
fifo8_init(&timerfifo3,8,timerbuf3);

timer=timer_alloc(timerctl,0);
//printdebug(timer->flag,160);
//printdebug(timer,0);

timer2=timer_alloc(timerctl,1);
//printdebug(timer2,150);
//while(1);


timer3=timer_alloc(timerctl,2);
//printdebug(timer3,160);



timer_init(timer,&timerfifo,1);
timer_init(timer2,&timerfifo2,1);
timer_init(timer3,&timerfifo3,1);
//while(1);
timer_settime(timer,1000);
timer_settime(timer2,300);
timer_settime(timer3,100);
GUI_Init(Font8x16);
GUI_SetVbuf(desktop);
GUI_SetPensize(3);
GUI_DrawRect(8,47,10,250,80);
GUI_FillRect(black,50,12,250,80);
GUI_DispCharAt('a',200,0);



sheet_refresh(sht_back,0,0,bootp->xsize,bootp->ysize);
sti();//中断过早开启，在虚拟机上没有问题，但是在实体机上，直接就挂起了。可能与定时器有关
//enable cpu interrupt
struct MOUSE_DEC mdec;
enable_mouse(&mdec);   //这里会产生一个mouse interrupt


while(1)
 {


    puts8(win_buf ,160,24,28,0,"timeount",Font8x16);
    sprintf(s,"%d",timerctl->count);
    boxfill8(win_buf,160,8,20,44,140,60);
    puts8(win_buf ,160,20,44,0,s,Font8x16);//y=28+16=44
    sheet_refresh(sht_win,20,28,140,60);
      //cli();//disable cpu interrupt
     // sti();
    //while(1);
   if(fifo8_status(&keyfifo) +
   fifo8_status(&mousefifo)  +
   fifo8_status(&timerfifo)  +
   fifo8_status(&timerfifo2) +
   fifo8_status(&timerfifo3)
    == 0)//no data in keyfifo and mousefifo
    {
    sti();
   //hlt();//wait for interrupt
   }
   else
   {
      if(fifo8_status(&keyfifo) != 0)
      {
        data=fifo8_read(&keyfifo);
        sti();
      }//end of keyboard decoder
      else if(fifo8_status(&mousefifo) != 0)//we have mouse interrupt data to process
      {
        data=fifo8_read(&mousefifo);
        sti();
        if(mouse_decode(&mdec,data))
        {
              //3个字节都得到了
              switch (mdec.button)
              {
                case 1:s[1]='L';break;
                case 2:s[3]='R';break;
                case 4:s[2]='M';break;
              }
              sprintf(s,"[lmr:%d %d]",mdec.x,mdec.y);
              boxfill8(desktop,320,0,32,16,32+15*8-1,33);//一个黑色的小box
              puts8(desktop,bootp->xsize,32,16,7,s,Font8x16);     //display e0
              sheet_refresh(sht_back,32,16,32+20*8-1,31);
        #define white 7
               //because we use sheet layer ,so we do not need this any more
              //boxfill8(p,320,white,mx,my,mx+15,my+15);//用背景色把鼠标原来的color填充，这样不会看到鼠标移动的path
              mx +=mdec.x;//mx=mx+dx
              my +=mdec.y;//my=my+dy
              if(mx<0)
              {
                mx=0;
              }
              if(my<0)
              {
                my=0;
              }


              if(mx>bootp->xsize-1)
              {
                mx=bootp->xsize-1;
              }

              if(my>bootp->ysize-1)
              {
                my=bootp->ysize-1;
              }
              sprintf(s,"(%d, %d)",mx,my);
              boxfill8(desktop,320,0,0,0,79,15);//坐标的背景色
              puts8(desktop ,bootp->xsize,0,0,7,s,Font8x16);//显示坐标
              sheet_refresh(sht_back,0,0,bootp->xsize,15);
              sheet_move(sht_mouse,mx,my);



        }
      }//end of mouse decoder
      else if(fifo8_status(&timerfifo)!=0)
      {
        data=fifo8_read(&timerfifo);
        sti();
        puts8(desktop ,bootp->xsize,0,64,0,"10[sec]",Font8x16);//显示坐标
        sheet_refresh(sht_back,0,64,bootp->xsize,80);
        //printdebug(99,80);


      }//end of timer do sth
      else if(fifo8_status(&timerfifo2)!=0)
      {
        data=fifo8_read(&timerfifo2);
        sti();
        puts8(desktop ,bootp->xsize,0,80,0,"3[sec]",Font8x16);//显示坐标
        sheet_refresh(sht_back,0,80,bootp->xsize,96);
       // printdebug(16,150);
     // while(1);
      }
      else if(fifo8_status(&timerfifo3)!=0)//cursor blink
      {
        timer_settime(timer3,50);
        data=fifo8_read(&timerfifo3);
        sti();
        //static unsigned a=0;a++;//因为你没有把里面的数据读走，所以一直会进来
        //printdebug(a,0);
            if(data!=0)
            {
                timer_init(timer3,&timerfifo3,0);
                boxfill8(desktop,bootp->xsize,7,55,18,61,20);
            }
            else
            {
                timer_init(timer3,&timerfifo3,1);
                boxfill8(desktop,bootp->xsize,0,55,18,61,20);
            }
            timer_settime(timer3,50);
            sheet_refresh(sht_back,55,18,61,20);


      }

   }

 }
}













