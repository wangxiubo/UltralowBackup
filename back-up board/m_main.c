/************************************************************************************************************************************
overview:  1����ѭ��
           2��ʱ��Ƭ��
*************************************************************************************************************************************/
#include "r_cg_macrodriver.h"
#include "r_cg_timer.h"
#include "r_cg_serial.h"
#include "r_cg_adc.h"
#include "r_cg_pclbuz.h"

#include "h_type_define.h"
#include "m_com.h"
#include "m_test.h"
#include "m_ad.h"
#include "m_e2.h"
#include "m_peripheral_control.h"
#include "m_main.h"

//��������
void system_init(void);  
void system_start(void);
void main_loop(void);    
void timer_op(void); 
void ad_convert_deal();
void timer_int(void);     

//��������
flag_type flg_time;
//----------------------------------------------------------
uint8_t   guc_5ms_timer;      //5�����ʱ��
uint8_t   guc_10ms_timer;     //10�����ʱ��
uint8_t   guc_100ms_timer;    //100ms��ʱ��
uint8_t   guc_1s_timer;       //1s��ʱ��
uint8_t   guc_1min_timer;     //1min��ʱ��



/*************************************************************************************************************************************************************
�������ܣ���ʼ�ϵ�ʱ��ʼ��

����λ�ã�ϵͳ��ʼ����ѭ����----------------------------------ok
**************************************************************************************************************************************************************/
void system_init(void)   
{
    R_ADC_Set_OperationOn(); //adת������
    com_init();    

    //--------------------------------------------------
    init_ram_para2();        //peak ��дE2���°���Ҫ��дһ�Σ�
    init_ram_para();

    eeprom2_read_deal();    //eeprom2��ȡ��������
    eeprom_read_deal();     //eeprom��ȡ��������
    
    
    guc_100ms_timer = 100;
    guc_1s_timer = 100;
    guc_1min_timer = 60;

    bflg_key_buzz_delaytime = 1;
    gss_key_buzz_delaytimer = 0;
    R_PCLBUZ0_Start();

    TEST_BLUE_OUT_PIN = 1;   //��װ����ʱ��
}
/*************************************************************************************************************************************************************
�������ܣ���ʼ�ϵ�ʱ��ʼ��

����λ�ã�ϵͳ��ʼ����ѭ����----------------------------------ok
**************************************************************************************************************************************************************/
void system_start(void)  //ϵͳ��������
{
    R_TAU0_Channel7_Start();   //��ʱ��Ƭ��ʱ��
    R_ADC_Start();             //ADת������
}

/*************************************************************************************************************************************************************
�������ܣ���Ҫ������ѭ���еĺ���

����λ�ã�ϵͳ��ѭ��----------------------------------ok
**************************************************************************************************************************************************************/
void main_loop(void)    
{
    timer_op();
    
    if (bflg_allow_ad_calc == 1)   //�������ad����
    {
        bflg_allow_ad_calc = 0;
        ad_temp_calc();            //ad�¶ȼ������
    }
    
    if(bflg_test_mode == 1)   
    {
        test_mode_com();
        test_in_out_pin();
        test_error_code_deal();
    }
    else
    {
        if (bflg_com_allow_rx == 1)  //�����������
        {
            bflg_com_allow_rx = 0;
            //------------------------------
            com_rx_init();   
            COM_RX_MODE;
            R_UART0_Start();
        }
        //----------------------------------
        if (bflg_com_rx_end == 1)    //������ս���
        {
            bflg_com_rx_end = 0;
            //------------------------------
            R_UART0_Stop();
        }
        //----------------------------------
        if (bflg_com_rx_ok == 1)     //������ճɹ�
        {
            bflg_com_rx_ok = 0;
            //------------------------------
            R_UART0_Stop();
            com_rx_data_deal();   
        }
        //----------------------------------
        if (bflg_com_allow_tx == 1)  //�����������
        {
            bflg_com_allow_tx = 0;
            //------------------------------
            R_UART0_Start();
            COM_TX_MODE;
            com_tx_init();   
        }
        if (bflg_com_tx_ok == 1)     //������ͳɹ�
        {
            bflg_com_tx_ok = 0;       
            //------------------------------
            R_UART0_Stop();
        }
    }
}
/*************************************************************************************************************************************************************
�������ܣ�����ʱ��Ƭ��

����λ�ã���ѭ��----------------------------------ok
**************************************************************************************************************************************************************/
void timer_op(void)  
{
    if (bflg_1ms_reach == 1)  //1ms
    {
        bflg_1ms_reach = 0;
        guc_100ms_timer--;
        
        ad_convert_deal();
        com_rx_delaytime();
        com_tx_delaytime();
        com_rx_end_delaytime();
    }
    if (bflg_5ms_reach == 1)  //5ms
    {
        bflg_5ms_reach = 0;

    }
    if (bflg_10ms_reach == 1)  //10ms
    {
        bflg_10ms_reach = 0;
        guc_1s_timer--;

        key_buzz_delaytime();
    }
    if (guc_100ms_timer == 0) //100ms
    {
        guc_100ms_timer = 100;

    }
    if (guc_1s_timer == 0)    //1s
    {
        guc_1s_timer = 100;
        guc_1min_timer--;
        
        test_mode_LED_deal();
    }
    if (guc_1min_timer == 0)  //1min
    {
        guc_1min_timer = 60;

        
    }
}
/*************************************************************************************************************************************************************
��������: ��ȡÿ��ͨ��ad�ɼ���ֵ

���ʽ���: ADCR: ת������Ĵ���
          ADCR  �Ĵ����� A/D ת�������������� 10 λ���� 6 λ�̶�Ϊ 0��;
                ��Ϊ�õ���10λadת����
          ADS: ģ������ͨ��ѡ��Ĵ���   
����λ��: 1ms��ʱ��---------------------------------------ok        
**************************************************************************************************************************************************************/
void ad_convert_deal(void)    //adת������������1ms��ʱ�����е���
{
    gus_ad_val = (uint16_t)(ADCR >> 6U);
    //------------------------------------------------------
    ad_val_deal();  //adֵ��������
    //------------------------------------------------------
    ADS = (uint8_t)(guc_ad_index + 2); //adͨ��ѡ��peak��ΪӲ���ӵ�2ͨ����ʼ��
    //------------------------------------------------------
    R_ADC_Start();  //����adת�� peak ÿ�ζ�Ҫ��ʼ����ÿ�ı�һ��ͨ����Ҫ��ʼ��
}

/*************************************************************************************************************************************************************
�������ܣ�ʱ��Ƭ��ʱ

����λ�ã�1ms��ʱ�ж�----------------------------------ok
**************************************************************************************************************************************************************/
void timer_int(void)     
{
    bflg_1ms_reach = 1;       //��1ms����־λ
    //----------------------------------
    guc_5ms_timer++;
    if (guc_5ms_timer >= 5)   //5ms��ʱ��
    {
        guc_5ms_timer = 0;
        bflg_5ms_reach = 1;
    }
    //----------------------------------
    guc_10ms_timer++;
    if (guc_10ms_timer >= 10) //10ms��ʱ��
    {
        guc_10ms_timer = 0;
        bflg_10ms_reach = 1;
    }
}




/***************************************END OF THE FILE******************************************************************************************/