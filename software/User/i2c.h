#include <stdint.h>

#ifndef I2C_H
#define I2C_H

void I2C_Init_Bus(void);
uint8_t I2C_Probe(uint8_t addr);
uint8_t I2C_WriteReg(uint8_t addr, uint8_t reg, uint8_t val);
uint8_t I2C_ReadReg(uint8_t addr, uint8_t reg, uint8_t *out);
uint8_t I2C_ReadBurst(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);
void I2C_Stop_Bus(void);

#endif