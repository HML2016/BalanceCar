#include "include.h"
uint8 flag_100ms,flag_record;  //������ѭ����ִ��

void PIT_IRQHandler(void);

void main (void)
{
  DisableInterrupts;  //�ر��ж� 
  All_Init();   
  set_vector_handler(PIT_VECTORn ,PIT_IRQHandler);
  set_vector_handler(UART0_VECTORn ,UART0_RX_IRQHandler); 
  enable_irq (PIT_IRQn);    
  NVIC_SetPriority(UART0_IRQn,1);
  NVIC_SetPriority(PIT_IRQn,2);
  EnableInterrupts; //���ж�
  while(1)
  {
    if(SendPara)
    {
      Send_Para();
      SendPara=0;
    }
    if(!SaveData&&Uart_Send) //��������ʱ���ܷ�����
    {
      if(SendCCD)
      {
       Send_CCD();
       SendCCD=0;
      }
       Send_Variable();     
    } 
    if(SendSD&&Stop)
    {
      SendSD=0;
      Send_SD();
    } 

    if(flag_100ms)        
    {
       Read_Switch();
       if(OLED_Refresh)
       { 
        Check_BottonPress();  
       if(Stop==true||SaveData==false)  OLED_Draw_UI();
       }
       else Check_StartPress();    
       flag_100ms=0;
    }        
    if(flag_record&&SaveData&&SD_OK) 
   {
      if(Stop!=true)             //���С���������У����¼
     {
       if (Starting) Block_Index=1;
       else Record();                     //�洢����  
     }
     else if(CarStopedJustNow) //С����ֹͣ
    {
        CarStopedJustNow=false; 
        Write_Information();
    }
      flag_record=false;
   }
  } 
}
void PIT_IRQHandler(void)
{
  if(PIT_TFLG(PIT0) == 1 )  
  {
      static uint8  part=0; 
      static uint8  cnt100ms;
      //�������
      static uint8 light_tower_cnt;
      static uint8 light_tower_delay;
      static int pulse_cnt;
      static uint8 light_tower_on=0;
      
      part++;
      cnt100ms++;
      SpeedCount++;    
      //ѭ����������
       switch(part) 
     {
        case 1: 
               //ֱ������
                Get_Attitude(); 
                Angle_Calculate();
                Angle_Control();
                //�ٶȿ���  
                Get_Speed();   //5ms�ɼ�һ���ٶ�
                SpeedCount++;  
                if(SpeedCount>= Speed_Filter_Times) 
               {                 
                   Speed_Control();
                   SpeedCount=0;
               } 
               Speed_Control_Output(); 
                break; 
        case 2: //CCD�ɼ� 
                CCD_Capture(); //�ɼ�CCD 
                break;
        case 3: 
               CCD_Normalization();
               read_buff[0]= Get_Y_Gyro();
                break;  
        case 4: 
                RoadType_Distinguish();  
                               
               if((RunTime<0.1)&&(Stop==0))  //��ֹ�𲽵�ʱ����Ʈ
              {
                Rightlastfind=0;
                Leftlastfind=0;
                Middle_Err=0;
              }
                Direction_Control(); 
                DirectionCount=0;     
                flag_record=1;                    //����¼��־λ
                SendCCD=1; 
                break;
        case 5:
                part=0; 
                read_buff[1]= Get_Y_Gyro();
                
                break;
        default:
                break;   
     }
      DirectionCount++;
      Direction_Control_Output();
      Moto_Out();          
    ///////////////////////////////////////////////////��������

    if(Stop)   
    {
        light_tower_delay++;
         if(light_tower_on&&light_tower_delay==100) 
         {
            light_tower_delay=0;
             pulse_cnt=lptmr_pulse_get();
             if(pulse_cnt<10)
             {
               if(Light_Tower_Index!=0) Light_Tower_OFF;
               light_tower_on=0;
             }
             lptmr_pulse_clean();
         }
          if(light_tower_on==0&&light_tower_delay==10)  //�ضϵ���10ms���ٴ�
          {
            light_tower_delay=0;
            Light_Tower_ON;
            light_tower_on=1;
            lptmr_pulse_clean();
          }
     }  
     ///////////////////////////////////////////////LED״ָ̬ʾ 
      if(cnt100ms>=100)
     {
        if(Stop)
        {
          LED_BLUE_TURN;
          LED_RED_OFF;
          LED_GREEN_OFF;
        }
        else
        {
         if(Starting)
         {
          LED_GREEN_TURN;
          LED_RED_OFF;
          LED_BLUE_OFF;
         }
         else
         {
          LED_RED_TURN;
          LED_GREEN_OFF;
          LED_BLUE_OFF;
         }
        }
        cnt100ms=0;
        flag_100ms=1;
     }
     ////////////////////////////////////////////����
     if(Starting) 
     {  
        if(Light_Tower_Index==0)        //�����õ�����⣬����ʱ����
        {
          Start_Cnt--;
          if(Start_Cnt==0)
          {
           Starting=false;
          }
        }
        else                                      //��������
        {
          light_tower_delay++;
          if(light_tower_detected==0)
         {
            if(light_tower_delay==200)                   
            { 
              pulse_cnt=lptmr_pulse_get();    
              if(pulse_cnt>150) 
              {
               light_tower_detected=1;
               light_tower_delay=0;
               Light_Tower_ON;
              }
              else   Light_Tower_OFF;
              lptmr_pulse_clean();
            }
            if(light_tower_delay>=210)
            {
              light_tower_delay=0;
              Light_Tower_ON;
            }
          }
         else
         {
            if(light_tower_delay==10)                   //����ʱ��������ܽ����Խ��յ�������������һ���ϵ�����Ϊ����������
            { 
              pulse_cnt=lptmr_pulse_get();   
              if(pulse_cnt<5) 
              {
                Starting=false; 
                light_tower_cnt=0;
                light_tower_on=0;
              }
              light_tower_delay=0;
              lptmr_pulse_clean();
            }
          }
        }
      }
     ///////////////////////////////////////////////////ͣ��
      if(((RunTime>28&&Light_Tower_Index==1)||(Light_Tower_Index==3))&&Stop==0&&Starting==0)  //����ͣ��
     { 
       light_tower_delay++;
        if(light_tower_on&&light_tower_delay==50) //50ms ɨ��
        {
          light_tower_delay=0;
          pulse_cnt=lptmr_pulse_get();
          if(pulse_cnt>30)            //˵������ǿ�ź���
          {
            light_tower_cnt++;
          }
          else
          {
            if(light_tower_cnt>=2)
            {
                ForceStop=true;//��ֹ���ŵĲ���
            }
            light_tower_cnt=0;
          }
          Light_Tower_OFF;
          light_tower_on=0;
          lptmr_pulse_clean();
        }
        if(light_tower_on==0&&light_tower_delay==10)  //�ضϵ���10ms���ٴ�
        {
          light_tower_delay=0;
          Light_Tower_ON;
          light_tower_on=1;
          lptmr_pulse_clean();
        }
      }  
     /////////////////////////////////////////////ͳ��ʱ�估�ٶ�
     if(Stop==false&&Starting==false)
     {
      RunTime=RunTime+0.001;
      AverageSpeed=Distance/RunTime;//�������ʱ��
     }
  } 
   PIT_Flag_Clear(PIT0);       //���жϱ�־λ
}