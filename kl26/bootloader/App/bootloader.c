#include "include.h"
#define APP_ADDRESS 0x00005000



uint32  APP_Sector = 0;
uint16  APP_Byte = 0;
FATFS  FS_Struct;	                   //����ע��
FIL    FP_Struct;			               //���ڴ��ļ�
uint8   gCheckFat = 0;	                 //�����жϣ��ļ������Ƿ���ȷ
UINT  ReadNum;
uint8   Percentage=0;
int  i = 0;
int  j=0;
  
uint8   ReadAppBuffer[1024];

void Jump_to_App()
{
  uint32_t *APP_VECTOR_TABLE=(uint32_t *)(APP_ADDRESS);
  uint32_t appEntry = APP_VECTOR_TABLE[1];
  uint32_t appStack = APP_VECTOR_TABLE[0];
  static void (*firmwareBootloader)(void) = 0;
  
  
__enable_interrupt();

  
  firmwareBootloader = (void (*)(void))appEntry;
  // Set the VTOR to the application vector table address.
  SCB->VTOR = (uint32_t)APP_VECTOR_TABLE;
  //Set stack pointers to the application stack pointer.
  __set_MSP(appStack);
  __set_PSP(appStack);
  // Jump to the application.
  firmwareBootloader();
}								    //��ת��APP.

void led_flash()
{
  uint8 i=0;
  for(i=0;i<10;i++)
  {
   gpio_set(PTE29,1);
   gpio_set(PTE30,0);
   gpio_set(PTE31,0);
   DELAY_MS(30); 
   gpio_set(PTE29,0);
   gpio_set(PTE30,1);
   gpio_set(PTE31,0);
   DELAY_MS(30); 
   gpio_set(PTE29,0);
   gpio_set(PTE30,0);
   gpio_set(PTE31,1);
   DELAY_MS(30); 
  }
   gpio_set(PTE29,0);
   gpio_set(PTE30,0);
   gpio_set(PTE31,0);
   DELAY_MS(200); 
}

void led_error()
{
  uint8 i=0;
  for(i=0;i<4;i++)
  {
   gpio_set(PTE29,1);
   gpio_set(PTE30,1);
   gpio_set(PTE31,1);
   DELAY_MS(200); 
   gpio_set(PTE29,0);
   gpio_set(PTE30,0);
   gpio_set(PTE31,0);
   DELAY_MS(200); 
  }
}

void main (void)
{
  //�������� OK��
  gpio_init(PTA6,GPI,0);                    
  port_init_NoALT(PTA6,PULLUP); 
  //  ��ⰴ���Ƿ��� yes��
  DELAY_MS(50);
  if(gpio_get(PTA6)==1)   //���������OK�����žʹ�SD�����³���
  {
    Jump_to_App();   //����ֱ����ת������
  }
   //LED��
  gpio_init(PTE29,GPO,0);
  gpio_init(PTE30,GPO,1); 
  gpio_init(PTE31,GPO,0); 
  while(gpio_get(PTA6)==0);
  //OLED�ܽ�
  gpio_init(PTA14,GPO,0); 
  gpio_init(PTA15,GPO,0);
  gpio_init(PTA16,GPO,0);
  gpio_init(PTA17,GPO,0);
  spi_init(SPI1,SPI_PCS0,MASTER,200*1000);   //��ʼ��Ϊ10k������
  OLED_Init();   
  OLED_P6x8Str(0,0,"BootLoader Entered!");
  led_flash();  //���������bootloader������
  flash_init();
  gCheckFat=SD_Initialize();
  if(gCheckFat)   //���SD����ʼ���ɹ���
  {
    OLED_P6x8Str(0,1,"SD Detect Succeed!");
    gCheckFat = f_mount (0,&FS_Struct); 
    if(gCheckFat == 0)
    {
      gCheckFat = f_open(&FP_Struct,"app.bin",FA_READ);	
      if(gCheckFat == 0)
      {
        OLED_P6x8Str(0,2,"Binary file find");
        OLED_P6x8Str(0,3,"Size of file=");
        {
            OLED_P6x8Char(0x30+FP_Struct.fsize/10000);
            OLED_P6x8Char(0x30+FP_Struct.fsize%10000/1000);
            OLED_P6x8Char(0x30+FP_Struct.fsize%1000/100);
            OLED_P6x8Char(0x30+FP_Struct.fsize%100/10);
            OLED_P6x8Char(0x30+FP_Struct.fsize%10);
            OLED_P6x8Char(' ');
            OLED_P6x8Char('K');
            OLED_P6x8Char('B');               
        }
        OLED_P6x8Str(0,4,"Programing:");
        DELAY_MS(500);
        APP_Sector = FP_Struct.fsize / 1024;
        APP_Byte   = FP_Struct.fsize % 1024;
        for(i = 0;i < APP_Sector;i++)
        {
          gpio_turn(PTE31);   //״ָ̬ʾ
          flash_erase_sector(20+i);//�Ȳ�������  
          
          if(f_read (&FP_Struct,ReadAppBuffer,1024,&ReadNum)!=FR_OK)
          {
             OLED_P6x8Str(0,4,"File read error");
             led_error();
             while(1);            //��ȡ����
          }
          else flash_write_buf(20+i,0,1024,ReadAppBuffer);        //20+i,ƫ�Ƶ�ַΪ0,д������Դ��ַ:buff,��Ŀsizeof(buff) 0x5000/1024=20
        
          while(1)
          {
            for(j=0;j<1024;j++)
            {
              if(flash_read(20+i,j,uint8)!=*(ReadAppBuffer+j))  //�鿴�����Ƿ����
              {
                flash_erase_sector(20+i);
                flash_write_buf(20+i,0,1024,ReadAppBuffer);     //��д
                break;
              }
            }
            if(j==1024) break;  //У�����
          }
          
          Percentage=i*100/APP_Sector;
          OLED_PrintValueI(70,4,Percentage); //��ʾ
          OLED_P6x8Char(' ');
          OLED_P6x8Char('%');
        }
        if(APP_Byte != 0)
        {
          flash_erase_sector(20+APP_Sector);          
           if(f_read(&FP_Struct,ReadAppBuffer,APP_Byte,&ReadNum)!=FR_OK)
          {
             OLED_P6x8Str(0,4,"File read error");
             led_error();
             while(1);            //��ȡ����
          }
          else flash_write_buf(20+APP_Sector,0,APP_Byte,ReadAppBuffer);        //20+APP_Sector,д������Դ��ַ:buff,��Ŀsizeof(buff) 
         
          while(1)             //У��
          {
            for(j=0;j<APP_Byte;j++)
            {
              if(flash_read(20+APP_Sector,j,uint8)!=*(ReadAppBuffer+j))  //�鿴�����Ƿ����
              {
                flash_erase_sector(20+APP_Sector);
                flash_write_buf(20+APP_Sector,0,1024,ReadAppBuffer);     //��д
                break;
              }
            }
            if(j==APP_Byte) 
            {
              Percentage=100;
              OLED_PrintValueI(70,4,Percentage); //��ʾ
              OLED_P6x8Char(' ');
              OLED_P6x8Char('%');
              break;  //У�����
            }
          }
          
        }
         OLED_P6x8Str(0,5,"Program Succeed");
         OLED_P6x8Str(0,6,"Jumping to app");
         led_flash();
         f_close (&FP_Struct);
         NVIC_SystemReset();  //��λϵͳ
      }
      else
      { 
        OLED_P6x8Str(0,2,"Can't find file");
        led_error();
      }
    }
  }
  else 
  {  
    OLED_P6x8Str(0,1,"SD Detect Failed!");
    led_error();
  }
  
  
  Jump_to_App();   //��ת������
  
  while(1)
  {
    
  }
}