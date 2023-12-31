#include "MSP430G2553.h"
#include "UART_Event.h"
///******************************************************************************************************
// * 名       称：USCI_A0_init()
// * 功       能：初始化USCI_A0模块为UART模式
// * 入口参数：无
// * 出口参数：无
// * 说       明：UART设为波特率9600,8位数据,无校验,1位停止位
// * 					UART初始化配置较复杂，可以使用Grace配置后再移植代码的方法
// * 范       例：无
// ******************************************************************************************************/
void USCI_A0_init(void)
{
	//-----开启IO口的TXD和RXD功能-----
	  P1SEL = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
	  P1SEL2 = BIT1 + BIT2;
	  //-----设置UART时钟源为ACLK-----
	  UCA0CTL1 |= UCSSEL_1;                // CLK = ACLK
	  //-----移植Grace配置的波特率参数-----
	  UCA0BR0 = 0x03;                           // 32kHz/9600 = 3.41
	  UCA0BR1 = 0x00;
	  UCA0MCTL = UCBRS1 + UCBRS0;     // Modulation UCBRSx = 3
	  UCA0CTL1 &= ~UCSWRST;               // **Initialize USCI state machine**
      //IE2 |= UCA0TXIE;           // Enable USCI_A0 TX/RX interrupt
      //_enable_interrupts();                 		  // 开总中断
}
///******************************************************************************************************
// * 名       称：USCI0TX_ISR()
// * 功       能：响应Tx中断服务
// * 入口参数：无
// * 出口参数：无
// * 说       明：凡是中断标志位有可能不被自动清除的，均手动清除一次，以防万一
// * 范       例：无
// ******************************************************************************************************/
//#pragma vector=USCIAB0TX_VECTOR
//__interrupt void USCI0TX_ISR(void)
//{
//    IFG2&=~UCA0TXIFG;     // 手动清除标志位
//	UART_OnTx();					// 调用Tx事件处理函数
//
//}
//
//
//
