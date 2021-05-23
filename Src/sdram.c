#include "sdram.h"
#include "sys.h"
#include "fmc.h"

//����SDRAM��ʼ������
void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef* hsdram)
{
    uint32_t temp = 0;
    //SDRAM��������ʼ������Ժ���Ҫ��������˳���ʼ��SDRAM
    SDRAM_Send_Cmd(0, FMC_SDRAM_CMD_CLK_ENABLE, 1, 0); //ʱ������ʹ��
    HAL_Delay(100);                                  //������ʱ200us
    SDRAM_Send_Cmd(0, FMC_SDRAM_CMD_PALL, 1, 0);       //�����д洢��Ԥ���
    SDRAM_Send_Cmd(0, FMC_SDRAM_CMD_AUTOREFRESH_MODE, 8, 0);//������ˢ�´��� 
    //����ģʽ�Ĵ���,SDRAM��bit0~bit2Ϊָ��ͻ�����ʵĳ��ȣ�
      //bit3Ϊָ��ͻ�����ʵ����ͣ�bit4~bit6ΪCASֵ��bit7��bit8Ϊ����ģʽ
      //bit9Ϊָ����дͻ��ģʽ��bit10��bit11λ����λ
    temp = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1 |	//����ͻ������:1(������1/2/4/8)
        SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL |	//����ͻ������:����(����������/����)
        SDRAM_MODEREG_CAS_LATENCY_2 |	//����CASֵ:3(������2/3)
        SDRAM_MODEREG_OPERATING_MODE_STANDARD |   //���ò���ģʽ:0,��׼ģʽ
        SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;     //����ͻ��дģʽ:1,�������
    SDRAM_Send_Cmd(0, FMC_SDRAM_CMD_LOAD_MODE, 1, temp);   //����SDRAM��ģʽ�Ĵ���

    //ˢ��Ƶ�ʼ�����(��SDCLKƵ�ʼ���),���㷽��:
      //COUNT=SDRAMˢ������/����-20=SDRAMˢ������(us)*SDCLKƵ��(Mhz)/����
    //����ʹ�õ�SDRAMˢ������Ϊ64ms,SDCLK=64/2=32,����Ϊ8192(2^13).
      //����,COUNT=64*1000*32/8192-20=230
    HAL_SDRAM_ProgramRefreshRate(hsdram, 230);

}


//��SDRAM��������
//bankx:0,��BANK5�����SDRAM����ָ��
//      1,��BANK6�����SDRAM����ָ��
//cmd:ָ��(0,����ģʽ/1,ʱ������ʹ��/2,Ԥ������д洢��/3,�Զ�ˢ��/4,����ģʽ�Ĵ���/5,��ˢ��/6,����)
//refresh:��ˢ�´���
//regval:ģʽ�Ĵ����Ķ���
//����ֵ:0,����;1,ʧ��.
uint8_t SDRAM_Send_Cmd(uint8_t bankx, uint8_t cmd, uint8_t refresh, uint16_t regval)
{
    uint32_t target_bank = 0;
    FMC_SDRAM_CommandTypeDef Command;

    if (bankx == 0) target_bank = FMC_SDRAM_CMD_TARGET_BANK1;
    else if (bankx == 1) target_bank = FMC_SDRAM_CMD_TARGET_BANK2;
    Command.CommandMode = cmd;                //����
    Command.CommandTarget = target_bank;      //Ŀ��SDRAM�洢����
    Command.AutoRefreshNumber = refresh;      //��ˢ�´���
    Command.ModeRegisterDefinition = regval;  //Ҫд��ģʽ�Ĵ�����ֵ
    if (HAL_SDRAM_SendCommand(&hsdram2, &Command, 0X1000) == HAL_OK) //��SDRAM��������
    {
        return 0;
    }
    else return 1;
}

//��ָ����ַ(WriteAddr+Bank5_SDRAM_ADDR)��ʼ,����д��n���ֽ�.
//pBuffer:�ֽ�ָ��
//WriteAddr:Ҫд��ĵ�ַ
//n:Ҫд����ֽ���
void FMC_SDRAM_WriteBuffer(uint8_t* pBuffer, uint32_t WriteAddr, uint32_t n)
{
    for (; n != 0; n--)
    {
        *(volatile uint8_t*)(Bank5_SDRAM_ADDR + WriteAddr) = *pBuffer;
        WriteAddr++;
        pBuffer++;
    }
}

//��ָ����ַ((WriteAddr+Bank5_SDRAM_ADDR))��ʼ,��������n���ֽ�.
//pBuffer:�ֽ�ָ��
//ReadAddr:Ҫ��������ʼ��ַ
//n:Ҫд����ֽ���
void FMC_SDRAM_ReadBuffer(uint8_t* pBuffer, uint32_t ReadAddr, uint32_t n)
{
    for (; n != 0; n--)
    {
        *pBuffer++ = *(volatile uint8_t*)(Bank5_SDRAM_ADDR + ReadAddr);
        ReadAddr++;
    }
}

