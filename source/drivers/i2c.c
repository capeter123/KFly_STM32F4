/* *
 *
 * Hardware Communication Layer for the I2C bus
 *
 * Many thanks to yigiter's example code that made
 * this implementation much easier to complete!
 *
 * */


#include "i2c.h"

/* Private Defines */
#define I2CBus						I2C2
#define RCC_APB1Periph_I2Cx			RCC_APB1Periph_I2C2
#define RCC_AHB1Periph_GPIO_SCL		RCC_AHB1Periph_GPIOB
#define RCC_AHB1Periph_GPIO_SDA		RCC_AHB1Periph_GPIOB
#define GPIO_AF_I2Cx				GPIO_AF_I2C2
#define GPIO_SCL					GPIOB
#define GPIO_SDA					GPIOB
#define GPIO_Pin_SCL				GPIO_Pin_10
#define GPIO_Pin_SDA				GPIO_Pin_11
#define GPIO_PinSource_SCL			GPIO_PinSource10
#define GPIO_PinSource_SDA			GPIO_PinSource11

/* Private Typedefs */
typedef struct
{
	I2C_MASTER_SETUP_Type *RXTX_Setup;		/* Transmission setup */
	I2C_DIRECTION_Type Direction;			/* Current direction phase */
} I2C_INT_CFG_Type;

/* Global variable defines */
volatile I2C_INT_CFG_Type I2Ctmp[3];		/* Pointer to I2C Config Setup */
volatile uint16_t status = 0;
volatile int whereami = 0;

/* I2C Interrupt handlers */
void I2C_MasterHandler(I2C_TypeDef *);
void I2C2_EV_IRQHandler(void);
void I2C2_ER_IRQHandler(void);

/* Private function defines */
int8_t I2C_getNum(I2C_TypeDef *);
static uint16_t I2C_Addr(I2C_TypeDef *, uint8_t, uint8_t);
static uint16_t I2C_SendByte(I2C_TypeDef *, uint8_t);
static uint16_t I2C_Read(I2C_TypeDef *, uint8_t *);
static uint16_t I2C_Start(I2C_TypeDef *);
static uint16_t WaitSR1FlagsSet(I2C_TypeDef *, uint16_t);
static uint16_t WaitLineIdle(I2C_TypeDef *);

/* Private external functions */

void SensorBusInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	I2C_InitTypeDef  I2C_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Enable the I2C */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2Cx, ENABLE);
	/* Reset the Peripheral */
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2Cx, ENABLE);
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2Cx, DISABLE);

	/* Enable the GPIOs for the SCL/SDA Pins */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIO_SCL | RCC_AHB1Periph_GPIO_SDA, ENABLE);

	/* Configure and initialize the GPIOs */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_SCL;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIO_SCL, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_SDA;
	GPIO_Init(GPIO_SDA, &GPIO_InitStructure);

	/* Connect GPIO pins to peripheral */
	GPIO_PinAFConfig(GPIO_SCL, GPIO_PinSource_SCL, GPIO_AF_I2Cx);
	GPIO_PinAFConfig(GPIO_SDA, GPIO_PinSource_SDA, GPIO_AF_I2Cx);

	/* Configure and Initialize the I2C */
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_OwnAddress1 = 0x00; /* Unimportant, we are master */
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; /* Unimportant, we are master */
	I2C_InitStructure.I2C_ClockSpeed = 400000; /* 400kHz clock */

	/* Initialize the Peripheral */
	I2C_Init(I2CBus, &I2C_InitStructure);

	/* Initialize interrupts */
	NVIC_InitStructure.NVIC_IRQChannel = I2C2_EV_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6; /* Lowest possible when using FreeRTOS */
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	//NVIC_InitStructure.NVIC_IRQChannel = I2C2_ER_IRQn;
	//NVIC_Init(&NVIC_InitStructure);

	/* I2C Peripheral Enable */
	I2C_Cmd(I2CBus, ENABLE);
}

int8_t I2C_getNum(I2C_TypeDef *I2Cx)
{
	if (I2Cx == I2C1)
		return 0;

	else if (I2Cx == I2C2)
		return 1;

	else if (I2Cx == I2C3)
		return 2;

	else
		return -1;
}

ErrorStatus I2C_MasterTransferData(I2C_TypeDef *I2Cx, I2C_MASTER_SETUP_Type *TransferCfg, I2C_TRANSFER_OPTION_Type Opt)
{
	uint8_t *txdat;
	uint8_t *rxdat;
	uint8_t tmp;
	/* Enable the ACK and reset Pos */
	I2Cx->CR1 |= I2C_CR1_ACK;
	I2Cx->CR1 &= ~I2C_CR1_POS;
	whereami = 0;

	if (Opt == I2C_TRANSFER_POLLING)
	{
		TransferCfg->Retransmissions_Count = 0;
retry:
		I2Cx->SR1 &= ~(I2Cx->SR1 & I2C_ERROR_BITMASK); /* Clear errors */
		if (TransferCfg->Retransmissions_Count > TransferCfg->Retransmissions_Max)
		{ /* Maximum number of retransmissions reached, abort */
			I2Cx->CR1 |= I2C_CR1_STOP;
			return ERROR;
		}
		/* Reset all values to default state */
		txdat = TransferCfg->TX_Data;
		rxdat = TransferCfg->RX_Data;

		/* Reset I2C setup value to default state */
		TransferCfg->TX_Count = 0;
		TransferCfg->RX_Count = 0;
		TransferCfg->Status = 0;

		/* Send start */
		whereami = 1;
		TransferCfg->Status = I2C_Start(I2Cx);
		if (TransferCfg->Status & (I2C_ERROR_BIT | I2C_ERROR_BITMASK)) /* Check for errors */
		{
			TransferCfg->Retransmissions_Count++;
			goto retry;
		}

		/* In case of sending data first */
		if ((TransferCfg->TX_Length != 0) && (TransferCfg->TX_Data != NULL))
		{
			/* Send slave address + W direction bit = 0 */
			whereami = 2;
			TransferCfg->Status = I2C_Addr(I2Cx, TransferCfg->Slave_Address_7bit, 0);
			if (TransferCfg->Status & (I2C_ERROR_BIT | I2C_ERROR_BITMASK)) /* Check for errors */
			{
				TransferCfg->Retransmissions_Count++;
				goto retry;
			}
			(void)I2Cx->SR2;

			/* Send a number of data bytes */
			while (TransferCfg->TX_Count < TransferCfg->TX_Length)
			{
				whereami = 3;
				TransferCfg->Status = I2C_SendByte(I2Cx, *(txdat++));
				if (TransferCfg->Status & (I2C_ERROR_BIT | I2C_ERROR_BITMASK)) /* Check for errors */
				{
					TransferCfg->Retransmissions_Count++;
					goto retry;
				}
				TransferCfg->TX_Count++;
			}

			whereami = 4;
			TransferCfg->Status = WaitSR1FlagsSet(I2Cx, I2C_SR1_BTF);
			if (TransferCfg->Status & (I2C_ERROR_BIT | I2C_ERROR_BITMASK)) /* Check for errors */
			{
				TransferCfg->Retransmissions_Count++;
				goto retry;
			}

			/* Send Stop if no data is to be received */
			if ((TransferCfg->RX_Length == 0) || (TransferCfg->RX_Data == NULL))
			{
				I2Cx->CR1 |= I2C_CR1_STOP;
				whereami = 5;
				TransferCfg->Status = WaitLineIdle(I2Cx);
				return SUCCESS;
			}
		}

		/* Second Start condition (Repeat Start) */
		if ((TransferCfg->TX_Length != 0) && (TransferCfg->TX_Data != NULL) && \
			(TransferCfg->RX_Length != 0) && (TransferCfg->RX_Data != NULL))
		{
			whereami = 6;
			TransferCfg->Status = I2C_Start(I2Cx);
			if (TransferCfg->Status & (I2C_ERROR_BIT | I2C_ERROR_BITMASK)) /* Check for errors */
			{
				TransferCfg->Retransmissions_Count++;
				goto retry;
			}
			(void)I2Cx->SR2;
		}

		/* *
		 *
		 * --------------------------- RECIEVE PHASE ---------------------------
		 * The I2C read on the STM32F4xx is not symmetric so special
		 * care must be taken if the number of bytes to be read are
		 * one, two or more than two.
		 *
		 * */


		/* Then, start reading after sending data */
		if ((TransferCfg->RX_Length != 0) && (TransferCfg->RX_Data != NULL))
		{
			/* Send slave address + RD direction bit = 1 */
			whereami = 7;
			TransferCfg->Status = I2C_Addr(I2Cx, TransferCfg->Slave_Address_7bit, 1);
			if (TransferCfg->Status & (I2C_ERROR_BIT | I2C_ERROR_BITMASK)) /* Check for errors */
			{
				TransferCfg->Retransmissions_Count++;
				goto retry;
			}

			/* Receive a number of data bytes */

			if (TransferCfg->RX_Length == 1) /* We are going to read only 1 byte */
			{
				/* Before Clearing Addr bit by reading SR2, we have to cancel ack. */
				I2Cx->CR1 &= ~I2C_CR1_ACK;

				/* Now Read the SR2 to clear ADDR */
				(void)I2Cx->SR2;

				/* Order a STOP condition */
				/* Note: Spec_p583 says this should be done just after clearing ADDR */
				/* If it is done before ADDR is set, a STOP is generated immediately as the clock is being stretched */
				I2Cx->CR1 |= I2C_CR1_STOP;
				/* Be careful that till the stop condition is actually transmitted the clock will stay active
				 * even if a NACK is generated after the next received byte. */

				/* Read the next byte */
				whereami = 8;
				TransferCfg->Status = I2C_Read(I2Cx, rxdat);
				if (TransferCfg->Status & (I2C_ERROR_BIT | I2C_ERROR_BITMASK)) /* Check for errors */
				{
					TransferCfg->Retransmissions_Count++;
					goto retry;
				}
				TransferCfg->RX_Count++;

				/* Make Sure Stop bit is cleared and Line is now Idle */
				whereami = 9;
				TransferCfg->Status = WaitLineIdle(I2Cx);

				/* Enable the Acknowledgment again */
				I2Cx->CR1 |= ((uint16_t)I2C_CR1_ACK);
			}

			else if (TransferCfg->RX_Length == 2) /* We are going to read 2 bytes (See: Spec_p584) */
			{
				/* Before Clearing Addr, reset ACK, set POS */
				I2Cx->CR1 &= ~I2C_CR1_ACK;
				I2Cx->CR1 |= I2C_CR1_POS;

				/* Read the SR2 to clear ADDR */
				(void)I2Cx->SR2;

				whereami = 10;
				/* Wait for the next 2 bytes to be received (1st in the DR, 2nd in the shift register) */
				TransferCfg->Status = WaitSR1FlagsSet(I2Cx, I2C_SR1_BTF);
				/* As we don't read anything from the DR, the clock is now being stretched. */

				/* Order a stop condition (as the clock is being stretched, the stop condition is generated immediately) */
				I2Cx->CR1 |= I2C_CR1_STOP;

				/* Read the next two bytes */
				whereami = 11;
				TransferCfg->Status = I2C_Read(I2Cx, rxdat++);
				if (TransferCfg->Status & (I2C_ERROR_BIT | I2C_ERROR_BITMASK)) /* Check for errors */
				{
					TransferCfg->Retransmissions_Count++;
					goto retry;
				}
				TransferCfg->RX_Count++;

				whereami = 12;
				TransferCfg->Status = I2C_Read(I2Cx, rxdat);
				if (TransferCfg->Status & (I2C_ERROR_BIT | I2C_ERROR_BITMASK)) /* Check for errors */
				{
					TransferCfg->Retransmissions_Count++;
					goto retry;
				}
				TransferCfg->RX_Count++;

				/* Make Sure Stop bit is cleared and Line is now Idle */
				whereami = 13;
				TransferCfg->Status = WaitLineIdle(I2Cx);

				/* Enable the ack and reset Pos */
				I2Cx->CR1 |= I2C_CR1_ACK;
				I2Cx->CR1 &= ~I2C_CR1_POS;
			}
			else /* We have more than 2 bytes. See spec_p585 */
			{
				/* Read the SR2 to clear ADDR */
				(void)I2Cx->SR2;

				while((TransferCfg->RX_Length - TransferCfg->RX_Count) > 3) /* Read till the last 3 bytes */
				{
					whereami = 14;
					TransferCfg->Status = I2C_Read(I2Cx, rxdat++);
					if (TransferCfg->Status & (I2C_ERROR_BIT | I2C_ERROR_BITMASK)) /* Check for errors */
					{
						TransferCfg->Retransmissions_Count++;
						goto retry;
					}
					TransferCfg->RX_Count++;
				}

				/* 3 more bytes to read. Wait till the next to is actually received */
				whereami = 15;
				TransferCfg->Status = WaitSR1FlagsSet(I2Cx, I2C_SR1_BTF);
				if (TransferCfg->Status & (I2C_ERROR_BIT | I2C_ERROR_BITMASK)) /* Check for errors */
				{
					TransferCfg->Retransmissions_Count++;
					goto retry;
				}
				/* Here the clock is stretched. One more to read. */

				/* Reset Ack */
				I2Cx->CR1 &= ~I2C_CR1_ACK;

				/* Read N-2 */
				whereami = 16;
				TransferCfg->Status = I2C_Read(I2Cx, rxdat++);
				if (TransferCfg->Status & (I2C_ERROR_BIT | I2C_ERROR_BITMASK)) /* Check for errors */
				{
					TransferCfg->Retransmissions_Count++;
					goto retry;
				}
				TransferCfg->RX_Count++;

				/* Once we read this, N is going to be read to the shift register and NACK is generated */

				/* Wait for the BTF */
				whereami = 17;
				TransferCfg->Status = WaitSR1FlagsSet(I2Cx, I2C_SR1_BTF); /* N-1 is in DR, N is in shift register */
				if (TransferCfg->Status & (I2C_ERROR_BIT | I2C_ERROR_BITMASK)) /* Check for errors */
				{
					TransferCfg->Retransmissions_Count++;
					goto retry;
				}
				/* Here the clock is stretched */

				/* Generate a stop condition */
				I2Cx->CR1 |= I2C_CR1_STOP;

				/* Read the last two bytes (N-1 and N) */
				whereami = 18;
				TransferCfg->Status = I2C_Read(I2Cx, rxdat++);
				if (TransferCfg->Status & (I2C_ERROR_BIT | I2C_ERROR_BITMASK)) /* Check for errors */
				{
					TransferCfg->Retransmissions_Count++;
					goto retry;
				}
				TransferCfg->RX_Count++;

				whereami = 19;
				TransferCfg->Status = I2C_Read(I2Cx, rxdat);
				if (TransferCfg->Status & (I2C_ERROR_BIT | I2C_ERROR_BITMASK)) /* Check for errors */
				{
					TransferCfg->Retransmissions_Count++;
					goto retry;
				}
				TransferCfg->RX_Count++;

				whereami = 20;
				/* Make Sure Stop bit is cleared and Line is now Idle  */
				TransferCfg->Status = WaitLineIdle(I2Cx);

				/* Enable the ACK */
				I2Cx->CR1 |= I2C_CR1_ACK;
			}
		}
	}
	else /* Interrupt transfer */
	{
		int8_t I2C_num = I2C_getNum(I2Cx);

		/* Copy setup into local variable */
		I2Ctmp[I2C_num].Direction = I2C_SENDING;
		I2Ctmp[I2C_num].RXTX_Setup = TransferCfg;
		TransferCfg->Status = 0;
		TransferCfg->TX_Count = 0;
		TransferCfg->RX_Count = 0;
		TransferCfg->Retransmissions_Count = 0;

		I2C_ITConfig(I2Cx, (I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR), ENABLE);
		I2Cx->CR1 |= I2C_CR1_START; /* Send start condition to start the transfer */
	}

	return SUCCESS;
}

void I2C_MasterHandler(I2C_TypeDef *I2Cx)
{
	status = I2Cx->SR1 & I2C_SR1_BITMASK;
	int8_t I2C_num = I2C_getNum(I2Cx);
	I2Ctmp[I2C_num].RXTX_Setup->Status = status; /* Save current status */

	/* *
	 * Note to self:
	 * In order to prevent repeated running of the ISR, when TXE or RXNE = 1 and
	 * awaiting BTF = 1, turn off the buffer interrupts (ITBUFEN) and just await
	 * the BTF interrupt. This should break the "infinite ISR recall"-loop.
	 * */


	if (status & I2C_ERROR_BITMASK) /* Error */
	{
		I2Cx->SR1 &= ~(I2Cx->SR1 & I2C_ERROR_BITMASK); /* Clear errors */
		I2Ctmp[I2C_num].RXTX_Setup->Retransmissions_Count++;

		if (I2Ctmp[I2C_num].RXTX_Setup->Retransmissions_Count > I2Ctmp[I2C_num].RXTX_Setup->Retransmissions_Max)
		{ 	/* Maximum number of retransmissions reached, abort */
			I2C_ITConfig(I2Cx, (I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR), DISABLE);
			I2Cx->CR1 |= I2C_CR1_STOP;
			I2Ctmp[I2C_num].RXTX_Setup->Status |= I2C_ERROR_BIT; /* Set error bit */
		}
		else
		{
			I2Ctmp[I2C_num].RXTX_Setup->TX_Count = 0;
			I2Ctmp[I2C_num].RXTX_Setup->RX_Count = 0;
			I2Ctmp[I2C_num].RXTX_Setup->Status = 0;

			I2C_ITConfig(I2Cx, (I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR), ENABLE);
			I2Cx->CR1 |= I2C_CR1_START; /* Send start condition to start the transfer */
		}

		return;
	}

	if (I2Ctmp[I2C_num].Direction == I2C_SENDING) /* Sending data */
	{
		switch (status & I2C_STATUS_BITMASK)
		{
			case I2C_SR1_SB: /* Start condition sent */
				if ((I2Ctmp[I2C_num].RXTX_Setup->TX_Length != 0) && (I2Ctmp[I2C_num].RXTX_Setup->TX_Data != NULL))
				{ 	/* If there is data to send, send Addr+W */
					I2Cx->DR = (uint16_t)(I2Ctmp[I2C_num].RXTX_Setup->Slave_Address_7bit << 1);
				}
				else /* Else go to Addr+R */
				{
					I2Ctmp[I2C_num].Direction = I2C_RECEIVING;
					goto send_slar;
				}
				break;

			case I2C_SR1_ADDR: /* Address+W sent, ACK received */
			case (I2C_SR1_ADDR | I2C_SR1_TXE):
				(void)I2Cx->SR2; /* Read SR2 to clear ADDR */
				/* Start sending the first byte */
				I2Cx->DR = (uint16_t)*(I2Ctmp[I2C_num].RXTX_Setup->TX_Data + I2Ctmp[I2C_num].RXTX_Setup->TX_Count++);
				break;

			case I2C_SR1_TXE: /* Data has been sent to the shift register, transmit register empty */
				if (I2Ctmp[I2C_num].RXTX_Setup->TX_Count == I2Ctmp[I2C_num].RXTX_Setup->TX_Length)
				{ 	/* All data has been sent, turn of TXE interrupt and await BTF */
					I2C_ITConfig(I2Cx, I2C_IT_BUF, DISABLE);
				}
				else
				{ 	/* More data to send */
					I2Cx->DR = (uint16_t)*(I2Ctmp[I2C_num].RXTX_Setup->TX_Data + I2Ctmp[I2C_num].RXTX_Setup->TX_Count++);
				}
				break;

			case I2C_SR1_BTF:
			case (I2C_SR1_TXE | I2C_SR1_BTF): /* Shift register and transmit register empty */
				/* All data has been sent, if needed send Restart and enable interrupts again
				 * and change the transfer direction */
				if ((I2Ctmp[I2C_num].RXTX_Setup->RX_Length != 0) && (I2Ctmp[I2C_num].RXTX_Setup->RX_Data != NULL))
				{
					I2Ctmp[I2C_num].Direction = I2C_RECEIVING;
					I2Cx->CR1 |= I2C_CR1_START;
					I2C_ITConfig(I2Cx, I2C_IT_BUF, ENABLE);
				}
				else
				{ 	/* No data to receive, end transmission */
					I2C_ITConfig(I2Cx, (I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR), DISABLE);
					I2Cx->CR1 |= I2C_CR1_STOP;

					/* If there is a callback function, run it */
					if (I2Ctmp[I2C_num].RXTX_Setup->Callback != NULL)
						I2Ctmp[I2C_num].RXTX_Setup->Callback();
				}
				break;

			default:
				break;
		}
	}

	/* *
	 *
	 * ----------------------------- RECIEVE PHASE -----------------------------
	 * The I2C read on the STM32F4xx is not symmetric so special
	 * care must be taken if the number of bytes to be read are
	 * one, two or more than two.
	 *
	 * */

	else
	{
		switch (status & I2C_STATUS_BITMASK)
		{
			case I2C_SR1_SB: /* Start/Restart condition sent */
send_slar:
				if ((I2Ctmp[I2C_num].RXTX_Setup->RX_Length != 0) && (I2Ctmp[I2C_num].RXTX_Setup->RX_Data != NULL))
				{ 	/* If there is data to receive, send Addr+R */
					I2Cx->DR = (uint16_t)((I2Ctmp[I2C_num].RXTX_Setup->Slave_Address_7bit << 1) | 0x01);
				}
				else /* Else go to end */
				{
					I2C_ITConfig(I2Cx, (I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR), DISABLE);
					I2Cx->CR1 |= I2C_CR1_STOP;

					/* If there is a callback function, run it */
					if (I2Ctmp[I2C_num].RXTX_Setup->Callback != NULL)
						I2Ctmp[I2C_num].RXTX_Setup->Callback();
				}
				break;

			case I2C_SR1_ADDR: /* Address+R sent, ack received */
				if (I2Ctmp[I2C_num].RXTX_Setup->RX_Length == 1)
				{	/* If there is only one byte to receive, reset ACK */
					I2Cx->CR1 &= ~I2C_CR1_ACK;
					/* Now Read the SR2 to clear ADDR */
					(void)I2Cx->SR2;
					/* Order a STOP condition, it shall be done after reading SR2 */
					I2Cx->CR1 |= I2C_CR1_STOP;
				}
				else if (I2Ctmp[I2C_num].RXTX_Setup->RX_Length == 2)
				{
					/* Now we shall wait for BTF so disable BUF interrupts */
					I2C_ITConfig(I2Cx, I2C_IT_BUF, DISABLE);

					/* If there is two bytes to receive, reset ACK, set POS */
					I2Cx->CR1 &= ~I2C_CR1_ACK;
					I2Cx->CR1 |= I2C_CR1_POS;

					(void)I2Cx->SR2; /* Read SR2 to start reading data */
				}
				else /* If there is more than two bytes to receive, just start shuffling data  */
				{
					(void)I2Cx->SR2; /* Read SR2 to start reading data */
				}
				break;

			case I2C_SR1_RXNE: /* Data in data register ready for read out shift register empty */
				if (I2Ctmp[I2C_num].RXTX_Setup->RX_Length == 1)
				{	/* One byte receive does not wait for BTF */
					I2C_ITConfig(I2Cx, (I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR), DISABLE);
					*(I2Ctmp[I2C_num].RXTX_Setup->RX_Data + I2Ctmp[I2C_num].RXTX_Setup->RX_Count++) = (uint8_t)I2Cx->DR;
				}
				else
				{
					if ((I2Ctmp[I2C_num].RXTX_Setup->RX_Length - I2Ctmp[I2C_num].RXTX_Setup->RX_Count) > 3)
					{ 	/* For as long as there are more than three bytes to receive, just read them out */
						*(I2Ctmp[I2C_num].RXTX_Setup->RX_Data + I2Ctmp[I2C_num].RXTX_Setup->RX_Count++) = (uint8_t)I2Cx->DR;
					}
					/* When there are three bytes left, disable RXNE interrupt */
					if ((I2Ctmp[I2C_num].RXTX_Setup->RX_Length - I2Ctmp[I2C_num].RXTX_Setup->RX_Count) == 3)
					{ 	/* 3 more bytes to read. Wait till the next is actually received (BTF = 1) */
						I2C_ITConfig(I2Cx, I2C_IT_BUF, DISABLE);
					}
				}
				break;

			case I2C_SR1_BTF:
			case (I2C_SR1_RXNE | I2C_SR1_BTF): /* Data in the data register and shift register */
				if (I2Ctmp[I2C_num].RXTX_Setup->RX_Length == 2)
				{	/* The next 2 bytes has been received (1st in the DR, 2nd in the shift register) */
					I2C_ITConfig(I2Cx, (I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR), DISABLE);

					/* Order a stop condition */
					I2Cx->CR1 |= I2C_CR1_STOP;

					/* Read the two bytes */
					*(I2Ctmp[I2C_num].RXTX_Setup->RX_Data + I2Ctmp[I2C_num].RXTX_Setup->RX_Count++) = (uint8_t)I2Cx->DR;
					*(I2Ctmp[I2C_num].RXTX_Setup->RX_Data + I2Ctmp[I2C_num].RXTX_Setup->RX_Count++) = (uint8_t)I2Cx->DR;

					/* If there is a callback function, run it */
					if (I2Ctmp[I2C_num].RXTX_Setup->Callback != NULL)
						I2Ctmp[I2C_num].RXTX_Setup->Callback();
				}
				else if (I2Ctmp[I2C_num].RXTX_Setup->RX_Length > 2) /* There is more than two bytes to recieve */
				{
					if ((I2Ctmp[I2C_num].RXTX_Setup->RX_Length - I2Ctmp[I2C_num].RXTX_Setup->RX_Count) == 3)
					{ 	/* 3 more bytes to read */
						/* Reset Ack */
						I2Cx->CR1 &= ~I2C_CR1_ACK;

						/* Read N-2 */
						*(I2Ctmp[I2C_num].RXTX_Setup->RX_Data + I2Ctmp[I2C_num].RXTX_Setup->RX_Count++) = (uint8_t)I2Cx->DR;
					}
					else if ((I2Ctmp[I2C_num].RXTX_Setup->RX_Length - I2Ctmp[I2C_num].RXTX_Setup->RX_Count) == 2)
					{ 	/* 2 more bytes to read */
						I2C_ITConfig(I2Cx, (I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR), DISABLE);

						/* Generate stop condition */
						I2Cx->CR1 |= I2C_CR1_STOP;
						/* Read the last two bytes (N-1 and N) */
						*(I2Ctmp[I2C_num].RXTX_Setup->RX_Data + I2Ctmp[I2C_num].RXTX_Setup->RX_Count++) = (uint8_t)I2Cx->DR;
						*(I2Ctmp[I2C_num].RXTX_Setup->RX_Data + I2Ctmp[I2C_num].RXTX_Setup->RX_Count++) = (uint8_t)I2Cx->DR;
						/* If there is a callback function, run it */
						if (I2Ctmp[I2C_num].RXTX_Setup->Callback != NULL)
							I2Ctmp[I2C_num].RXTX_Setup->Callback();
					}
				}
				break;

			default:
				break;
		}
	}
}

void I2C2_EV_IRQHandler(void)
{
	I2C_MasterHandler(I2C2);
}

void I2C2_ER_IRQHandler(void)
{
	I2C_MasterHandler(I2C2);
}

static uint16_t I2C_Addr(I2C_TypeDef *I2Cx, uint8_t DevAddr, uint8_t dir)
{
	/*  Write address to the DR */
	I2Cx->DR = (DevAddr << 1) | (dir & 0x01); /* Or in the lowest bit in dir */

	/* Wait till ADDR is set (ADDR is set when the slave sends ACK to the address). */
	/* Clock stretches till ADDR is Reset. To reset the hardware i)Read the SR1 ii)Wait till ADDR is Set iii)Read SR2 */
	/* Note1: Spec_p602 recommends the waiting operation */
	/* Note2: We don't read SR2 here. Therefore the clock is going to be stretched even after return from this function */
	return WaitSR1FlagsSet(I2Cx, I2C_SR1_ADDR);
}

static uint16_t I2C_SendByte(I2C_TypeDef *I2Cx, uint8_t data)
{
	/* Write data to the DR */
	I2Cx->DR = data;

	return WaitSR1FlagsSet(I2Cx, I2C_SR1_TXE);
}

static uint16_t I2C_Read(I2C_TypeDef *I2Cx, uint8_t *pBuf)
{
	uint32_t err;
    uint32_t TimeOut = HSI_VALUE;

	while(((I2Cx->SR1) & I2C_SR1_RXNE) != I2C_SR1_RXNE)
		if (!(TimeOut--))
			return ((I2Cx->SR1 & I2C_SR1_BITMASK) | I2C_ERROR_BIT);

	*pBuf = I2Cx->DR;   /* This clears the RXNE bit. IF both RXNE and BTF is set, the clock stretches */
	return (I2Cx->SR1 & I2C_SR1_BITMASK);

}

static uint16_t I2C_Start(I2C_TypeDef *I2Cx)
{
	/* Generate a start condition. (As soon as the line becomes idle, a Start condition will be generated) */
	I2Cx->CR1 |= I2C_CR1_START;

	/* When start condition is generated SB is set and clock is stretched. */
	/* To activate the clock again i)read SR1 ii)write something to DR (e.g. address) */
	return WaitSR1FlagsSet(I2Cx, I2C_SR1_SB);  /* Wait till SB is set */
}

volatile uint32_t TimeOut = 0;

static uint16_t WaitSR1FlagsSet(I2C_TypeDef *I2Cx, uint16_t Flags)
{
	/* Wait till the specified SR1 Bits are set */
	/* More than 1 Flag can be "or"ed. This routine reads only SR1. */
	TimeOut = HSI_VALUE/2;

	while(((I2Cx->SR1) & Flags) != Flags)
		if (!(TimeOut--))
			return ((I2Cx->SR1 & I2C_SR1_BITMASK) | I2C_ERROR_BIT); /* Set one of the reserved bits to signal timeout */

	return (I2Cx->SR1 & I2C_SR1_BITMASK);
}

static uint16_t WaitLineIdle(I2C_TypeDef *I2Cx)
{
	/* Wait till the Line becomes idle. */

	uint32_t TimeOut = HSI_VALUE;
	/* Check to see if the Line is busy */
	/* This bit is set automatically when a start condition is broadcasted on the line (even from another master) */
	/* and is reset when stop condition is detected. */
	while((I2Cx->SR2) & (I2C_SR2_BUSY))
		if (!(TimeOut--))
			return ((I2Cx->SR1 & I2C_SR1_BITMASK) | I2C_ERROR_BIT); /* Set one of the reserved bits to signal timeout */

	return (I2Cx->SR1 & I2C_SR1_BITMASK);
}
