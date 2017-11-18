/*
 * (C) Copyright 2017
 * Texas Instruments Incorporated, <www.ti.com>
 *
 * Venkateswara Rao Mandela <venkat.mandela@ti.com>
 *
 * SPDX-License-Identifier:	BSD-3-Clause
 */
#include "common.h"

typedef void __attribute__((noreturn)) (*mlo_func_proto)(uint32_t *);

#define	COUNTER32K_CR			(0x4AE04030)
#define	QSPI_MMAP_BASE			(0x5C000000)
#define	CM_L3MAIN1_TPCC_CLKCTRL		(0x4A008770)
#define	CM_L3MAIN1_TPTC1_CLKCTRL	(0x4A008778)
#define	CM_L3MAIN1_TPTC2_CLKCTRL	(0x4A008780)
#define CM_L4PER2_QSPI_CLKCTRL		(0x4A009838)
#define CTRL_CORE_CONTROL_IO_2		(0x4A002558)
#define EDMA_BASE			(0x43300000)
#define EDMA_PARAM_0			(0x43304000)
#define EDMA_TPCC_ESR			(0x43301010)
#define EDMA_TPCC_EESR			(0x43301030)
#define EDMA_TPCC_IPR			(0x43301068)
#define EDMA_TPCC_ICR			(0x43301070)

#define QSPI_CMD_READ_QUAD              (0x6c << 0)
#define QSPI_SETUP0_NUM_A_BYTES         (0x3 << 8)
#define QSPI_SETUP0_NUM_D_BYTES_8_BITS  (0x1 << 10)
#define QSPI_SETUP0_READ_QUAD           (0x3 << 12)
#define QSPI_CMD_WRITE                  (0x12 << 16)
#define QSPI_NUM_DUMMY_BITS             (0x0 << 24)
#define QSPI_SPI_CLOCK_CNTRL_REG	(0x4B300040)
#define QSPI_SPI_DC_REG			(0x4B300044)
#define QSPI_SPI_STATUS_REG		(0x4B30004C)
#define QSPI_SPI_SETUP0_REG		(0x4B300054)
#define QSPI_SPI_SWITCH_REG		(0x4B300064)
#define QSPI_SPI_CMD_REG		(0x4B300048)

#define CTRL_CORE_PAD_GPMC_A13 (0x4A003474)
#define CTRL_CORE_PAD_GPMC_A14 (0x4A003478)
#define CTRL_CORE_PAD_GPMC_A15 (0x4A00347C)
#define CTRL_CORE_PAD_GPMC_A16 (0x4A003480)
#define CTRL_CORE_PAD_GPMC_A17 (0x4A003484)
#define CTRL_CORE_PAD_GPMC_A18 (0x4A003488)
#define CTRL_CORE_PAD_GPMC_CS2 (0x4A0034B8)


#define CM_DIV_H13_DPLL_PER (0x4A008160)

struct edma_param_entry {
	/** OPT field of PaRAM Set */
	u32 opt;

	/**
	 * @brief Starting byte address of Source
	 * For FIFO mode, src_addr must be a 256-bit aligned address.
	 */
	u32 src_addr;

	/**
	 * @brief Number of bytes in each Array (ACNT)
	 */
	u16 a_cnt;

	/**
	 * @brief Number of Arrays in each Frame (BCNT)
	 */
	u16 b_cnt;

	/**
	 * @brief Starting byte address of destination
	 * For FIFO mode, dest_addr must be a 256-bit aligned address.
	 * i.e. 5 LSBs should be 0.
	 */
	u32 dest_addr;

	/**
	 * @brief Index between consec. arrays of a Source Frame (SRCBIDX)
	 */
	s16 src_bidx;

	/**
	 * @brief Index between consec. arrays of a Destination Frame (DSTBIDX)
	 */
	s16 dest_bidx;

	/**
	 * @brief Address for linking (AutoReloading of a PaRAM Set)
	 * This must point to a valid aligned 32-byte PaRAM set
	 * A value of 0xFFFF means no linking
	 */
	u16 link_addr;

	/**
	 * @brief Reload value of the numArrInFrame (BCNT)
	 * Relevant only for A-sync transfers
	 */
	u16 b_cnt_reload;

	/**
	 * @brief Index between consecutive frames of a Source Block (SRCCIDX)
	 */
	s16 src_cidx;

	/**
	 * @brief Index between consecutive frames of a Dest Block (DSTCIDX)
	 */
	s16 dest_cidx;

	/**
	 * @brief Number of Frames in a block (CCNT)
	 */
	u16 c_cnt;

	/**
	 * @brief reserved member.
	 */
	u16 rsv;
} __packed;
u32 edma_clkctrl[2] = {
	CM_L3MAIN1_TPTC1_CLKCTRL,
	CM_L3MAIN1_TPTC2_CLKCTRL,
};

volatile int ccs_dbg_flag = 1;
void wait_for_debugger(void)
{
	ccs_dbg_flag = 1;
	while (ccs_dbg_flag == 1) {
		asm(" NOP");
	}
	return;
}

static u32 read_fast_counter(void)
{
	u32 reg_val = (u32) reg_read16(COUNTER32K_CR);
	reg_val |= (u32)(reg_read16(COUNTER32K_CR+2) << 16);
	return reg_val;
}

static void wait_qspi_idle(void)
{
	u32 addr = QSPI_SPI_STATUS_REG;
	u32 val, reg;

	do {
		reg = reg_read32(addr);
		val = FLD_GET(reg, 0, 0);
	} while (val != 0);
}

u32 pinmux[7] = {
	CTRL_CORE_PAD_GPMC_A13,
	CTRL_CORE_PAD_GPMC_A14,
	CTRL_CORE_PAD_GPMC_A15,
	CTRL_CORE_PAD_GPMC_A16,
	CTRL_CORE_PAD_GPMC_A17,
	CTRL_CORE_PAD_GPMC_A18,
	CTRL_CORE_PAD_GPMC_CS2,
};
void set_qspi_pinmux(void)
{
	u32 i = 0;

	/* Match pinmux settings from ROM */
	for (i = 0; i < 7; i++) {
		reg_write32(pinmux[i], 0x40001);
	}
	reg_write32(pinmux[6], 0x60001);

}
void set_qspi_clock(void)
{
	/* set qspi clock to 76.8 MHz */
	u32 addr = CM_L4PER2_QSPI_CLKCTRL;
	u32 reg;
	u32 t;
	u32 div = 0;

	/* Reset QSPI CLKCTRL to default */
	reg_write32(addr, 0x0);

	/* Wait until QSPI is off */
	do {
		reg = reg_read32(addr);
		t = FLD_GET(reg, 17, 16);
	} while (t != 3);


	/* Set DPLL PER divider to 10 => output 76.8 MHz */
	reg = reg_read32(CM_DIV_H13_DPLL_PER);
	reg= FLD_MOD(reg, 10, 5, 0);
	reg_write32(CM_DIV_H13_DPLL_PER, reg);

	reg = 0x0;
#if 1
	/* Default Clock is from DPLL_PER H13*/
	reg = FLD_MOD(reg, 1, 24, 24);
	/* Set divider to 1 for interface clock of 76.8 MHz */
	reg = FLD_MOD(reg, 0, 26, 25);
#else
	/* Default Clock is from 128 MHz clock */
	reg = FLD_MOD(reg, 0, 24, 24);
	/* Set divider to 1 to for interface clock of 64 MHz */
	reg = FLD_MOD(reg, 1, 26, 25);
#endif
	/* Enable QSPI. */
	reg = FLD_MOD(reg, 2, 1, 0);
	reg_write32(addr, reg);
	do {
		reg = reg_read32(addr);
		t = FLD_GET(reg, 17, 16);
	} while (t != 0);

	wait_qspi_idle();

	/* turn off QSPI clock */
	reg = reg_read32(QSPI_SPI_CLOCK_CNTRL_REG);
	reg = FLD_MOD(reg, 0, 31, 31);
	reg_write32(QSPI_SPI_CLOCK_CNTRL_REG, reg);

	/* write divider */
	reg = FLD_MOD(reg, div, 15, 0);
	reg_write32(QSPI_SPI_CLOCK_CNTRL_REG, reg);

	/* enable qspi clock */
	reg = FLD_MOD(reg, 1, 31, 31);
	reg_write32(QSPI_SPI_CLOCK_CNTRL_REG, reg);

	/* Set the Mode for CS0 to mode 0 */
	addr = QSPI_SPI_DC_REG;
	reg = reg_read32(addr);
	reg = FLD_MOD(reg, 0, 4, 0);
	reg_write32(addr, reg);

	return;
}

void enable_qspi_mmap(void)
{
	u32 addr = CTRL_CORE_CONTROL_IO_2;
	u32 reg;


	/* Setup command for CS 0 */
	reg = (QSPI_CMD_READ_QUAD | QSPI_SETUP0_NUM_A_BYTES |
	       QSPI_SETUP0_NUM_D_BYTES_8_BITS |
	       QSPI_SETUP0_READ_QUAD | QSPI_CMD_WRITE |
	       QSPI_NUM_DUMMY_BITS);

	reg_write32(QSPI_SPI_SETUP0_REG, reg);

	reg_write32(QSPI_SPI_SWITCH_REG, 0x1);

	reg = reg_read32(addr);
	reg = FLD_MOD(reg, 1, 10, 8);
	reg_write32(addr, reg);
}

void disable_qspi_mmap(void)
{
	u32 addr = CTRL_CORE_CONTROL_IO_2;
	u32 reg;

	reg = reg_read32(addr);
	reg = FLD_MOD(reg, 0, 10, 8);
	reg_write32(addr, reg);
}

void enable_edma(void)
{
	u32 i = 0;
	for(i = 0; i < 2; i++) {
		u32 addr = edma_clkctrl[i];
		u32 reg;
		u32 t;

		reg = reg_read32(addr);
		reg = FLD_MOD(reg, 1, 1, 0);

		reg_write32(addr, reg);

		do {
			reg = reg_read32(addr);
			t = FLD_GET(reg, 17, 16);
		} while (t != 0);
	}

	/* Enable Channel 0 */
	reg_write32(EDMA_TPCC_EESR,0x1);
	/* Clear any pending interrupts for channel 0 */
	reg_write32(EDMA_TPCC_ICR,0x1);
}

void disable_edma(void)
{
	u32 i = 0;

	/* Clear any pending interrupts for channel 0 */
	reg_write32(EDMA_TPCC_ICR,0x1);
	for(i = 0; i < 2; i++) {
		u32 addr = edma_clkctrl[i];
		u32 reg;
		u32 t;

		reg = reg_read32(addr);
		reg = FLD_MOD(reg, 0, 1, 0);

		reg_write32(addr, reg);

		do {
			reg = reg_read32(addr);
			t = FLD_GET(reg, 17, 16);
		} while (t != 3);
	}
}

void edma_copy(u32 src, u32 dst, u32 len)
{
	u32 reg = 0;
	const u32 max_acnt = (1 << 4);
	struct edma_param_entry *edma_param;

	edma_param = (struct edma_param_entry *)EDMA_PARAM_0;
	edma_param->opt      = 0;
	edma_param->src_addr  = ((u32) src);
	edma_param->dest_addr = ((u32) dst);
	edma_param->a_cnt     = max_acnt;
	edma_param->b_cnt     = (len + max_acnt - 1) >> 4;
	edma_param->c_cnt     = 1;
	edma_param->src_bidx  = max_acnt;
	edma_param->dest_bidx = max_acnt;
	edma_param->src_cidx  = 0;
	edma_param->dest_cidx = 0;
	edma_param->link_addr = 0xFFFF;

	reg = 0;
	reg = FLD_MOD(reg, 1, 20, 20); /* TCINTEN */
	reg = FLD_MOD(reg, 1, 2, 2); /* ABSYNC */
	reg = FLD_MOD(reg, 1, 3, 3); /* STATIC */
	edma_param->opt = reg;
	reg_write32(EDMA_TPCC_ESR,0x01);
	do {
		reg = reg_read32(EDMA_TPCC_IPR);
	} while(FLD_GET(reg, 0 , 0) != 1);
	/* Clear any pending interrupts for channel 0 */
	reg_write32(EDMA_TPCC_ICR,0x1);
}

volatile u32 entry_cnt = 1;
volatile u32 exit_cnt = 1;
volatile u32 boot_addr_copy = 1;
void main_loop(uint32_t boot_addr)
{
	u32 len;
	u32 entry_point_addr;
	u32 src_addr;
	u32 base;
	mlo_func_proto mlo_entry;

	boot_addr_copy = boot_addr;
	/* find entry time */
	entry_cnt = read_fast_counter();
	enable_edma();
	set_qspi_pinmux();
	set_qspi_clock();
	enable_qspi_mmap();

	/* Find length */
	base = QSPI_MMAP_BASE + 0x10000 + 0x200;
	len = *((uint32_t *)(base));
	entry_point_addr = *((uint32_t *)(base + 4));
	src_addr = base + 8;
	edma_copy(src_addr, entry_point_addr, len - 8);
	mlo_entry = (mlo_func_proto)(entry_point_addr);

	disable_qspi_mmap();
	disable_edma();
	exit_cnt = read_fast_counter();

	/* Jump to the newly loaded image */
	mlo_entry((uint32_t *)boot_addr_copy);
}
