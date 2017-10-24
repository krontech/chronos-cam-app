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
#ifndef GPMCREGS_H
#define GPMCREGS_H

#define		GPMC_BASE						0x50000000

#define		GPMC_REVISION_OFFSET			0x0
#define		GPMC_SYSCONFIG_OFFSET			0x10
#define		GPMC_SYSSTATUS_OFFSET			0x14
#define		GPMC_IRQSTATUS_OFFSET			0x18
#define		GPMC_IRQENABLE_OFFSET			0x1C
#define		GPMC_TIMEOUT_CONTROL_OFFSET		0x40
#define		GPMC_ERR_ADDRESS_OFFSET			0x44
#define		GPMC_ERR_TYPE_OFFSET			0x48
#define		GPMC_CONFIG_OFFSET				0x50
#define		GPMC_STATUS_OFFSET				0x54

#define		GPMC_CONFIG1_i_OFFSET(i)		(0x60 + (i * 0x30))
#define		GPMC_CONFIG2_i_OFFSET(i)		(0x64 + (i * 0x30))
#define		GPMC_CONFIG3_i_OFFSET(i)		(0x68 + (i * 0x30))
#define		GPMC_CONFIG4_i_OFFSET(i)		(0x6C + (i * 0x30))
#define		GPMC_CONFIG5_i_OFFSET(i)		(0x70 + (i * 0x30))
#define		GPMC_CONFIG6_i_OFFSET(i)		(0x74 + (i * 0x30))
#define		GPMC_CONFIG7_i_OFFSET(i)		(0x78 + (i * 0x30))
#define		GPMC_NAND_COMMAND_i_OFFSET(i)	(0x7C + (i * 0x30))
#define		GPMC_ADDRESS_i_OFFSET(i)		(0x80 + (i * 0x30))
#define		GPMC_NAND_DATA_i_OFFSET(i)		(0x84 + (i * 0x30))

#define		GPMC_PREFETCH_CONFIG1_OFFSET	0x1E0
#define		GPMC_PREFETCH_CONFIG2_OFFSET	0x1E4
#define		GPMC_PREFETCH_CONTROL_OFFSET	0x1EC
#define		GPMC_PREFETCH_STATUS_OFFSET		0x1F0
#define		GPMC_ECC_CONFIG_OFFSET			0x1F4
#define		GPMC_ECC_CONTROL_OFFSET			0x1F8
#define		GPMC_ECC_SIZE_CONFIG_OFFSET		0x1FC

#define		GPMC_ECCj_RESULT_OFFSET(j)		(0x200 + (0x4 × j))
#define		GPMC_BCH_RESULT0_i_OFFSET(i)	(0x240 + (0x10 × i))
#define		GPMC_BCH_RESULT1_i_OFFSET(i)	(0x244 + (0x10 × i))
#define		GPMC_BCH_RESULT2_i_OFFSET(i)	(0x248 + (0x10 × i))
#define		GPMC_BCH_RESULT3_i_OFFSET(i)	(0x24C + (0x10 × i))

#define		GPMC_BCH_SWDATA_OFFSET			0x2D0

#define		GPMC_BCH_RESULT4_i_OFFSET(i)	(0x300 + (0x10 × i))
#define		GPMC_BCH_RESULT5_i_OFFSET(i)	(0x304 + (0x10 × i))
#define		GPMC_BCH_RESULT6_i_OFFSET(i)	(0x308 + (0x10 × i))

//-----------------------------------------------------------------------

#define		GPMC_REVISION			(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_REVISION_OFFSET)))
#define		GPMC_SYSCONFIG			(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_SYSCONFIG_OFFSET)))
#define		GPMC_SYSSTATUS			(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_SYSSTATUS_OFFSET)))
#define		GPMC_IRQSTATUS			(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_IRQSTATUS_OFFSET)))
#define		GPMC_IRQENABLE			(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_IRQENABLE_OFFSET)))
#define		GPMC_TIMEOUT_CONTROL	(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_TIMEOUT_CONTROL_OFFSET)))
#define		GPMC_ERR_ADDRESS		(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_ERR_ADDRESS_OFFSET)))
#define		GPMC_ERR_TYPE			(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_ERR_TYPE_OFFSET)))
#define		GPMC_CONFIG				(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_CONFIG_OFFSET)))
#define		GPMC_STATUS				(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_STATUS_OFFSET)))

#define		GPMC_CONFIG1_i(i)		(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_CONFIG1_i_OFFSET(i))))
#define		GPMC_CONFIG2_i(i)		(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_CONFIG2_i_OFFSET(i))))
#define		GPMC_CONFIG3_i(i)		(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_CONFIG3_i_OFFSET(i))))
#define		GPMC_CONFIG4_i(i)		(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_CONFIG4_i_OFFSET(i))))
#define		GPMC_CONFIG5_i(i)		(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_CONFIG5_i_OFFSET(i))))
#define		GPMC_CONFIG6_i(i)		(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_CONFIG6_i_OFFSET(i))))
#define		GPMC_CONFIG7_i(i)		(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_CONFIG7_i_OFFSET(i))))
#define		GPMC_NAND_COMMAND_i(i)	(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_NAND_COMMAND_i_OFFSET(i))))
#define		GPMC_ADDRESS_i(i)		(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_ADDRESS_i_OFFSET(i))))
#define		GPMC_NAND_DATA_i(i)		(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_NAND_DATA_i_OFFSET(i))))

#define		GPMC_PREFETCH_CONFIG1	(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_PREFETCH_CONFIG1_OFFSET)))
#define		GPMC_PREFETCH_CONFIG2	(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_PREFETCH_CONFIG2_OFFSET)))
#define		GPMC_PREFETCH_CONTROL	(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_PREFETCH_CONTROL_OFFSET)))
#define		GPMC_PREFETCH_STATUS	(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_PREFETCH_STATUS_OFFSET)))
#define		GPMC_ECC_CONFIG			(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_ECC_CONFIG_OFFSET)))
#define		GPMC_ECC_CONTROL		(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_ECC_CONTROL_OFFSET)))
#define		GPMC_ECC_SIZE_CONFIG	(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_ECC_SIZE_CONFIG_OFFSET)))

#define		GPMC_ECCj_RESULT(j)		(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_ECCj_RESULT_OFFSET(j))))
#define		GPMC_BCH_RESULT0_i(i)	(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_BCH_RESULT0_i_OFFSET(i))))
#define		GPMC_BCH_RESULT1_i(i)	(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_BCH_RESULT1_i_OFFSET(i))))
#define		GPMC_BCH_RESULT2_i(i)	(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_BCH_RESULT2_i_OFFSET(i))))
#define		GPMC_BCH_RESULT3_i(i)	(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_BCH_RESULT3_i_OFFSET(i))))

#define		GPMC_BCH_SWDATA			(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_BCH_SWDATA_OFFSET)))

#define		GPMC_BCH_RESULT4_i(i)	(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_BCH_RESULT4_i_OFFSET(i))))
#define		GPMC_BCH_RESULT5_i(i)	(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_BCH_RESULT5_i_OFFSET(i))))
#define		GPMC_BCH_RESULT6_i(i)	(*((unsigned int *)(GPMC_MAPPED_BASE + GPMC_BCH_RESULT6_i_OFFSET(i))))




#endif // GPMCREGS_H
