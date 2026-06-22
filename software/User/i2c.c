#include "ch32v00X.h"
#include "i2c.h"

#define I2C_TIMEOUT   10000

void I2C_Init_Bus(void)
{
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOC, ENABLE);
    RCC_PB1PeriphClockCmd(RCC_PB1Periph_I2C1, ENABLE);

    GPIO_InitTypeDef g = {
        .GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_2,
        .GPIO_Mode  = GPIO_Mode_AF_OD,
        .GPIO_Speed = GPIO_Speed_30MHz,
    };
    GPIO_Init(GPIOC, &g);

    I2C_InitTypeDef i = {
        .I2C_ClockSpeed          = 100000,
        .I2C_Mode                = I2C_Mode_I2C,
        .I2C_DutyCycle           = I2C_DutyCycle_2,
        .I2C_OwnAddress1         = 0x00,
        .I2C_Ack                 = I2C_Ack_Enable,
        .I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit,
    };
    I2C_Init(I2C1, &i);
    I2C_Cmd(I2C1, ENABLE);
}

void I2C_Stop_Bus(void)
{
    I2C_Cmd(I2C1, DISABLE);
    RCC_PB1PeriphClockCmd(RCC_PB1Periph_I2C1, DISABLE);

    GPIO_InitTypeDef g = {
        .GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_2,
        .GPIO_Mode  = GPIO_Mode_IN_FLOATING,
        .GPIO_Speed = GPIO_Speed_30MHz,
    };
    GPIO_Init(GPIOC, &g);
}

uint8_t I2C_Probe(uint8_t addr)
{
    uint32_t t;
    t = I2C_TIMEOUT;
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) && --t);
    if (!t) return 1;

    I2C_GenerateSTART(I2C1, ENABLE);
    t = I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && --t);
    if (!t) return 1;

    I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Transmitter);
    t = I2C_TIMEOUT;
    while (t--) {
        if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
            I2C_GenerateSTOP(I2C1, ENABLE);
            return 0;
        }
        if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF)) {
            I2C_ClearFlag(I2C1, I2C_FLAG_AF);
            I2C_GenerateSTOP(I2C1, ENABLE);
            return 2;
        }
    }
    I2C_GenerateSTOP(I2C1, ENABLE);
    return 3;
}

uint8_t I2C_WriteReg(uint8_t addr, uint8_t reg, uint8_t val)
{
    uint32_t t;
    t = I2C_TIMEOUT;
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) && --t);
    if (!t) return 1;

    I2C_GenerateSTART(I2C1, ENABLE);
    t = I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && --t);
    if (!t) return 1;

    I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Transmitter);
    t = I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && --t);
    if (!t) return 1;

    I2C_SendData(I2C1, reg);
    t = I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && --t);
    if (!t) return 1;

    I2C_SendData(I2C1, val);
    t = I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && --t);
    if (!t) return 1;

    I2C_GenerateSTOP(I2C1, ENABLE);
    return 0;
}

uint8_t I2C_ReadReg(uint8_t addr, uint8_t reg, uint8_t *out)
{
    uint32_t t;
    t = I2C_TIMEOUT;
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) && --t);
    if (!t) return 1;

    I2C_GenerateSTART(I2C1, ENABLE);
    t = I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && --t);
    if (!t) return 1;

    I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Transmitter);
    t = I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && --t);
    if (!t) return 1;

    I2C_SendData(I2C1, reg);
    t = I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && --t);
    if (!t) return 1;

    I2C_GenerateSTART(I2C1, ENABLE);
    t = I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && --t);
    if (!t) return 1;

    I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Receiver);
    t = I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) && --t);
    if (!t) return 1;

    I2C_AcknowledgeConfig(I2C1, DISABLE);
    I2C_GenerateSTOP(I2C1, ENABLE);

    t = I2C_TIMEOUT;
    while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) && --t);
    if (!t) return 1;

    *out = I2C_ReceiveData(I2C1);
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    return 0;
}

uint8_t I2C_ReadBurst(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint32_t t;
    t = I2C_TIMEOUT;
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) && --t);
    if (!t) return 1;

    I2C_GenerateSTART(I2C1, ENABLE);
    t = I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && --t);
    if (!t) return 1;

    I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Transmitter);
    t = I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && --t);
    if (!t) return 1;

    I2C_SendData(I2C1, reg);
    t = I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && --t);
    if (!t) return 1;

    I2C_GenerateSTART(I2C1, ENABLE);
    t = I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && --t);
    if (!t) return 1;

    I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Receiver);
    t = I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) && --t);
    if (!t) return 1;

    for (uint8_t i = 0; i < len; i++) {
        if (i == len - 1) {
            I2C_AcknowledgeConfig(I2C1, DISABLE);
            I2C_GenerateSTOP(I2C1, ENABLE);
        }
        t = I2C_TIMEOUT;
        while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) && --t);
        if (!t) return 1;
        buf[i] = I2C_ReceiveData(I2C1);
    }
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    return 0;
}
