#include "include.h"


#define  SD_CS PTE4_OUT

uint8   SD_Type = 0;
void   SD_DisSelect();
uint8   SD_GetResponse(uint8 Response);
uint8   SD_RecvData(uint8*buf,uint16 len);
uint8   SD_Select();

uint8   SD_WaitReady();
uint8  SPI_SendReceiveData(uint8 TxData);



void SD_SPI_HighSpeed()
{
  spi_set_baud (SPI1,1000*1000);	
}

void SD_DisSelect()
{
  SD_CS=1; //ȡ��Ƭѡ
  SPI_SendReceiveData(0xff);   //�ṩ�����8��ʱ��
}
uint8 SD_WaitReady()
{
  uint32 t = 0;
  do
  {
    if(SPI_SendReceiveData(0xff) == 0xff) return 0;
    t++;
  }while(t < 0xffffff);
  return 1;
}

uint8 SD_GetResponse(uint8 Response)
{
  uint16 Count=0xFFFF;//�ȴ�����	   						  
  while((SPI_SendReceiveData(0XFF)!=Response)&&Count) Count--;//�ȴ��õ�׼ȷ�Ļ�Ӧ  	  
  if(Count==0)return MSD_RESPONSE_FAILURE;//�õ���Ӧʧ��   
  else return MSD_RESPONSE_NO_ERROR;//��ȷ��Ӧ	
}
//��sd����ȡһ�����ݰ�������
//buf:���ݻ�����
//len:Ҫ��ȡ�����ݳ���.
//����ֵ:0,�ɹ�;����,ʧ��;	
uint8 SD_RecvData(uint8*buf,uint16 len)
{			  	  
   if(SD_GetResponse(0xFE)) return 1;//�ȴ�SD������������ʼ����0xFE
    while(len--)//��ʼ��������
    {
      *buf=SPI_SendReceiveData(0xFF);
       buf++;
    }
    //������2��αCRC��dummy CRC��
    SPI_SendReceiveData(0xFF);
    SPI_SendReceiveData(0xFF);									  					    
    return 0;//��ȡ�ɹ�
}
//��SD������һ������
//����: uint8 cmd   ���� 
//      u32 arg  �������
//      uint8 crc   crcУ��ֵ	   
//����ֵ:SD�����ص���Ӧ															  
uint8 SD_SendCmd(uint8 cmd, uint32 arg, uint8 crc) 
{
    uint8 r1;	
    uint8 Retry=0;
    SD_DisSelect();//ȡ��Ƭѡ
    SD_CS=0; //Ƭѡ
    SPI_SendReceiveData(cmd | 0x40);//�ֱ�д������
    SPI_SendReceiveData(arg >> 24);
    SPI_SendReceiveData(arg >> 16);
    SPI_SendReceiveData(arg >> 8);
    SPI_SendReceiveData(arg);	  
    SPI_SendReceiveData(crc); 
    Retry=0X1F;
    do
    {
     r1=SPI_SendReceiveData(0xFF);
    }
    while((r1&0X80)&&Retry--);
    //����״ֵ̬;
    return r1;
}
uint8 SD_Initialize(void)
{
    uint8 r1;           //���SD���ķ���ֵ
    uint16 retry;       //�������г�ʱ����
    uint16 i;
    uint8 buff[4];
    uint8 success=0;
    
    SD_DisSelect();
    for(i=0;i<10;i++)SPI_SendReceiveData(0XFF);//��������74������
    retry=20;
    do
    {
       r1=SD_SendCmd(CMD0,0,0x95);//����IDLE״̬
       SD_DisSelect();            //ȡ��Ƭѡ
    }
    while((r1!=0X01)&&retry--);
    SD_Type=0;//Ĭ���޿�
    if(r1==0X01)
    {
      if(SD_SendCmd(CMD8,0x1AA,0x87)==1)//SD V2.0
      {
        retry=0XFFFE;
        do
        {
            SD_SendCmd(CMD55,0,1);	       //����CMD55
            r1=SD_SendCmd(CMD41,0x40000000,1);//����CMD41
        }while(r1&&retry--);
        if(r1==0)success=1;  //��ʼ���ɹ���
        r1 = SD_SendCmd(CMD58, 0, 1);
        if(r1==0)
        {
          buff[0] =SPI_SendReceiveData(0xFF);
          buff[1] =SPI_SendReceiveData(0xFF);
          buff[2] =SPI_SendReceiveData(0xFF);
          buff[3] =SPI_SendReceiveData(0xFF); 
          SD_DisSelect();//ȡ��Ƭѡ
          if(buff[0]&0x40)SD_Type = SD_TYPE_V2HC; //���CCS
          else SD_Type = SD_TYPE_V2;
        } 
      }
    }
    SD_DisSelect();
    SD_SPI_HighSpeed();//����	
    return success;//��������
}
//��SD��
//buf:���ݻ�����
//sector:����
//cnt:������
//����ֵ:0,ok;����,ʧ��.
uint8 SD_ReadDisk(uint8*buf,uint32 sector,uint8 cnt)
{
  uint8 r1;
  if(SD_Type!=SD_TYPE_V2HC)sector<<= 9;//ת��Ϊ�ֽڵ�ַ
  if(cnt==1)
  {
    r1=SD_SendCmd(CMD17,sector,0X01);//������
    if(r1==0)//ָ��ͳɹ�
    {
      r1=SD_RecvData(buf,512);//����512���ֽ�	   
    }
  }
  else
  {
    r1=SD_SendCmd(CMD18,sector,0X01);//����������
    do
    {
      r1=SD_RecvData(buf,512);//����512���ֽ�	 
      buf+=512;  
    }while(--cnt && r1==0); 	
    SD_SendCmd(CMD12,0,0X01);	//����ֹͣ����
  }   
  SD_DisSelect();//ȡ��Ƭѡ
  return r1;
}
uint8 SPI_SendReceiveData(uint8 TxData)
{
  uint16 retry=0;
  SPI_MemMapPtr spi_ptr=SPI1_BASE_PTR;
  
  while(!(SPI_S_REG(spi_ptr)&SPI_S_SPTEF_MASK)) //���ͼĴ����ǿյȴ�
  {
    retry++;
    if(retry>200)return 0;
  }
  SPI_DL_REG(SPI1_BASE_PTR)=TxData;                     //ֱ�Ӷ�ȡ��������ս��ջ�����

  retry=0;
 
  while(!(SPI_S_REG(spi_ptr)& SPI_S_SPRF_MASK))  //�����ջ������ǿ�
  {
    retry++;
    if(retry>2000)return 0;
  } 	                                    //ֱ�Ӷ�ȡ��������ս��ջ�����
 return   SPI_DL_REG(SPI1_BASE_PTR);		
}

      