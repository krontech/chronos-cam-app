/****************************************************************************
 *  Copyright (C) 2013-2017 Kron Technologies Inc <http://www.krontech.ca>. *
 *                                                                          *
 *  This program is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/
#ifndef CAMERAREGISTERS_H
#define CAMERAREGISTERS_H

//Image sensor control register
#define	IMAGE_SENSOR_CONTROL_ADDR		0x000
#define	IMAGE_SENSOR_RESET_MASK				(1 << 0)
#define IMAGE_SENSOR_EVEN_TIMESLOT_MASK		(1 << 1)
#define IMAGE_SENSOR_CLK_PHASE_ADDR		0x004
#define IMAGE_SENSOR_SYNC_TOKEN_ADDR	0x008
#define IMAGE_SENSOR_DATA_CORRECT_ADDR	0x00C		// Bitmask of syncrhonized channels: data[11], data[10], ... data[0], sync
#define IMAGE_SENSOR_FIFO_START_ADDR	0x010		// Start write threshold
#define IMAGE_SENSOR_FIFO_STOP_ADDR		0x014		// Stop write threshold
#define	IMAGER_FRAME_PERIOD_ADDR		0x01C
#define IMAGER_INT_TIME_ADDR			0x020

#define	SENSOR_SCI_CONTROL_ADDR			0x024
#define SENSOR_SCI_CONTROL_RUN_MASK			(1 << 0)	//Write 1 to start, reads 1 while busy, 0 when done
#define SENSOR_SCI_CONTROL_RW_MASK			(1 << 1)	//Read == 1, Write == 0
#define SENSOR_SCI_CONTROL_FIFO_FULL_MASK	(1 << 2)	//1 indicates FIFO is full
#define	SENSOR_SCI_ADDRESS_ADDR			0x028
#define	SENSOR_SCI_DATALEN_ADDR			0x02C
#define SENSOR_SCI_FIFO_WR_ADDR_ADDR	0x030
#define SENSOR_SCI_READ_DATA_ADDR		0x034

//---------------------------------- Timing Sequencer Block --------------------------------------

#define SEQ_BASE_ADDR					0x040

#define	SEQ_CTL_ADDR					(SEQ_BASE_ADDR + 0x00)
#define	SEQ_CTL_SW_TRIG_MASK				(1 << 0)
#define SEQ_CTL_START_REC_MASK				(1 << 1)
#define	SEQ_CTL_STOP_REC_MASK				(1 << 2)
#define	SEQ_CTL_TRIG_DELAY_MODE_MASK		(1 << 3)

#define	SEQ_STATUS_ADDR					(SEQ_BASE_ADDR + 0x04)
#define	SEQ_STATUS_RECORDING_MASK			(1 << 0)
#define	SEQ_STATUS_MD_FIFO_EMPTY_MASK		(1 << 1)

#define	SEQ_FRAME_SIZE_ADDR				(SEQ_BASE_ADDR + 0x08)
#define	SEQ_REC_REGION_START_ADDR		(SEQ_BASE_ADDR + 0x0C)
#define	SEQ_REC_REGION_END_ADDR			(SEQ_BASE_ADDR + 0x10)
#define	SEQ_LIVE_ADDR_0_ADDR			(SEQ_BASE_ADDR + 0x14)
#define	SEQ_LIVE_ADDR_1_ADDR			(SEQ_BASE_ADDR + 0x18)
#define	SEQ_LIVE_ADDR_2_ADDR			(SEQ_BASE_ADDR + 0x1C)
#define	SEQ_TRIG_DELAY_ADDR				(SEQ_BASE_ADDR + 0x20)
#define	SEQ_MD_FIFO_READ_ADDR			(SEQ_BASE_ADDR + 0x24)
#define SENSOR_MAGIC_START_DELAY_ADDR	(SEQ_BASE_ADDR + 0x28)
#define	SENSOR_LINE_PERIOD_ADDR			(SEQ_BASE_ADDR + 0x2C)

#define	SEQ_PGM_MEM_START_ADDR			0x100

//---------------------------------- Trigger and IO Block ----------------------------------------

#define	TRIG_ENABLE_ADDR				0x0A0
#define	TRIG_INVERT_ADDR				0x0A4
#define	TRIG_DEBOUNCE_ADDR				0x0A8

#define	IO_OUT_LEVEL_ADDR				0x0AC		//1 outputs high if selected (invert does not affect this)
#define	IO_OUT_SOURCE_ADDR				0x0B0		//1 selects int time, 0 selects IO_OUT_LEVEL
#define	IO_OUT_INVERT_ADDR				0x0B4		//1 inverts int time signal
#define	IO_IN_ADDR						0x0B8		//1 inverts int time signal

#define	EXT_SHUTTER_CTL_ADDR			0x0BC
#define	EXT_SH_TRIGD_EXP_EN_MASK			(1 << 0)
#define	EXT_SH_GATING_EN_MASK				(1 << 1)
#define	EXT_SH_SRC_EN_MASK					0x1C
#define	EXT_SH_SRC_EN_OFFSET				2

//---------------------------------- Video Display Block -----------------------------------------

#define DISPLAY_BASE_ADDR				0x400

#define DISPLAY_CTL_ADDR				(DISPLAY_BASE_ADDR + 0x00)
#define DISPLAY_CTL_ADDRESS_SEL_MASK		(1 << 0)
#define DISPLAY_CTL_SCALER_NN_MASK			(1 << 1)
#define DISPLAY_CTL_SYNC_INH_MASK			(1 << 2)
#define DISPLAY_CTL_READOUT_INH_MASK		(1 << 3)
#define DISPLAY_CTL_COLOR_MODE_MASK			(1 << 4)
#define DISPLAY_CTL_FOCUS_PEAK_EN_MASK		(1 << 5)
#define DISPLAY_CTL_FOCUS_PEAK_COLOR_OFFSET	6
#define DISPLAY_CTL_FOCUS_PEAK_COLOR_MASK	(0x7 << DISPLAY_CTL_FOCUS_PEAK_COLOR_OFFSET)
#define DISPLAY_CTL_FOCUS_PEAK_COLOR_BLUE	(1 << DISPLAY_CTL_FOCUS_PEAK_COLOR_OFFSET)
#define DISPLAY_CTL_FOCUS_PEAK_COLOR_GREEN	(2 << DISPLAY_CTL_FOCUS_PEAK_COLOR_OFFSET)
#define DISPLAY_CTL_FOCUS_PEAK_COLOR_CYAN	(3 << DISPLAY_CTL_FOCUS_PEAK_COLOR_OFFSET)
#define DISPLAY_CTL_FOCUS_PEAK_COLOR_RED	(4 << DISPLAY_CTL_FOCUS_PEAK_COLOR_OFFSET)
#define DISPLAY_CTL_FOCUS_PEAK_COLOR_MAGENTA (5 << DISPLAY_CTL_FOCUS_PEAK_COLOR_OFFSET)
#define DISPLAY_CTL_FOCUS_PEAK_COLOR_YELLOW	(6 << DISPLAY_CTL_FOCUS_PEAK_COLOR_OFFSET)
#define DISPLAY_CTL_FOCUS_PEAK_COLOR_WHITE	(7 << DISPLAY_CTL_FOCUS_PEAK_COLOR_OFFSET)
#define DISPLAY_CTL_ZEBRA_EN_MASK			(1 << 9)

#define	DISPLAY_FRAME_ADDRESS_ADDR		(DISPLAY_BASE_ADDR + 0x04)
#define	DISPLAY_FPN_ADDRESS_ADDR		(DISPLAY_BASE_ADDR + 0x08)
#define DISPLAY_GAIN_ADDR				(DISPLAY_BASE_ADDR + 0x0C)
#define DISPLAY_H_PERIOD_ADDR			(DISPLAY_BASE_ADDR + 0x10)
#define DISPLAY_V_PERIOD_ADDR			(DISPLAY_BASE_ADDR + 0x14)
#define DISPLAY_H_SYNC_LEN_ADDR			(DISPLAY_BASE_ADDR + 0x18)
#define DISPLAY_V_SYNC_LEN_ADDR			(DISPLAY_BASE_ADDR + 0x1C)
#define DISPLAY_H_BACK_PORCH_ADDR		(DISPLAY_BASE_ADDR + 0x20)
#define DISPLAY_V_BACK_PORCH_ADDR		(DISPLAY_BASE_ADDR + 0x24)
#define DISPLAY_H_RES_ADDR				(DISPLAY_BASE_ADDR + 0x28)
#define DISPLAY_V_RES_ADDR				(DISPLAY_BASE_ADDR + 0x2C)
#define DISPLAY_H_OUT_RES_ADDR			(DISPLAY_BASE_ADDR + 0x30)
#define DISPLAY_V_OUT_RES_ADDR			(DISPLAY_BASE_ADDR + 0x40)
#define DISPLAY_PEAKING_THRESH_ADDR		(DISPLAY_BASE_ADDR + 0x44)

#define DISPLAY_PIPELINE_ADDR           (DISPLAY_BASE_ADDR + 0x48)
#define DISPLAY_PIPELINE_BIPASS_FPN         (1 << 0)
#define DISPLAY_PIPELINE_BIPASS_GAIN        (1 << 1)
#define DISPLAY_PIPELINE_BIPASS_DEMOSIAC    (1 << 2)
#define DISPLAY_PIPELINE_BIPASS_COLORMATRIX (1 << 3)
#define DISPLAY_PIPELINE_BIPASS_GAMMA_TABLE (1 << 4)
#define DISPLAY_PIPELINE_RAW_12BPP          (1 << 5)
#define DISPLAY_PIPELINE_RAW_16BPP          (1 << 6)
#define DISPLAY_PIPELINE_RAW_RIGHT_JUSTIFY  (1 << 7)
#define DISPLAY_PIPELINE_RAW_TEST_PATTERN   (1 << 15)

#define DISPLAY_MANUAL_SYNC_ADDR        (DISPLAY_BASE_ADDR + 0x50)
#define DISPLAY_MANUAL_SYNC_MASK			(1 << 0)

//---------------------------------- Color Correction Block --------------------------------------

#define COLOR_BASE_ADDR					0x4C0

#define CCM_11_ADDR						(COLOR_BASE_ADDR + 0x00)
#define CCM_12_ADDR						(COLOR_BASE_ADDR + 0x04)
#define CCM_13_ADDR						(COLOR_BASE_ADDR + 0x08)
#define CCM_21_ADDR						(COLOR_BASE_ADDR + 0x10)
#define CCM_22_ADDR						(COLOR_BASE_ADDR + 0x14)
#define CCM_23_ADDR						(COLOR_BASE_ADDR + 0x18)
#define CCM_31_ADDR						(COLOR_BASE_ADDR + 0x1C)
#define CCM_32_ADDR						(COLOR_BASE_ADDR + 0x20)
#define CCM_33_ADDR						(COLOR_BASE_ADDR + 0x24)

#define WBAL_RED_ADDR					(COLOR_BASE_ADDR + 0x30)
#define WBAL_GREEN_ADDR					(COLOR_BASE_ADDR + 0x34)
#define WBAL_BLUE_ADDR					(COLOR_BASE_ADDR + 0x38)

//---------------------------------- FPGA Miscellaneous Block ------------------------------------

#define GPMC_PAGE_OFFSET_ADDR			0x200

#define WL_DYNDLY_0_ADDR				0x500
#define WL_DYNDLY_1_ADDR				0x504
#define WL_DYNDLY_2_ADDR				0x408
#define WL_DYNDLY_3_ADDR				0x50C

#define MMU_CONFIG_ADDR                 0x520
#define MMU_INVERT_CS						(1 << 0)
#define MMU_SWITCH_STUFFED					(1 << 1)

#define SYSTEM_RESET_ADDR				0x600
#define FPGA_VERSION_ADDR				0x604
#define FPGA_SUBVERSION_ADDR            0x608

#define	COL_GAIN_MEM_START_ADDR			0x1000
#define COL_OFFSET_MEM_START_ADDR		0x5000
#define COL_CURVE_MEM_START_ADDR		0xD000

//========================================================================================================
//                                   New format registers

//---------------------------------- Wishbone RAM Access -----------------------------------------


#define RAM_REG_BASE                (0x2000)
#define RAM_REG_BLOCK               (RAM_REG_BASE)

// identifier
#define RAM_IDENTIFIER_REG          (RAM_REG_BLOCK + 0x00)
#define RAM_VERSION_REG             (RAM_REG_BLOCK + 0x04)
#define RAM_SUBVERSION_REG          (RAM_REG_BLOCK + 0x08)

// reset and control
#define RAM_CONTROL                 (RAM_REG_BLOCK + 0x0C)
#define RAM_STATUS                  (RAM_REG_BLOCK + 0x10)

#define RAM_ADDRESS                 (RAM_REG_BLOCK + 0x20) /* 32 bit */
#define RAM_BURST_LENGTH            (RAM_REG_BLOCK + 0x24)

// Read/write buffers
#define RAM_BUFFER_START            (RAM_REG_BLOCK + 0x200) /* 1024*16bit (64-ram page) */
#define RAM_BUFFER_END              (RAM_REG_BLOCK + 0xA00)



#define RAM_IDENTIFIER (0x0010)

#define RAM_CONTROL_TRIGGER_READ  0x0001
#define RAM_CONTROL_TRIGGER_WRITE 0x0002

#define RAM_BURST_LENGTH_MAX 32

//------------------------------------------------------------------------------------------------


#endif // CAMERAREGISTERS_H


