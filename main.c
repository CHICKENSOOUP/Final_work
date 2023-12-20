#include"MSP430G2553.h"
#include"TCA6416A.h"
#include"HT1621.h"
#include"LCD_128.h"



#define     TASK_CLOCK          0
#define     TASK_TEMPERATURE    1
#define     TASK_VOLTAGE        2
#define     TASK_FREQUENCY      3

#define     AY_PLUS             1
#define     DAY_MINUS           2
#define     MONTH_PLUS          3
#define     MONTH_MINUS         4

#define     NO_SET              0
#define     SET_SEC             1
#define     SET_MIN             2
#define     SET_HOUR            3
#define     SET_DAY             4
#define     SET_MONTH           5


long        temp;                                              //��ʱ����
long        IntDeg;                                            //�¶�
long        voltage;                                           //��ѹ
char        task=0;                                            //��ǰ��ʾ״̬
int         sec=0,min=14,hour=9,day=16,month=12;                //���� �� �� ʱ �� ��
char        timing=1;                                          //��ʱ���б�־
char        clock_change;                                      //�����޸ĵ�ʱ��λ��
char        light;                                             //����Ϩ��lcd�ź�
long        timestamp = 0;                                     //ʱ���
int         capvalue_1 = 0;                                     //��һ�β�׽ֵ
int         capvalue_2 = 0;                                     //�ڶ��β�׽ֵ
long        timestamp_1 = 0;                                   //��һ��ʱ���
long        timestamp_2 = 0;                                   //�ڶ���ʱ���
long        totaltime = 0;                                     //���δ���ʱ����
float       freq = 0;                                         //���㵽��Ƶ��


void InitSystemClock(void);
void LCD_Init(void);
void ADC10_Temperature_Init(void);
void TIME_Init(void);
void Capture_Init(void);
void clock(void);
void thermometer(void);
void voltmeter(void);
void freqmeter(void);
void I2C_IODect(void);
void change_month_day(int state);


/*������*/
void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;                   //�ع�

    InitSystemClock();                          //��ʼ��ϵͳʱ��
    TIME_Init();                                //��ʼ����ʱ��
    LCD_Init();                                 //��ʼ��LCD
    _enable_interrupts();                       //ʹ�����ж�
    while(1)
    {
        PinIN();
        I2C_IODect();
        switch(task)
        {
            case TASK_CLOCK:                clock();            break;
            case TASK_TEMPERATURE:          thermometer();      break;
            case TASK_VOLTAGE:              voltmeter();        break;
            case TASK_FREQUENCY:            freqmeter();        break;
            default:break;
        }

    }
}


/*��ʼ��ϵͳʱ��*/
void InitSystemClock(void)
{
    /*����DCOΪ1MHz*/
    DCOCTL = CALDCO_1MHZ;
    BCSCTL1 = CALBC1_1MHZ;
    /*����SMCLK��ʱ��ԴΪDCO*/
    BCSCTL2 &= ~SELS;
    /*SMCLK�ķ�Ƶϵ����Ϊ1*/
    BCSCTL2 &= ~(DIVS0 | DIVS1);
}


/*��ʼ��LCD*/
void LCD_Init(void)
{
    TCA6416A_Init();
    HT1621_init();
    LCD_Clear();
    LCD_Clear();
    LCD_DisplaySeg(_LCD_QDU_logo);
    LCD_DisplaySeg(_LCD_TI_logo);
    HT1621_Reflash(LCD_Buffer);

}


/*��ʼ��ADC*/
void ADC10_Temperature_Init(void)
{
    ADC10CTL0 &= ~ENC;
    ADC10CTL0 =  ADC10ON + REFON + ADC10SHT_2 + SREF_1;
    ADC10CTL1 = CONSEQ_0 + ADC10SSEL_0 + ADC10DIV_3 + SHS_0 + INCH_10;
    __delay_cycles(3000);
    ADC10CTL0 |= ENC;

}


void ADC10_Voltage_Init(void)
{
    ADC10CTL0 &= ~ENC;
    ADC10CTL0 =  ADC10ON + ADC10SHT_2 + SREF_0;
    ADC10CTL1 = CONSEQ_0 + ADC10SSEL_0 + ADC10DIV_3+SHS_0 + INCH_1;
    ADC10AE0 = BIT1;
    __delay_cycles(3000);
    ADC10CTL0 |= ENC;
}


/*��ʼ����ʱ��*/
void TIME_Init(void)
{
    /*����ʱ��ԴΪSMCLK*/
    TA1CTL |= TASSEL_2;
    /*���ù���ģʽΪUp Mode*/
    TA1CTL |= MC_1;
    /*���ö�ʱ���*/
    TA1CCR0 = 49999;// 50ms 1MHz 1/1MHz 1ns 50ms / 1ns = 50000 50000 - 1 = 49999
    /*����TAIFG�ж�*/
    TA1CTL |= TAIE;
    __bis_SR_register(GIE);

}


/*��ʼ����Ƶ��ʱ����׽*/
void Capture_Init(void)
{
    /*����ʱ��ԴΪSMCLK���ù���ģʽΪUp Mode����TAIFG�ж�*/
    TA0CTL |= TASSEL_2+MC_1+TAIE;
    /*���ö�ʱ���*/
    TA0CCR0 = 49999;//50ms
    /*TA1,CCR1���ڲ�׽���������ز�׽P1.2��Ϊ��׽����(CCI2A)����׽�Ƚ��ж�*/
    TA0CCTL1 |= CAP+CM_1+CCIS_0+CCIE;
    //P1.2ѡ��ΪTimer0_A3.CCI1A
    P1SEL |= BIT2;
}



/*��ʾʱ��*/
void clock(void)
{
    LCD_DisplaySeg(_LCD_COLON0);
    LCD_DisplaySeg(_LCD_COLON1);

    /*��ʾ����*/
    LCD_DisplayDigit(month/10,10);
    LCD_DisplayDigit(month%10,9);
    LCD_DisplayDigit(day/10,8);
    LCD_DisplayDigit(day%10,7);
    LCD_DisplayDigit(sec/10,5);
    LCD_DisplayDigit(sec%10,6);
    LCD_DisplayDigit(min/10,3);
    LCD_DisplayDigit(min%10,4);
    LCD_DisplayDigit(hour/10,1);
    LCD_DisplayDigit(hour%10,2);

    if(clock_change==SET_SEC)
    {
        /*����˸*/
        if(light)
        {
            /*��ʾ��*/
            LCD_DisplayDigit(sec/10,5);
            LCD_DisplayDigit(sec%10,6);
        }
        else if(~light)
        {
            /*�����*/
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,5);
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,6);
        }
        light++;
        if(light==2) light=0;
    }
    else if(clock_change==SET_MIN)
    {
        /*����˸*/
        if(light)
        {
            /*��ʾ��*/
            LCD_DisplayDigit(min/10,3);
            LCD_DisplayDigit(min%10,4);
        }
        else if(~light)
        {
            /*�����*/
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,3);
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,4);
        }
        light++;
        if(light==2) light=0;
    }
    else if(clock_change==SET_HOUR)
    {
        /*ʱ��˸*/
        if(light)
        {
            /*��ʾʱ*/
            LCD_DisplayDigit(hour/10,1);
            LCD_DisplayDigit(hour%10,2);
        }
        else if(~light)
        {
            /*���ʱ*/
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,1);
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,2);
        }
        light++;
        if(light==2) light=0;
    }
    else if(clock_change==SET_DAY)
    {
        /*����˸*/
        if(light)
        {
            /*��ʾ��*/
            LCD_DisplayDigit(day/10,8);
            LCD_DisplayDigit(day%10,7);
        }
        else if(~light)
        {
            /*�����*/
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,7);
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,8);
        }
        light++;
        if(light==2) light=0;
    }
    else if(clock_change==SET_MONTH)
    {
        /*����˸*/
        if(light)
        {
            /*��ʾ��*/
            LCD_DisplayDigit(month/10,10);
            LCD_DisplayDigit(month%10,9);
        }
        else if(~light)
        {
            /*�����*/
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,9);
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,10);
        }
        light++;
        if(light==2) light=0;
    }
    HT1621_Reflash(LCD_Buffer);                //�����Դ�
}



/*��ʾ�¶�*/
void thermometer(void)
{
    LCD_DisplaySeg(_LCD_DEGREE);                        //"��C"��־
    LCD_DisplaySeg(_LCD_DOT3);                          //�¶�С����

    ADC10CTL0 |= ENC + ADC10SC;                         // Sampling and conversion start
    while(ADC10CTL1&ADC10BUSY);
    //-----ADCת������жϻ���CPU���ִ�����´���-----
    temp = ADC10MEM;                                    //��ȡAD����ֵ
    ADC10CTL0&=~ENC;
    IntDeg= temp*42253/1023 - 27775;                    //ת��Ϊ���϶ȣ���100������

    if( IntDeg>=0)   LCD_ClearSeg(_LCD_NEG);            //���¶ȣ����������
    else if(temp!=0)
    {
        IntDeg=-IntDeg;                                 //���¶ȣ���������ֵ����(ͬʱ��ֹ������)
        LCD_DisplaySeg(_LCD_NEG);                       //���¶ȣ���Ӹ���
    }
    //-----���4λ��ʾ����-----
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,3);
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,4);
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,5);
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,6);
    //-----���4λ����ʾ����-----

    if(temp>0 && temp<1023)
    {
        LCD_ClearSeg(_LCD_ERROR);//���"Error"
        if(IntDeg/10000>0) LCD_DisplayDigit(IntDeg/10000,2);            //��λ����
        if(IntDeg/1000>0) LCD_DisplayDigit((IntDeg%10000)/1000,3);      //ʮλ����
        LCD_DisplayDigit((IntDeg%1000)/100,4);                          //��λ����
        LCD_DisplayDigit((IntDeg%100)/10,5);                            //С����һλ
        LCD_DisplayDigit(IntDeg%10,6);                                  //С���ڶ�λ
    }
    //�������̣���ʾ"Error"��"-----"
    else
    {
        LCD_ClearSeg(_LCD_DOT3);                                        //���С����
        LCD_DisplaySeg(_LCD_ERROR);                                     //��ʾ"Error"
        LCD_DisplaySeg(_LCD_2G);                                        //��ʾ"-"
        LCD_DisplaySeg(_LCD_3G);                                        //��ʾ"-"
        LCD_DisplaySeg(_LCD_4G);                                        //��ʾ"-"
        LCD_DisplaySeg(_LCD_5G);                                        //��ʾ"-"
        LCD_DisplaySeg(_LCD_6G);                                        //��ʾ"-"
    }

    //-----���»��棬������ʾ-----
    HT1621_Reflash(LCD_Buffer);
}


/*��ʾ��ѹ*/
void voltmeter(void)
{
    LCD_DisplaySeg(_LCD_DOT3);                      //С����
    LCD_DisplaySeg(_LCD_V);                         //"V"��־
    ADC10CTL0 |= ENC + ADC10SC;                     // Sampling and conversion start

    while(ADC10CTL1&ADC10BUSY);
    //-----ADCת������жϻ���CPU���ִ�����´���-----
    temp = ADC10MEM;                                //��ȡAD����ֵ
    voltage= temp*360/1023;                         //ת��Ϊ��ѹ����100������

    //-----���3λ��ʾ����-----
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,4);            //��ѹ��λ
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,5);            //��ѹС��һλ
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,6);            //��ѹС����λ
    //-----���3λ����ʾ����-----
    if(voltage<360)
    {
        LCD_ClearSeg(_LCD_ERROR);                   //���"Error��־"
        LCD_DisplayDigit(voltage/100,4);            //��ѹ��λ
        LCD_DisplayDigit((voltage%100)/10,5);       //��ѹС��һλ
        LCD_DisplayDigit(voltage%10,6);             //��ѹС����λ
    }
    else
    {
        LCD_ClearSeg(_LCD_DOT3);
        LCD_DisplaySeg(_LCD_ERROR);
        LCD_DisplaySeg(_LCD_4G);
        LCD_DisplaySeg(_LCD_5G);
        LCD_DisplaySeg(_LCD_6G);
    }
    //-----���»��棬������ʾ-----
    HT1621_Reflash(LCD_Buffer);
}


/**/
void freqmeter(void)
{

    LCD_DisplaySeg(_LCD_Hz);
    LCD_DisplaySeg(_LCD_DOT3);
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,6);
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,5);
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,4);
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,3);
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,2);
    if(freq<1000)
    {
        LCD_ClearSeg(_LCD_k_Hz);
        temp=freq*100;
        LCD_DisplayDigit(temp%10,6);
        LCD_DisplayDigit((temp%100)/10,5);
        LCD_DisplayDigit((temp%1000)/100,4);
        if(temp>1000) LCD_DisplayDigit((temp%10000)/1000,3);
        if(temp>10000) LCD_DisplayDigit(temp/10000,2);

    }
    else
    {
        LCD_DisplaySeg(_LCD_k_Hz);
        temp=freq*100/1000;
        if(temp/10000>0)    LCD_DisplayDigit(temp/10000,2);
        if(temp/1000>0)   LCD_DisplayDigit((temp%10000)/1000,3);
        LCD_DisplayDigit((temp%1000)/100,4);
        LCD_DisplayDigit((temp%100)/10,5);
        LCD_DisplayDigit(temp%10,6);
    }

    HT1621_Reflash(LCD_Buffer);
    TA0CTL |= TAIE;
    TA0CCTL1 |= CCIE;
    __delay_cycles(500000);
    TA0CTL &= ~TAIE;
    TA0CCTL1 &= ~CCIE;
}



void I2C_IODect(void)                            //����¼�ȷʵ������
{
    static unsigned char KEY_Past=0,KEY_Now=0;
    KEY_Past=KEY_Now;

    //----�ж�I2C_IO10������KEY1�����Ƿ񱻰���------
    //KEY1�Ǽӷ���ť
    if((TCA6416A_InputBuffer&BIT8) == BIT8)
        KEY_Now |=BIT0;
    else
        KEY_Now &=~BIT0;
    //----�ж�I2C_IO11������KEY2�����Ƿ񱻰���------
    //KEY2�Ǽ�����ť
    if((TCA6416A_InputBuffer&BIT9)== BIT9)
        KEY_Now |=BIT1;
    else
        KEY_Now &=~BIT1;
    //----�ж�I2C_IO12������KEY3�����Ƿ񱻰���------
    //KEY3���л��޸���Ŀ��ť
    if((TCA6416A_InputBuffer&BITA) == BITA)
        KEY_Now |=BIT2;
    else
        KEY_Now &=~BIT2;
    //----�ж�I2C_IO13������KEY4�����Ƿ񱻰���------
    //KEY4���л����ܰ�ť
    if((TCA6416A_InputBuffer&BITB) ==  BITB)
        KEY_Now |=BIT3;
    else
        KEY_Now &=~BIT3;


    if((KEY_Past&BIT0)&&(~KEY_Now&BIT0))
    {
        if(clock_change==SET_SEC)
        {
            sec++;
            if(sec==60)
                sec=0;
            LCD_DisplayDigit(sec/10,5);
            LCD_DisplayDigit(sec%10,6);
        }
        else if(clock_change==SET_MIN)
        {
            min++;
            if(min==60)
                min=0;
            LCD_DisplayDigit(min/10,3);
            LCD_DisplayDigit(min%10,4);
        }
        else if(clock_change==SET_HOUR)
        {
            hour++;
            if(hour==24)
                hour=0;
            LCD_DisplayDigit(hour/10,1);
            LCD_DisplayDigit(hour%10,2);
        }
        else if(clock_change==SET_DAY)
        {
            day++;
            change_month_day(DAY_PLUS);
            LCD_DisplayDigit(day/10,8);
            LCD_DisplayDigit(day%10,7);
        }
        else if(clock_change==SET_MONTH)
        {
            month++;
            change_month_day(MONTH_PLUS);
            LCD_DisplayDigit(month/10,10);
            LCD_DisplayDigit(month%10,9);
        }

    }
    else if((KEY_Past&BIT1)&&(~KEY_Now&BIT1))
    {
        if(clock_change==SET_SEC)
        {
            sec--;
            if(sec==-1)
                sec=59;
            LCD_DisplayDigit(sec/10,5);
            LCD_DisplayDigit(sec%10,6);
        }
        else if(clock_change==SET_MIN)
        {
            min--;
            if(min==-1)
                min=59;
            LCD_DisplayDigit(min/10,3);
            LCD_DisplayDigit(min%10,4);
        }
        else if(clock_change==SET_HOUR)
        {
            hour--;
            if(hour==-1)
                hour=23;
            LCD_DisplayDigit(hour/10,1);
            LCD_DisplayDigit(hour%10,2);
        }
        else if(clock_change==SET_DAY)
        {
            day--;
            change_month_day(DAY_MINUS);
            LCD_DisplayDigit(day/10,8);
            LCD_DisplayDigit(day%10,7);
        }
        else if(clock_change==SET_MONTH)
        {
            month--;
            change_month_day(MONTH_MINUS);
            LCD_DisplayDigit(month/10,10);
            LCD_DisplayDigit(month%10,9);
        }
    }


    else if((KEY_Past&BIT2)&&(~KEY_Now&BIT2))
    {
        clock_change++;
        timing=0;
        if(clock_change>SET_MONTH)
        {
            clock_change=NO_SET;
            timing=1;
        }
    }



    else if((KEY_Past&BIT3)&&(~KEY_Now&BIT3))
    {
        task++;
        if(task==TASK_TEMPERATURE)
        {
            ADC10_Temperature_Init();                       //��ʼ��ADC(�¶�)
        }
        else if(task==TASK_VOLTAGE)
        {
            ADC10_Voltage_Init();                           //��ʼ��ADC(��ѹ)
        }
        else if(task==TASK_FREQUENCY)
        {
            Capture_Init();
        }
        else if(task>TASK_FREQUENCY)
        {
            task=TASK_CLOCK;
        }
        LCD_Clear();
        LCD_DisplaySeg(_LCD_QDU_logo);
        LCD_DisplaySeg(_LCD_TI_logo);

    }
}

void change_month_day(int state)
{
    switch(state)
    {
        case DAY_PLUS:
            if(month==1){
               if(day>31){
                    day=1;
                    month++;
                }
            }
            if(month==2){
               if(day>28){
                    day=1;
                    month++;
                }
            }
            if(month==3){
               if(day>31){
                    day=1;
                    month++;
                }
            }
            if(month==4){
               if(day==31){
                    day=1;
                    month++;
                }
            }
            if(month==5){
               if(day>31){
                    day=1;
                    month++;
                }
            }
            if(month==6){
               if(day==31){
                    day=1;
                    month++;
                }
            }
            if(month==7){
               if(day>31){
                    day=1;
                    month++;
                }
            }
            if(month==8){
               if(day>31){
                    day=1;
                    month++;
                }
            }
            if(month==9){
               if(day==31){
                    day=1;
                    month++;
                }
            }
            if(month==10){
               if(day>31){
                    day=1;
                    month++;
                }
            }
            if(month==11){
               if(day==31){
                    day=1;
                    month++;
                }
            }
            if(month==12){
               if(day>31){
                    day=1;
                    month=1;
                }
            }
            break;
        case DAY_MINUS:
            if(month==1){
                if(day==0)
                day=31;
            }
            if(month==2){
                if(day==0)
                day=28;
            }
            if(month==3){
                if(day==0)
                day=31;
            }
            if(month==4){
                if(day==0)
                day=30;
            }
            if(month==5){
                if(day==0)
                day=31;
            }
            if(month==6){
                if(day==0)
                day=30;
            }
            if(month==7){
                if(day==0)
                day=31;
            }
            if(month==8){
                if(day==0)
                day=31;
            }
            if(month==9){
                if(day==0)
                day=30;
            }
            if(month==10){
                if(day==0)
                day=31;
            }
            if(month==11){
                if(day==0)
                day=30;
            }
            if(month==12){
                if(day==0)
                day=31;
            }
            break;
        case MONTH_PLUS:
            if(month==13){
                month=1;
            }
            if(month==2){
                if(day>28)
                    day=28;
            }
            if(month==4){
                if(day>30)
                    day=30;
            }
            if(month==6){
                if(day>30)
                    day=30;
            }
            if(month==9){
                if(day>30)
                    day=30;
            }
            if(month==11){
                if(day>30)
                    day=30;
            }
            break;
        case MONTH_MINUS:
            if(month==0){
                month=12;
            }

            if(month==2){
                if(day>28)
                    day=28;
            }
            if(month==4){
                if(day>30)
                    day=30;
            }
            if(month==6){
                if(day>30)
                    day=30;
            }
            if(month==9){
                if(day>30)
                    day=30;
            }
            if(month==11){
                if(day>30)
                    day=30;
            }
            break;

        default:    break;
    }
}


#pragma vector = TIMER0_A1_VECTOR
__interrupt void Time0_Tick(void)
{
    __bic_SR_register_on_exit(CPUOFF);
    static char cnt0 = 0;
    switch(TA0IV)
    {
        case TA0IV_TACCR1://��׽�Ƚ��ж�1
            if(cnt0 == 0)
            {
                capvalue_1 = TA0CCR1;//�����һ�β�׽ֵ
                timestamp_1 = timestamp;//�����һ��ʱ���
                cnt0 ++;
            }
            else
            {
                capvalue_2 = TA0CCR1;//����ڶ��β�׽ֵ
                timestamp_2 = timestamp;//����ڶ���ʱ���
                cnt0 = 0;
                timestamp=0;
                totaltime = (timestamp_2 - timestamp_1) * 50000 + capvalue_2 - capvalue_1;//������ʱ��
                freq = (float)(1000000.0) / totaltime;
            }
            break;
        case TA0IV_TAIFG://����ж�
            timestamp ++;
            break;
        default:break;
    }
}



#pragma vector = TIMER1_A1_VECTOR
__interrupt void Time1_Tick(void)
{
    __bic_SR_register_on_exit(CPUOFF);
    static int cnt = 0;
    switch(TA1IV)
    {
        case 0x02:
            break;
        case 0x04:
            break;
        case 0x0A:
            if(timing>=1)
            {
                cnt ++;
                if(cnt >= 2)
                {
                    cnt = 0;
                    sec++;
                    if(sec>=60)
                    {
                        sec=0;
                        min++;
                        if(min>=60)
                        {
                            min=0;
                            hour++;
                            if(hour>=24)
                            {
                                hour=0;
                                day++;
                                change_month_day(DAY_PLUS);
                            }
                        }
                    }
                }
            }
            break;
        default:
            break;
    }
}
