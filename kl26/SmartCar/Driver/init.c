#include "include.h"
void Para_Init();
void GPIO_Init();
void Fix_Offset();
void All_Init()
{   
   GPIO_Init(); 
   I2C_Init(); //I2C�����Լ���������ʼ��
   Read_Switch();
   tpm_pwm_init(TPM0,TPM_CH1,10000,0);  //PWM��ʼ�� 
   tpm_pwm_init(TPM0,TPM_CH2,10000,0); 
   tpm_pwm_init(TPM0,TPM_CH4,10000,0); 
   tpm_pwm_init(TPM0,TPM_CH5,10000,0); 
   adc_init(ADC0_SE8); //ccd
   adc_init(ADC0_SE14);//��ѹ�ɼ�
   pit_init_ms(PIT0,1); //1ms��ʱ�ж�
   Para_Init();         //������ʼ��
   tpm_pulse_init(TPM1,TPM_CLKIN0,TPM_PS_1);  //�����̼���
   tpm_pulse_init(TPM2,TPM_CLKIN1,TPM_PS_1);  //�����̼���
   uart_init(UART0,115200); 
   uart_rx_irq_en(UART0);//ʹ�ܴ��ڽ����ж� 
   OLED_Init();
   Fix_Offset();
   EEPROM_init();
   spi_init(SPI1,SPI_PCS0,MASTER,200*1000);   //��ʼ��Ϊ10k������
   if(SD_Initialize())
   {
     SD_OK=1; 
   }
}

//������ֵ��ʼ��
void Para_Init()
{
 PID_ANGLE.P=0.10;
 PID_ANGLE.D=0.01;
 PID_SPEED.P=1.5;
 PID_SPEED.I=0.05;
 PID_TURN.P=0.01;
 PID_TURN.D=0.005; 
 Fuzzy_Kp=0.005;
 Fuzzy_Kd=0.0005;
 SetSpeed=3.8;
 Set_Angle=50;
 Threshold=30;
 Acc_Offset=230;
 Hill_Slow_Ratio=0.6;
 Strong_Turn_Angle=40;
 Strong_Turn_Speed=60;
 CCD_Offset=200;
}
//GPIO��ʼ��
void GPIO_Init()
{
  //����
  lptmr_pulse_init(LPT0_ALT2,0xFFFF,LPT_Rising);
  gpio_init(PTB10,GPO,0); 
  //���뿪��
  gpio_init(PTB3,GPI,0);                   
  port_init_NoALT(PTB3,PULLUP);
  gpio_init(PTB7,GPI,0);                     
  port_init_NoALT(PTB7,PULLUP);
  gpio_init(PTB8,GPI,0);                     
  port_init_NoALT(PTB8,PULLUP);
  gpio_init(PTB9,GPI,0);                     
  port_init_NoALT(PTB9,PULLUP);
  //��������
  gpio_init(PTA4,GPI,0);                    
  port_init_NoALT(PTA4,PULLUP); 
  gpio_init(PTA5,GPI,0);                     
  port_init_NoALT(PTA5,PULLUP);
  gpio_init(PTA6,GPI,0);                     
  port_init_NoALT(PTA6,PULLUP);
  gpio_init(PTA7,GPI,0);                     
  port_init_NoALT(PTA7,PULLUP);
  gpio_init(PTA12,GPI,0);                     
  port_init_NoALT(PTA12,PULLUP);
  gpio_init(PTA13,GPI,0);                     
  port_init_NoALT(PTA13,PULLUP);
  //LED��ʼ��&��չ�ӿڳ�ʼ��
  gpio_init(PTE26,GPO,0);  //extern
  gpio_init(PTE29,GPO,0);
  gpio_init(PTE30,GPO,0); 
  gpio_init(PTE31,GPO,0); 
  //CCD�ܽ�
  gpio_init(PTB1,GPO,0); 
  gpio_init(PTB2,GPO,0); 
  //OLED�ܽ�
  gpio_init(PTA14,GPO,0); 
  gpio_init(PTA15,GPO,0);
  gpio_init(PTA16,GPO,0);
  gpio_init(PTA17,GPO,0);
}
//����������ƫ��
void Fix_Offset()
{
  int Offset_X_Sum=0,Offset_Y_Sum=0;  //ƫ���ۻ�
  int i; 
  LED_GREEN_ON; 
  LED_RED_ON;
  LED_BLUE_ON;
  DELAY_MS(1000); 
  for(i=0;i<20;i++)
  {
   Offset_X_Sum+= Get_X_Gyro();
   DELAY_MS(5); 
   Offset_Y_Sum+= Get_Y_Gyro();
   DELAY_MS(5);
  } 
  Gyro_X_Offset=(int)(Offset_X_Sum/20.0);
  Gyro_Y_Offset=(int)(Offset_Y_Sum/20.0);
}
