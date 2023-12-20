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


long        temp;                                              //临时变量
long        IntDeg;                                            //温度
long        voltage;                                           //电压
char        task=0;                                            //当前显示状态
int         sec=0,min=14,hour=9,day=16,month=12;                //定义 秒 分 时 日 月
char        timing=1;                                          //计时运行标志
char        clock_change;                                      //正在修改的时钟位数
char        light;                                             //亮起熄灭lcd信号
long        timestamp = 0;                                     //时间戳
int         capvalue_1 = 0;                                     //第一次捕捉值
int         capvalue_2 = 0;                                     //第二次捕捉值
long        timestamp_1 = 0;                                   //第一次时间戳
long        timestamp_2 = 0;                                   //第二次时间戳
long        totaltime = 0;                                     //两次触发时间间隔
float       freq = 0;                                         //计算到的频率


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


/*主函数*/
void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;                   //关狗

    InitSystemClock();                          //初始化系统时钟
    TIME_Init();                                //初始化定时器
    LCD_Init();                                 //初始化LCD
    _enable_interrupts();                       //使能总中断
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


/*初始化系统时钟*/
void InitSystemClock(void)
{
    /*配置DCO为1MHz*/
    DCOCTL = CALDCO_1MHZ;
    BCSCTL1 = CALBC1_1MHZ;
    /*配置SMCLK的时钟源为DCO*/
    BCSCTL2 &= ~SELS;
    /*SMCLK的分频系数置为1*/
    BCSCTL2 &= ~(DIVS0 | DIVS1);
}


/*初始化LCD*/
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


/*初始化ADC*/
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


/*初始化定时器*/
void TIME_Init(void)
{
    /*设置时钟源为SMCLK*/
    TA1CTL |= TASSEL_2;
    /*设置工作模式为Up Mode*/
    TA1CTL |= MC_1;
    /*设置定时间隔*/
    TA1CCR0 = 49999;// 50ms 1MHz 1/1MHz 1ns 50ms / 1ns = 50000 50000 - 1 = 49999
    /*开启TAIFG中断*/
    TA1CTL |= TAIE;
    __bis_SR_register(GIE);

}


/*初始化测频定时器捕捉*/
void Capture_Init(void)
{
    /*设置时钟源为SMCLK设置工作模式为Up Mode开启TAIFG中断*/
    TA0CTL |= TASSEL_2+MC_1+TAIE;
    /*设置定时间隔*/
    TA0CCR0 = 49999;//50ms
    /*TA1,CCR1用于捕捉功能上升沿捕捉P1.2作为捕捉输入(CCI2A)允许捕捉比较中断*/
    TA0CCTL1 |= CAP+CM_1+CCIS_0+CCIE;
    //P1.2选择为Timer0_A3.CCI1A
    P1SEL |= BIT2;
}



/*显示时间*/
void clock(void)
{
    LCD_DisplaySeg(_LCD_COLON0);
    LCD_DisplaySeg(_LCD_COLON1);

    /*显示日期*/
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
        /*秒闪烁*/
        if(light)
        {
            /*显示秒*/
            LCD_DisplayDigit(sec/10,5);
            LCD_DisplayDigit(sec%10,6);
        }
        else if(~light)
        {
            /*清除秒*/
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,5);
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,6);
        }
        light++;
        if(light==2) light=0;
    }
    else if(clock_change==SET_MIN)
    {
        /*分闪烁*/
        if(light)
        {
            /*显示分*/
            LCD_DisplayDigit(min/10,3);
            LCD_DisplayDigit(min%10,4);
        }
        else if(~light)
        {
            /*清除分*/
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,3);
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,4);
        }
        light++;
        if(light==2) light=0;
    }
    else if(clock_change==SET_HOUR)
    {
        /*时闪烁*/
        if(light)
        {
            /*显示时*/
            LCD_DisplayDigit(hour/10,1);
            LCD_DisplayDigit(hour%10,2);
        }
        else if(~light)
        {
            /*清除时*/
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,1);
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,2);
        }
        light++;
        if(light==2) light=0;
    }
    else if(clock_change==SET_DAY)
    {
        /*日闪烁*/
        if(light)
        {
            /*显示日*/
            LCD_DisplayDigit(day/10,8);
            LCD_DisplayDigit(day%10,7);
        }
        else if(~light)
        {
            /*清除日*/
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,7);
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,8);
        }
        light++;
        if(light==2) light=0;
    }
    else if(clock_change==SET_MONTH)
    {
        /*月闪烁*/
        if(light)
        {
            /*显示月*/
            LCD_DisplayDigit(month/10,10);
            LCD_DisplayDigit(month%10,9);
        }
        else if(~light)
        {
            /*清除月*/
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,9);
            LCD_DisplayDigit(LCD_DIGIT_CLEAR,10);
        }
        light++;
        if(light==2) light=0;
    }
    HT1621_Reflash(LCD_Buffer);                //更新显存
}



/*显示温度*/
void thermometer(void)
{
    LCD_DisplaySeg(_LCD_DEGREE);                        //"°C"标志
    LCD_DisplaySeg(_LCD_DOT3);                          //温度小数点

    ADC10CTL0 |= ENC + ADC10SC;                         // Sampling and conversion start
    while(ADC10CTL1&ADC10BUSY);
    //-----ADC转换完成中断唤醒CPU后才执行以下代码-----
    temp = ADC10MEM;                                    //读取AD采样值
    ADC10CTL0&=~ENC;
    IntDeg= temp*42253/1023 - 27775;                    //转换为摄氏度，并100倍处理

    if( IntDeg>=0)   LCD_ClearSeg(_LCD_NEG);            //正温度，则清除负号
    else if(temp!=0)
    {
        IntDeg=-IntDeg;                                 //负温度，则做绝对值处理(同时防止超量程)
        LCD_DisplaySeg(_LCD_NEG);                       //负温度，添加负号
    }
    //-----清除4位显示数字-----
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,3);
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,4);
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,5);
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,6);
    //-----拆分4位并显示数字-----

    if(temp>0 && temp<1023)
    {
        LCD_ClearSeg(_LCD_ERROR);//清除"Error"
        if(IntDeg/10000>0) LCD_DisplayDigit(IntDeg/10000,2);            //百位数字
        if(IntDeg/1000>0) LCD_DisplayDigit((IntDeg%10000)/1000,3);      //十位数字
        LCD_DisplayDigit((IntDeg%1000)/100,4);                          //个位数字
        LCD_DisplayDigit((IntDeg%100)/10,5);                            //小数第一位
        LCD_DisplayDigit(IntDeg%10,6);                                  //小数第二位
    }
    //超出量程，显示"Error"和"-----"
    else
    {
        LCD_ClearSeg(_LCD_DOT3);                                        //清除小数点
        LCD_DisplaySeg(_LCD_ERROR);                                     //显示"Error"
        LCD_DisplaySeg(_LCD_2G);                                        //显示"-"
        LCD_DisplaySeg(_LCD_3G);                                        //显示"-"
        LCD_DisplaySeg(_LCD_4G);                                        //显示"-"
        LCD_DisplaySeg(_LCD_5G);                                        //显示"-"
        LCD_DisplaySeg(_LCD_6G);                                        //显示"-"
    }

    //-----更新缓存，真正显示-----
    HT1621_Reflash(LCD_Buffer);
}


/*显示电压*/
void voltmeter(void)
{
    LCD_DisplaySeg(_LCD_DOT3);                      //小数点
    LCD_DisplaySeg(_LCD_V);                         //"V"标志
    ADC10CTL0 |= ENC + ADC10SC;                     // Sampling and conversion start

    while(ADC10CTL1&ADC10BUSY);
    //-----ADC转换完成中断唤醒CPU后才执行以下代码-----
    temp = ADC10MEM;                                //读取AD采样值
    voltage= temp*360/1023;                         //转换为电压，并100倍处理

    //-----清除3位显示数字-----
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,4);            //电压个位
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,5);            //电压小数一位
    LCD_DisplayDigit(LCD_DIGIT_CLEAR,6);            //电压小数二位
    //-----拆分3位并显示数字-----
    if(voltage<360)
    {
        LCD_ClearSeg(_LCD_ERROR);                   //清除"Error标志"
        LCD_DisplayDigit(voltage/100,4);            //电压个位
        LCD_DisplayDigit((voltage%100)/10,5);       //电压小数一位
        LCD_DisplayDigit(voltage%10,6);             //电压小数二位
    }
    else
    {
        LCD_ClearSeg(_LCD_DOT3);
        LCD_DisplaySeg(_LCD_ERROR);
        LCD_DisplaySeg(_LCD_4G);
        LCD_DisplaySeg(_LCD_5G);
        LCD_DisplaySeg(_LCD_6G);
    }
    //-----更新缓存，真正显示-----
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



void I2C_IODect(void)                            //检测事件确实发生了
{
    static unsigned char KEY_Past=0,KEY_Now=0;
    KEY_Past=KEY_Now;

    //----判断I2C_IO10所连的KEY1按键是否被按下------
    //KEY1是加法按钮
    if((TCA6416A_InputBuffer&BIT8) == BIT8)
        KEY_Now |=BIT0;
    else
        KEY_Now &=~BIT0;
    //----判断I2C_IO11所连的KEY2按键是否被按下------
    //KEY2是减法按钮
    if((TCA6416A_InputBuffer&BIT9)== BIT9)
        KEY_Now |=BIT1;
    else
        KEY_Now &=~BIT1;
    //----判断I2C_IO12所连的KEY3按键是否被按下------
    //KEY3是切换修改项目按钮
    if((TCA6416A_InputBuffer&BITA) == BITA)
        KEY_Now |=BIT2;
    else
        KEY_Now &=~BIT2;
    //----判断I2C_IO13所连的KEY4按键是否被按下------
    //KEY4是切换功能按钮
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
            ADC10_Temperature_Init();                       //初始化ADC(温度)
        }
        else if(task==TASK_VOLTAGE)
        {
            ADC10_Voltage_Init();                           //初始化ADC(电压)
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
        case TA0IV_TACCR1://捕捉比较中断1
            if(cnt0 == 0)
            {
                capvalue_1 = TA0CCR1;//保存第一次捕捉值
                timestamp_1 = timestamp;//保存第一次时间戳
                cnt0 ++;
            }
            else
            {
                capvalue_2 = TA0CCR1;//保存第二次捕捉值
                timestamp_2 = timestamp;//保存第二次时间戳
                cnt0 = 0;
                timestamp=0;
                totaltime = (timestamp_2 - timestamp_1) * 50000 + capvalue_2 - capvalue_1;//计算总时间
                freq = (float)(1000000.0) / totaltime;
            }
            break;
        case TA0IV_TAIFG://溢出中断
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
