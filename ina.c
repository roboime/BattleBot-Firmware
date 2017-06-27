//
// ina.c
// Copyright (c) 2017 João Baptista de Paula e Silva
// Este arquivo está sob a licença MIT
//

//
// Este arquivo tem as funções para interfacear com o
// sensor de corrente
//

#include "default.h"

#define INA_CONFIG 0
#define INA_SHUNT_V 1
#define INA_BUS_V 2
#define INA_POWER 3
#define INA_CURRENT 4
#define INA_CALIBRATION 5
#define INA_NUM_REGS 6

#define ADDR B1000000

static volatile uint16_t reg_temps[INA_NUM_REGS];
static uint16_t regs[INA_NUM_REGS];
const uint8_t reg_addr[] = { 0, 1, 2, 3, 4, 5 };
uint8_t reg_cmds[INA_NUM_REGS];

#pragma pack(push, 1)
struct { uint8_t reg; uint16_t param; }
cfg_reg = { 0, 0 }, cal_reg = { 5, 0 };
#pragma pack(pop)

static void ina_command_read_register(uint8_t reg)
{
	twi_write(ADDR, &reg_addr[reg], sizeof(uint8_t));
	reg_cmds[reg] = twi_read(ADDR, &reg_temps[reg], sizeof(uint16_t));
}

static uint16_t ina_read_register_async(uint8_t reg)
{
	if (reg_cmds[reg] != -1 && twi_cmd_ready(reg_cmds[reg]))
	{
		regs[reg] = reg_temps[reg];
		reg_cmds[reg] = -1;
	}
		
	return regs[reg];
}

static uint16_t ina_read_register_sync(uint8_t reg)
{
	ina_command_read_register(reg);
	while (!twi_cmd_ready(reg_cmds[reg]));
	regs[reg] = reg_temps[reg];	
	return regs[reg];
}

static void ina_write_config_register(uint16_t param)
{
	cfg_reg.param = param;
	twi_write(ADDR, &cfg_reg, sizeof(cfg_reg));
}

static void ina_write_calibration_register(uint16_t param)
{
	cal_reg.param = param;
	twi_write(ADDR, &cal_reg, sizeof(cal_reg));
}

int16_t ina_get_shunt_voltage_10uv()
{
	return ina_read_register_sync(INA_SHUNT_V);
}

void ina_init()
{
	for (uint8_t i = 0; i < INA_NUM_REGS; i++)
	{
		reg_temps[i] = regs[i] = 0;
		reg_cmds[i] = -1;
	}
	
	ina_write_config_register(0x8000); // reseta o INA	
}
