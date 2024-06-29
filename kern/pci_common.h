/*	$NetBSD: pcireg.h,v 1.45 2004/02/04 06:58:24 soren Exp $	*/

/*
 * Copyright (c) 1995, 1996, 1999, 2000
 *     Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994, 1996 Charles M. Hannum.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_PCI_PCIREG_H_
#define	_DEV_PCI_PCIREG_H_

/*
 * Standardized PCI configuration information
 *
 * XXX This is not complete.
 */

#include <inc/types.h>

/*
 * Device identification register; contains a vendor ID and a device ID.
 */
#define	PCI_ID_REG			0x00

typedef uint16_t pci_vendor_id_t;
typedef uint16_t pci_product_id_t;

#define	PCI_VENDOR_SHIFT			0
#define	PCI_VENDOR_MASK				0xffff
#define	PCI_VENDOR(id) \
	    (((id) >> PCI_VENDOR_SHIFT) & PCI_VENDOR_MASK)

#define	PCI_PRODUCT_SHIFT			16
#define	PCI_PRODUCT_MASK			0xffff
#define	PCI_PRODUCT(id) \
	    (((id) >> PCI_PRODUCT_SHIFT) & PCI_PRODUCT_MASK)

#define PCI_ID_CODE(vid,pid)					\
	((((vid) & PCI_VENDOR_MASK) << PCI_VENDOR_SHIFT) |	\
	 (((pid) & PCI_PRODUCT_MASK) << PCI_PRODUCT_SHIFT))	\

/*
 * Command and status register.
 */
#define	PCI_COMMAND_STATUS_REG			0x04
#define	PCI_COMMAND_SHIFT			0
#define	PCI_COMMAND_MASK			0xffff
#define	PCI_STATUS_SHIFT			16
#define	PCI_STATUS_MASK				0xffff

#define PCI_COMMAND_STATUS_CODE(cmd,stat)			\
	((((cmd) & PCI_COMMAND_MASK) >> PCI_COMMAND_SHIFT) |	\
	 (((stat) & PCI_STATUS_MASK) >> PCI_STATUS_SHIFT))	\

#define	PCI_COMMAND_IO_ENABLE			0x00000001
#define	PCI_COMMAND_MEM_ENABLE			0x00000002
#define	PCI_COMMAND_MASTER_ENABLE		0x00000004
#define	PCI_COMMAND_SPECIAL_ENABLE		0x00000008
#define	PCI_COMMAND_INVALIDATE_ENABLE		0x00000010
#define	PCI_COMMAND_PALETTE_ENABLE		0x00000020
#define	PCI_COMMAND_PARITY_ENABLE		0x00000040
#define	PCI_COMMAND_STEPPING_ENABLE		0x00000080
#define	PCI_COMMAND_SERR_ENABLE			0x00000100
#define	PCI_COMMAND_BACKTOBACK_ENABLE		0x00000200

#define	PCI_STATUS_CAPLIST_SUPPORT		0x00100000
#define	PCI_STATUS_66MHZ_SUPPORT		0x00200000
#define	PCI_STATUS_UDF_SUPPORT			0x00400000
#define	PCI_STATUS_BACKTOBACK_SUPPORT		0x00800000
#define	PCI_STATUS_PARITY_ERROR			0x01000000
#define	PCI_STATUS_DEVSEL_FAST			0x00000000
#define	PCI_STATUS_DEVSEL_MEDIUM		0x02000000
#define	PCI_STATUS_DEVSEL_SLOW			0x04000000
#define	PCI_STATUS_DEVSEL_MASK			0x06000000
#define	PCI_STATUS_TARGET_TARGET_ABORT		0x08000000
#define	PCI_STATUS_MASTER_TARGET_ABORT		0x10000000
#define	PCI_STATUS_MASTER_ABORT			0x20000000
#define	PCI_STATUS_SPECIAL_ERROR		0x40000000
#define	PCI_STATUS_PARITY_DETECT		0x80000000

/*
 * PCI Class and Revision Register; defines type and revision of device.
 */
#define	PCI_CLASS_REG			0x08

typedef uint8_t pci_class_t;
typedef uint8_t pci_subclass_t;
typedef uint8_t pci_interface_t;
typedef uint8_t pci_revision_t;

#define	PCI_CLASS_SHIFT				24
#define	PCI_CLASS_MASK				0xff
#define	PCI_CLASS(cr) \
	    (((cr) >> PCI_CLASS_SHIFT) & PCI_CLASS_MASK)

#define	PCI_SUBCLASS_SHIFT			16
#define	PCI_SUBCLASS_MASK			0xff
#define	PCI_SUBCLASS(cr) \
	    (((cr) >> PCI_SUBCLASS_SHIFT) & PCI_SUBCLASS_MASK)

#define	PCI_INTERFACE_SHIFT			8
#define	PCI_INTERFACE_MASK			0xff
#define	PCI_INTERFACE(cr) \
	    (((cr) >> PCI_INTERFACE_SHIFT) & PCI_INTERFACE_MASK)

#define	PCI_REVISION_SHIFT			0
#define	PCI_REVISION_MASK			0xff
#define	PCI_REVISION(cr) \
	    (((cr) >> PCI_REVISION_SHIFT) & PCI_REVISION_MASK)

#define	PCI_CLASS_CODE(mainclass, subclass, interface) \
	    ((((mainclass) & PCI_CLASS_MASK) << PCI_CLASS_SHIFT) | \
	     (((subclass) & PCI_SUBCLASS_MASK) << PCI_SUBCLASS_SHIFT) | \
	     (((interface) & PCI_INTERFACE_MASK) << PCI_INTERFACE_SHIFT))

/*
 * PCI BIST/Header Type/Latency Timer/Cache Line Size Register.
 */
#define	PCI_BHLC_REG			0x0c

#define	PCI_BIST_SHIFT				24
#define	PCI_BIST_MASK				0xff
#define	PCI_BIST(bhlcr) \
	    (((bhlcr) >> PCI_BIST_SHIFT) & PCI_BIST_MASK)

#define	PCI_HDRTYPE_SHIFT			16
#define	PCI_HDRTYPE_MASK			0xff
#define	PCI_HDRTYPE(bhlcr) \
	    (((bhlcr) >> PCI_HDRTYPE_SHIFT) & PCI_HDRTYPE_MASK)

#define	PCI_HDRTYPE_TYPE(bhlcr) \
	    (PCI_HDRTYPE(bhlcr) & 0x7f)
#define	PCI_HDRTYPE_MULTIFN(bhlcr) \
	    ((PCI_HDRTYPE(bhlcr) & 0x80) != 0)

#define	PCI_LATTIMER_SHIFT			8
#define	PCI_LATTIMER_MASK			0xff
#define	PCI_LATTIMER(bhlcr) \
	    (((bhlcr) >> PCI_LATTIMER_SHIFT) & PCI_LATTIMER_MASK)

#define	PCI_CACHELINE_SHIFT			0
#define	PCI_CACHELINE_MASK			0xff
#define	PCI_CACHELINE(bhlcr) \
	    (((bhlcr) >> PCI_CACHELINE_SHIFT) & PCI_CACHELINE_MASK)

#define PCI_BHLC_CODE(bist,type,multi,latency,cacheline)		\
	    ((((bist) & PCI_BIST_MASK) << PCI_BIST_SHIFT) |		\
	     (((type) & PCI_HDRTYPE_MASK) << PCI_HDRTYPE_SHIFT) |	\
	     (((multi)?0x80:0) << PCI_HDRTYPE_SHIFT) |			\
	     (((latency) & PCI_LATTIMER_MASK) << PCI_LATTIMER_SHIFT) |	\
	     (((cacheline) & PCI_CACHELINE_MASK) << PCI_CACHELINE_SHIFT))

/*
 * PCI header type
 */
#define PCI_HDRTYPE_DEVICE	0
#define PCI_HDRTYPE_PPB		1
#define PCI_HDRTYPE_PCB		2

/*
 * Mapping registers
 */
#define	PCI_MAPREG_START		0x10
#define	PCI_MAPREG_END			0x28
#define	PCI_MAPREG_ROM			0x30
#define	PCI_MAPREG_PPB_END		0x18
#define	PCI_MAPREG_PCB_END		0x14

#define	PCI_MAPREG_TYPE(mr)						\
	    ((mr) & PCI_MAPREG_TYPE_MASK)
#define	PCI_MAPREG_TYPE_MASK			0x00000001

#define	PCI_MAPREG_TYPE_MEM			0x00000000
#define	PCI_MAPREG_TYPE_IO			0x00000001
#define	PCI_MAPREG_ROM_ENABLE			0x00000001

#define	PCI_MAPREG_MEM_TYPE(mr)						\
	    ((mr) & PCI_MAPREG_MEM_TYPE_MASK)
#define	PCI_MAPREG_MEM_TYPE_MASK		0x00000006

#define	PCI_MAPREG_MEM_TYPE_32BIT		0x00000000
#define	PCI_MAPREG_MEM_TYPE_32BIT_1M		0x00000002
#define	PCI_MAPREG_MEM_TYPE_64BIT		0x00000004

#define	PCI_MAPREG_MEM_PREFETCHABLE(mr)				\
	    (((mr) & PCI_MAPREG_MEM_PREFETCHABLE_MASK) != 0)
#define	PCI_MAPREG_MEM_PREFETCHABLE_MASK	0x00000008

#define	PCI_MAPREG_MEM_ADDR(mr)						\
	    ((mr) & PCI_MAPREG_MEM_ADDR_MASK)
#define	PCI_MAPREG_MEM_SIZE(mr)						\
	    (PCI_MAPREG_MEM_ADDR(mr) & -PCI_MAPREG_MEM_ADDR(mr))
#define	PCI_MAPREG_MEM_ADDR_MASK		0xfffffff0

#define	PCI_MAPREG_MEM64_ADDR(mr)					\
	    ((mr) & PCI_MAPREG_MEM64_ADDR_MASK)
#define	PCI_MAPREG_MEM64_SIZE(mr)					\
	    (PCI_MAPREG_MEM64_ADDR(mr) & -PCI_MAPREG_MEM64_ADDR(mr))
#define	PCI_MAPREG_MEM64_ADDR_MASK		0xfffffffffffffff0ULL

#define	PCI_MAPREG_IO_ADDR(mr)						\
	    ((mr) & PCI_MAPREG_IO_ADDR_MASK)
#define	PCI_MAPREG_IO_SIZE(mr)						\
	    (PCI_MAPREG_IO_ADDR(mr) & -PCI_MAPREG_IO_ADDR(mr))
#define	PCI_MAPREG_IO_ADDR_MASK			0xfffffffc

#define PCI_MAPREG_SIZE_TO_MASK(size)					\
	    (-(size))

#define PCI_MAPREG_NUM(offset)						\
	    (((unsigned)(offset)-PCI_MAPREG_START)/4)


/*
 * Cardbus CIS pointer (PCI rev. 2.1)
 */
#define PCI_CARDBUS_CIS_REG 0x28

/*
 * Subsystem identification register; contains a vendor ID and a device ID.
 * Types/macros for PCI_ID_REG apply.
 * (PCI rev. 2.1)
 */
#define PCI_SUBSYS_ID_REG 0x2c

/*
 * Capabilities link list (PCI rev. 2.2)
 */
#define	PCI_CAPLISTPTR_REG		0x34	/* header type 0 */
#define	PCI_CARDBUS_CAPLISTPTR_REG	0x14	/* header type 2 */
#define	PCI_CAPLIST_PTR(cpr)	((cpr) & 0xff)
#define	PCI_CAPLIST_NEXT(cr)	(((cr) >> 8) & 0xff)
#define	PCI_CAPLIST_CAP(cr)	((cr) & 0xff)

#define	PCI_CAP_RESERVED0	0x00
#define	PCI_CAP_PWRMGMT		0x01
#define	PCI_CAP_AGP		0x02
#define PCI_CAP_AGP_MAJOR(cr)	(((cr) >> 20) & 0xf)
#define PCI_CAP_AGP_MINOR(cr)	(((cr) >> 16) & 0xf)
#define	PCI_CAP_VPD		0x03
#define	PCI_CAP_SLOTID		0x04
#define	PCI_CAP_MSI		0x05
#define	PCI_CAP_CPCI_HOTSWAP	0x06
#define	PCI_CAP_PCIX		0x07
#define	PCI_CAP_LDT		0x08
#define	PCI_CAP_VENDSPEC	0x09
#define	PCI_CAP_DEBUGPORT	0x0a
#define	PCI_CAP_CPCI_RSRCCTL	0x0b
#define	PCI_CAP_HOTPLUG		0x0c
#define	PCI_CAP_AGP8		0x0e
#define	PCI_CAP_SECURE		0x0f
#define	PCI_CAP_PCIEXPRESS     	0x10
#define	PCI_CAP_MSIX		0x11

/*
 * Vital Product Data; access via capability pointer (PCI rev 2.2).
 */
#define	PCI_VPD_ADDRESS_MASK	0x7fff
#define	PCI_VPD_ADDRESS_SHIFT	16
#define	PCI_VPD_ADDRESS(ofs)	\
	(((ofs) & PCI_VPD_ADDRESS_MASK) << PCI_VPD_ADDRESS_SHIFT)
#define	PCI_VPD_DATAREG(ofs)	((ofs) + 4)
#define	PCI_VPD_OPFLAG		0x80000000

/*
 * Power Management Capability; access via capability pointer.
 */

/* Power Management Capability Register */
#define PCI_PMCR		0x02
#define PCI_PMCR_D1SUPP		0x0200
#define PCI_PMCR_D2SUPP		0x0400
/* Power Management Control Status Register */
#define PCI_PMCSR		0x04
#define PCI_PMCSR_STATE_MASK	0x03
#define PCI_PMCSR_STATE_D0      0x00
#define PCI_PMCSR_STATE_D1      0x01
#define PCI_PMCSR_STATE_D2      0x02
#define PCI_PMCSR_STATE_D3      0x03

/*
 * PCI-X capability.
 */

/*
 * Command. 16 bits at offset 2 (e.g. upper 16 bits of the first 32-bit
 * word at the capability; the lower 16 bits are the capability ID and
 * next capability pointer).
 *
 * Since we always read PCI config space in 32-bit words, we define these
 * as 32-bit values, offset and shifted appropriately.  Make sure you perform
 * the appropriate R/M/W cycles!
 */
#define PCI_PCIX_CMD			0x00
#define PCI_PCIX_CMD_PERR_RECOVER	0x00010000
#define PCI_PCIX_CMD_RELAXED_ORDER	0x00020000
#define PCI_PCIX_CMD_BYTECNT_MASK	0x000c0000
#define	PCI_PCIX_CMD_BYTECNT_SHIFT	18
#define		PCI_PCIX_CMD_BCNT_512		0x00000000
#define		PCI_PCIX_CMD_BCNT_1024		0x00040000
#define		PCI_PCIX_CMD_BCNT_2048		0x00080000
#define		PCI_PCIX_CMD_BCNT_4096		0x000c0000
#define PCI_PCIX_CMD_SPLTRANS_MASK	0x00700000
#define		PCI_PCIX_CMD_SPLTRANS_1		0x00000000
#define		PCI_PCIX_CMD_SPLTRANS_2		0x00100000
#define		PCI_PCIX_CMD_SPLTRANS_3		0x00200000
#define		PCI_PCIX_CMD_SPLTRANS_4		0x00300000
#define		PCI_PCIX_CMD_SPLTRANS_8		0x00400000
#define		PCI_PCIX_CMD_SPLTRANS_12	0x00500000
#define		PCI_PCIX_CMD_SPLTRANS_16	0x00600000
#define		PCI_PCIX_CMD_SPLTRANS_32	0x00700000

/*
 * Status. 32 bits at offset 4.
 */
#define PCI_PCIX_STATUS			0x04
#define PCI_PCIX_STATUS_FN_MASK		0x00000007
#define PCI_PCIX_STATUS_DEV_MASK	0x000000f8
#define PCI_PCIX_STATUS_BUS_MASK	0x0000ff00
#define PCI_PCIX_STATUS_64BIT		0x00010000
#define PCI_PCIX_STATUS_133		0x00020000
#define PCI_PCIX_STATUS_SPLDISC		0x00040000
#define PCI_PCIX_STATUS_SPLUNEX		0x00080000
#define PCI_PCIX_STATUS_DEVCPLX		0x00100000
#define PCI_PCIX_STATUS_MAXB_MASK	0x00600000
#define	PCI_PCIX_STATUS_MAXB_SHIFT	21
#define		PCI_PCIX_STATUS_MAXB_512	0x00000000
#define		PCI_PCIX_STATUS_MAXB_1024	0x00200000
#define		PCI_PCIX_STATUS_MAXB_2048	0x00400000
#define		PCI_PCIX_STATUS_MAXB_4096	0x00600000
#define PCI_PCIX_STATUS_MAXST_MASK	0x03800000
#define		PCI_PCIX_STATUS_MAXST_1		0x00000000
#define		PCI_PCIX_STATUS_MAXST_2		0x00800000
#define		PCI_PCIX_STATUS_MAXST_3		0x01000000
#define		PCI_PCIX_STATUS_MAXST_4		0x01800000
#define		PCI_PCIX_STATUS_MAXST_8		0x02000000
#define		PCI_PCIX_STATUS_MAXST_12	0x02800000
#define		PCI_PCIX_STATUS_MAXST_16	0x03000000
#define		PCI_PCIX_STATUS_MAXST_32	0x03800000
#define PCI_PCIX_STATUS_MAXRS_MASK	0x1c000000
#define		PCI_PCIX_STATUS_MAXRS_1K	0x00000000
#define		PCI_PCIX_STATUS_MAXRS_2K	0x04000000
#define		PCI_PCIX_STATUS_MAXRS_4K	0x08000000
#define		PCI_PCIX_STATUS_MAXRS_8K	0x0c000000
#define		PCI_PCIX_STATUS_MAXRS_16K	0x10000000
#define		PCI_PCIX_STATUS_MAXRS_32K	0x14000000
#define		PCI_PCIX_STATUS_MAXRS_64K	0x18000000
#define		PCI_PCIX_STATUS_MAXRS_128K	0x1c000000
#define PCI_PCIX_STATUS_SCERR			0x20000000


/*
 * Interrupt Configuration Register; contains interrupt pin and line.
 */
#define	PCI_INTERRUPT_REG		0x3c

typedef uint8_t pci_intr_latency_t;
typedef uint8_t pci_intr_grant_t;
typedef uint8_t pci_intr_pin_t;
typedef uint8_t pci_intr_line_t;

#define PCI_MAX_LAT_SHIFT			24
#define	PCI_MAX_LAT_MASK			0xff
#define	PCI_MAX_LAT(icr) \
	    (((icr) >> PCI_MAX_LAT_SHIFT) & PCI_MAX_LAT_MASK)

#define PCI_MIN_GNT_SHIFT			16
#define	PCI_MIN_GNT_MASK			0xff
#define	PCI_MIN_GNT(icr) \
	    (((icr) >> PCI_MIN_GNT_SHIFT) & PCI_MIN_GNT_MASK)

#define	PCI_INTERRUPT_GRANT_SHIFT		24
#define	PCI_INTERRUPT_GRANT_MASK		0xff
#define	PCI_INTERRUPT_GRANT(icr) \
	    (((icr) >> PCI_INTERRUPT_GRANT_SHIFT) & PCI_INTERRUPT_GRANT_MASK)

#define	PCI_INTERRUPT_LATENCY_SHIFT		16
#define	PCI_INTERRUPT_LATENCY_MASK		0xff
#define	PCI_INTERRUPT_LATENCY(icr) \
	    (((icr) >> PCI_INTERRUPT_LATENCY_SHIFT) & PCI_INTERRUPT_LATENCY_MASK)

#define	PCI_INTERRUPT_PIN_SHIFT			8
#define	PCI_INTERRUPT_PIN_MASK			0xff
#define	PCI_INTERRUPT_PIN(icr) \
	    (((icr) >> PCI_INTERRUPT_PIN_SHIFT) & PCI_INTERRUPT_PIN_MASK)

#define	PCI_INTERRUPT_LINE_SHIFT		0
#define	PCI_INTERRUPT_LINE_MASK			0xff
#define	PCI_INTERRUPT_LINE(icr) \
	    (((icr) >> PCI_INTERRUPT_LINE_SHIFT) & PCI_INTERRUPT_LINE_MASK)

#define PCI_INTERRUPT_CODE(lat,gnt,pin,line)		\
	  ((((lat)&PCI_INTERRUPT_LATENCY_MASK)<<PCI_INTERRUPT_LATENCY_SHIFT)| \
	   (((gnt)&PCI_INTERRUPT_GRANT_MASK)  <<PCI_INTERRUPT_GRANT_SHIFT)  | \
	   (((pin)&PCI_INTERRUPT_PIN_MASK)    <<PCI_INTERRUPT_PIN_SHIFT)    | \
	   (((line)&PCI_INTERRUPT_LINE_MASK)  <<PCI_INTERRUPT_LINE_SHIFT))

#define	PCI_INTERRUPT_PIN_NONE			0x00
#define	PCI_INTERRUPT_PIN_A			0x01
#define	PCI_INTERRUPT_PIN_B			0x02
#define	PCI_INTERRUPT_PIN_C			0x03
#define	PCI_INTERRUPT_PIN_D			0x04
#define	PCI_INTERRUPT_PIN_MAX			0x04

/* Header Type 1 (Bridge) configuration registers */
#define PCI_BRIDGE_BUS_REG		0x18
#define   PCI_BRIDGE_BUS_PRIMARY_SHIFT		0
#define   PCI_BRIDGE_BUS_SECONDARY_SHIFT	8
#define   PCI_BRIDGE_BUS_SUBORDINATE_SHIFT	16

#define PCI_BRIDGE_STATIO_REG		0x1C
#define	  PCI_BRIDGE_STATIO_IOBASE_SHIFT	0
#define	  PCI_BRIDGE_STATIO_IOLIMIT_SHIFT	8
#define	  PCI_BRIDGE_STATIO_STATUS_SHIFT	16
#define	  PCI_BRIDGE_STATIO_IOBASE_MASK		0xf0
#define	  PCI_BRIDGE_STATIO_IOLIMIT_MASK	0xf0
#define	  PCI_BRIDGE_STATIO_STATUS_MASK		0xffff
#define	  PCI_BRIDGE_IO_32BITS(reg)		(((reg) & 0xf) == 1)

#define PCI_BRIDGE_MEMORY_REG		0x20
#define	  PCI_BRIDGE_MEMORY_BASE_SHIFT		4
#define	  PCI_BRIDGE_MEMORY_LIMIT_SHIFT		20
#define	  PCI_BRIDGE_MEMORY_BASE_MASK		0xffff
#define	  PCI_BRIDGE_MEMORY_LIMIT_MASK		0xffff

#define PCI_BRIDGE_PREFETCHMEM_REG	0x24
#define	  PCI_BRIDGE_PREFETCHMEM_BASE_SHIFT	4
#define	  PCI_BRIDGE_PREFETCHMEM_LIMIT_SHIFT	20
#define	  PCI_BRIDGE_PREFETCHMEM_BASE_MASK	0xffff
#define	  PCI_BRIDGE_PREFETCHMEM_LIMIT_MASK	0xffff
#define	  PCI_BRIDGE_PREFETCHMEM_64BITS(reg)	((reg) & 0xf)

#define PCI_BRIDGE_PREFETCHBASE32_REG	0x28
#define PCI_BRIDGE_PREFETCHLIMIT32_REG	0x2C

#define PCI_BRIDGE_IOHIGH_REG		0x30
#define	  PCI_BRIDGE_IOHIGH_BASE_SHIFT		0
#define	  PCI_BRIDGE_IOHIGH_LIMIT_SHIFT		16
#define	  PCI_BRIDGE_IOHIGH_BASE_MASK		0xffff
#define	  PCI_BRIDGE_IOHIGH_LIMIT_MASK		0xffff

#define PCI_BRIDGE_CONTROL_REG		0x3C
#define	  PCI_BRIDGE_CONTROL_SHIFT		16
#define	  PCI_BRIDGE_CONTROL_MASK		0xffff
#define   PCI_BRIDGE_CONTROL_PERE		(1 <<  0)
#define   PCI_BRIDGE_CONTROL_SERR		(1 <<  1)
#define   PCI_BRIDGE_CONTROL_ISA		(1 <<  2)
#define   PCI_BRIDGE_CONTROL_VGA		(1 <<  3)
/* Reserved					(1 <<  4) */
#define   PCI_BRIDGE_CONTROL_MABRT		(1 <<  5)
#define   PCI_BRIDGE_CONTROL_SECBR		(1 <<  6)
#define   PCI_BRIDGE_CONTROL_SECFASTB2B		(1 <<  7)
#define   PCI_BRIDGE_CONTROL_PRI_DISC_TIMER	(1 <<  8)
#define   PCI_BRIDGE_CONTROL_SEC_DISC_TIMER	(1 <<  9)
#define   PCI_BRIDGE_CONTROL_DISC_TIMER_STAT	(1 << 10)
#define   PCI_BRIDGE_CONTROL_DISC_TIMER_SERR	(1 << 11)
/* Reserved					(1 << 12) - (1 << 15) */

/*
 * Vital Product Data resource tags.
 */
struct pci_vpd_smallres {
	uint8_t		vpdres_byte0;		/* length of data + tag */
	/* Actual data. */
} __attribute__((__packed__));

struct pci_vpd_largeres {
	uint8_t		vpdres_byte0;
	uint8_t		vpdres_len_lsb;		/* length of data only */
	uint8_t		vpdres_len_msb;
	/* Actual data. */
} __attribute__((__packed__));

#define	PCI_VPDRES_ISLARGE(x)			((x) & 0x80)

#define	PCI_VPDRES_SMALL_LENGTH(x)		((x) & 0x7)
#define	PCI_VPDRES_SMALL_NAME(x)		(((x) >> 3) & 0xf)

#define	PCI_VPDRES_LARGE_NAME(x)		((x) & 0x7f)

#define	PCI_VPDRES_TYPE_COMPATIBLE_DEVICE_ID	0x3	/* small */
#define	PCI_VPDRES_TYPE_VENDOR_DEFINED		0xe	/* small */
#define	PCI_VPDRES_TYPE_END_TAG			0xf	/* small */

#define	PCI_VPDRES_TYPE_IDENTIFIER_STRING	0x02	/* large */
#define	PCI_VPDRES_TYPE_VPD			0x10	/* large */

struct pci_vpd {
	uint8_t		vpd_key0;
	uint8_t		vpd_key1;
	uint8_t		vpd_len;		/* length of data only */
	/* Actual data. */
} __attribute__((__packed__));

/*
 * Recommended VPD fields:
 *
 *	PN		Part number of assembly
 *	FN		FRU part number
 *	EC		EC level of assembly
 *	MN		Manufacture ID
 *	SN		Serial Number
 *
 * Conditionally recommended VPD fields:
 *
 *	LI		Load ID
 *	RL		ROM Level
 *	RM		Alterable ROM Level
 *	NA		Network Address
 *	DD		Device Driver Level
 *	DG		Diagnostic Level
 *	LL		Loadable Microcode Level
 *	VI		Vendor ID/Device ID
 *	FU		Function Number
 *	SI		Subsystem Vendor ID/Subsystem ID
 *
 * Additional VPD fields:
 *
 *	Z0-ZZ		User/Product Specific
 */

/*
 * Threshold below which 32bit PCI DMA needs bouncing.
 */
#define PCI32_DMA_BOUNCE_THRESHOLD	0x100000000ULL

// THIS FILE IS AUTOMATICALLY GENERATED
// from kernel/device/pci.bits

//
// PCI_ID: Identifiers
#define PCI_ID               0x00


// Device ID
#define PCI_ID_DID_BIT       16

// Vendor ID
#define PCI_ID_VID_BIT       0


// Device ID
#define PCI_ID_DID_BITS      16

// Vendor ID
#define PCI_ID_VID_BITS      16

// Device ID
#define PCI_ID_DID_MASK      ((1U << PCI_ID_DID_BITS)-1)

// Vendor ID
#define PCI_ID_VID_MASK      ((1U << PCI_ID_VID_BITS)-1)

// Device ID
#define PCI_ID_DID           (PCI_ID_DID_MASK << PCI_ID_DID_BIT)

// Vendor ID
#define PCI_ID_VID           (PCI_ID_VID_MASK << PCI_ID_VID_BIT)


// Device ID
#define PCI_ID_DID_n(n)      ((n) << PCI_ID_DID_BIT)

// Vendor ID
#define PCI_ID_VID_n(n)      ((n) << PCI_ID_VID_BIT)


// Device ID
#define PCI_ID_DID_GET(n)    (((n) >> PCI_ID_DID_BIT) & PCI_ID_DID_MASK)

// Vendor ID
#define PCI_ID_VID_GET(n)    (((n) >> PCI_ID_VID_BIT) & PCI_ID_VID_MASK)


// Device ID
#define PCI_ID_DID_SET(r,n)  ((r) = ((r) & ~PCI_ID_DID) | PCI_ID_DID_n((n)))

// Vendor ID
#define PCI_ID_VID_SET(r,n)  ((r) = ((r) & ~PCI_ID_VID) | PCI_ID_VID_n((n)))

//
// PCI_CMD: Command
#define PCI_CMD                0x04


// Interrupt pin disable (does not affect MSI)
#define PCI_CMD_ID_BIT         10

// Fast back-to-back enable
#define PCI_CMD_FBE_BIT        9

// SERR# Enable
#define PCI_CMD_SEE_BIT        8

// Parity error response enable
#define PCI_CMD_PEE_BIT        6

// VGA palette snooping enable
#define PCI_CMD_VGA_BIT        5

// Memory write and invalidate enable
#define PCI_CMD_MWIE_BIT       4

// Special cycle enable
#define PCI_CMD_SCE_BIT        3

// Bus master enable
#define PCI_CMD_BME_BIT        2

// Memory space enable
#define PCI_CMD_MSE_BIT        1

// I/O space enable
#define PCI_CMD_IOSE_BIT       0


// Interrupt pin disable (does not affect MSI)
#define PCI_CMD_ID_BITS        1

// Fast back-to-back enable
#define PCI_CMD_FBE_BITS       1

// SERR# Enable
#define PCI_CMD_SEE_BITS       1

// Parity error response enable
#define PCI_CMD_PEE_BITS       1

// VGA palette snooping enable
#define PCI_CMD_VGA_BITS       1

// Memory write and invalidate enable
#define PCI_CMD_MWIE_BITS      1

// Special cycle enable
#define PCI_CMD_SCE_BITS       1

// Bus master enable
#define PCI_CMD_BME_BITS       1

// Memory space enable
#define PCI_CMD_MSE_BITS       1

// I/O space enable
#define PCI_CMD_IOSE_BITS      1

// Interrupt pin disable (does not affect MSI)
#define PCI_CMD_ID_MASK        ((1U << PCI_CMD_ID_BITS)-1)

// Fast back-to-back enable
#define PCI_CMD_FBE_MASK       ((1U << PCI_CMD_FBE_BITS)-1)

// SERR# Enable
#define PCI_CMD_SEE_MASK       ((1U << PCI_CMD_SEE_BITS)-1)

// Parity error response enable
#define PCI_CMD_PEE_MASK       ((1U << PCI_CMD_PEE_BITS)-1)

// VGA palette snooping enable
#define PCI_CMD_VGA_MASK       ((1U << PCI_CMD_VGA_BITS)-1)

// Memory write and invalidate enable
#define PCI_CMD_MWIE_MASK      ((1U << PCI_CMD_MWIE_BITS)-1)

// Special cycle enable
#define PCI_CMD_SCE_MASK       ((1U << PCI_CMD_SCE_BITS)-1)

// Bus master enable
#define PCI_CMD_BME_MASK       ((1U << PCI_CMD_BME_BITS)-1)

// Memory space enable
#define PCI_CMD_MSE_MASK       ((1U << PCI_CMD_MSE_BITS)-1)

// I/O space enable
#define PCI_CMD_IOSE_MASK      ((1U << PCI_CMD_IOSE_BITS)-1)

// Interrupt pin disable (does not affect MSI)
#define PCI_CMD_ID             (PCI_CMD_ID_MASK << PCI_CMD_ID_BIT)

// Fast back-to-back enable
#define PCI_CMD_FBE            (PCI_CMD_FBE_MASK << PCI_CMD_FBE_BIT)

// SERR# Enable
#define PCI_CMD_SEE            (PCI_CMD_SEE_MASK << PCI_CMD_SEE_BIT)

// Parity error response enable
#define PCI_CMD_PEE            (PCI_CMD_PEE_MASK << PCI_CMD_PEE_BIT)

// VGA palette snooping enable
#define PCI_CMD_VGA            (PCI_CMD_VGA_MASK << PCI_CMD_VGA_BIT)

// Memory write and invalidate enable
#define PCI_CMD_MWIE           (PCI_CMD_MWIE_MASK << PCI_CMD_MWIE_BIT)

// Special cycle enable
#define PCI_CMD_SCE            (PCI_CMD_SCE_MASK << PCI_CMD_SCE_BIT)

// Bus master enable
#define PCI_CMD_BME            (PCI_CMD_BME_MASK << PCI_CMD_BME_BIT)

// Memory space enable
#define PCI_CMD_MSE            (PCI_CMD_MSE_MASK << PCI_CMD_MSE_BIT)

// I/O space enable
#define PCI_CMD_IOSE           (PCI_CMD_IOSE_MASK << PCI_CMD_IOSE_BIT)


// Interrupt pin disable (does not affect MSI)
#define PCI_CMD_ID_n(n)        ((n) << PCI_CMD_ID_BIT)

// Fast back-to-back enable
#define PCI_CMD_FBE_n(n)       ((n) << PCI_CMD_FBE_BIT)

// SERR# Enable
#define PCI_CMD_SEE_n(n)       ((n) << PCI_CMD_SEE_BIT)

// Parity error response enable
#define PCI_CMD_PEE_n(n)       ((n) << PCI_CMD_PEE_BIT)

// VGA palette snooping enable
#define PCI_CMD_VGA_n(n)       ((n) << PCI_CMD_VGA_BIT)

// Memory write and invalidate enable
#define PCI_CMD_MWIE_n(n)      ((n) << PCI_CMD_MWIE_BIT)

// Special cycle enable
#define PCI_CMD_SCE_n(n)       ((n) << PCI_CMD_SCE_BIT)

// Bus master enable
#define PCI_CMD_BME_n(n)       ((n) << PCI_CMD_BME_BIT)

// Memory space enable
#define PCI_CMD_MSE_n(n)       ((n) << PCI_CMD_MSE_BIT)

// I/O space enable
#define PCI_CMD_IOSE_n(n)      ((n) << PCI_CMD_IOSE_BIT)


// Interrupt pin disable (does not affect MSI)
#define PCI_CMD_ID_GET(n)      (((n) >> PCI_CMD_ID_BIT) & PCI_CMD_ID_MASK)

// Fast back-to-back enable
#define PCI_CMD_FBE_GET(n)     (((n) >> PCI_CMD_FBE_BIT) & PCI_CMD_FBE_MASK)

// SERR# Enable
#define PCI_CMD_SEE_GET(n)     (((n) >> PCI_CMD_SEE_BIT) & PCI_CMD_SEE_MASK)

// Parity error response enable
#define PCI_CMD_PEE_GET(n)     (((n) >> PCI_CMD_PEE_BIT) & PCI_CMD_PEE_MASK)

// VGA palette snooping enable
#define PCI_CMD_VGA_GET(n)     (((n) >> PCI_CMD_VGA_BIT) & PCI_CMD_VGA_MASK)

// Memory write and invalidate enable
#define PCI_CMD_MWIE_GET(n)    (((n) >> PCI_CMD_MWIE_BIT) & PCI_CMD_MWIE_MASK)

// Special cycle enable
#define PCI_CMD_SCE_GET(n)     (((n) >> PCI_CMD_SCE_BIT) & PCI_CMD_SCE_MASK)

// Bus master enable
#define PCI_CMD_BME_GET(n)     (((n) >> PCI_CMD_BME_BIT) & PCI_CMD_BME_MASK)

// Memory space enable
#define PCI_CMD_MSE_GET(n)     (((n) >> PCI_CMD_MSE_BIT) & PCI_CMD_MSE_MASK)

// I/O space enable
#define PCI_CMD_IOSE_GET(n)    (((n) >> PCI_CMD_IOSE_BIT) & PCI_CMD_IOSE_MASK)


// Interrupt pin disable (does not affect MSI)
#define PCI_CMD_ID_SET(r,n)    ((r) = ((r) & ~PCI_CMD_ID) | PCI_CMD_ID_n((n)))

// Fast back-to-back enable
#define PCI_CMD_FBE_SET(r,n) \
    ((r) = ((r) & ~PCI_CMD_FBE) | PCI_CMD_FBE_n((n)))

// SERR# Enable
#define PCI_CMD_SEE_SET(r,n) \
    ((r) = ((r) & ~PCI_CMD_SEE) | PCI_CMD_SEE_n((n)))

// Parity error response enable
#define PCI_CMD_PEE_SET(r,n) \
    ((r) = ((r) & ~PCI_CMD_PEE) | PCI_CMD_PEE_n((n)))

// VGA palette snooping enable
#define PCI_CMD_VGA_SET(r,n) \
    ((r) = ((r) & ~PCI_CMD_VGA) | PCI_CMD_VGA_n((n)))

// Memory write and invalidate enable
#define PCI_CMD_MWIE_SET(r,n) \
    ((r) = ((r) & ~PCI_CMD_MWIE) | PCI_CMD_MWIE_n((n)))

// Special cycle enable
#define PCI_CMD_SCE_SET(r,n) \
    ((r) = ((r) & ~PCI_CMD_SCE) | PCI_CMD_SCE_n((n)))

// Bus master enable
#define PCI_CMD_BME_SET(r,n) \
    ((r) = ((r) & ~PCI_CMD_BME) | PCI_CMD_BME_n((n)))

// Memory space enable
#define PCI_CMD_MSE_SET(r,n) \
    ((r) = ((r) & ~PCI_CMD_MSE) | PCI_CMD_MSE_n((n)))

// I/O space enable
#define PCI_CMD_IOSE_SET(r,n) \
    ((r) = ((r) & ~PCI_CMD_IOSE) | PCI_CMD_IOSE_n((n)))

//
// PCI_STS: Device status
#define PCI_STS                0x06


// Detected parity error
#define PCI_STS_DPE_BIT        15

// Signalled system error
#define PCI_STS_SSE_BIT        14

// Received master abort
#define PCI_STS_RMA_BIT        13

// Received target abort
#define PCI_STS_RTA_BIT        12

// Signaled target abort
#define PCI_STS_STO_BIT        11

// DEVSEL# timing
#define PCI_STS_DEVT_BIT       9

// Master data parity error detected
#define PCI_STS_DPD_BIT        8

// Fast back to back capable
#define PCI_STS_FBC_BIT        7

// 66MHz capable
#define PCI_STS_C66_BIT        5

// Capabilities list present
#define PCI_STS_CL_BIT         4

// Interrupt status (1=asserted)
#define PCI_STS_IS_BIT         3


// Detected parity error
#define PCI_STS_DPE_BITS       1

// Signalled system error
#define PCI_STS_SSE_BITS       1

// Received master abort
#define PCI_STS_RMA_BITS       1

// Received target abort
#define PCI_STS_RTA_BITS       1

// Signaled target abort
#define PCI_STS_STO_BITS       1

// DEVSEL# timing
#define PCI_STS_DEVT_BITS      2

// Master data parity error detected
#define PCI_STS_DPD_BITS       1

// Fast back to back capable
#define PCI_STS_FBC_BITS       1

// 66MHz capable
#define PCI_STS_C66_BITS       1

// Capabilities list present
#define PCI_STS_CL_BITS        1

// Interrupt status (1=asserted)
#define PCI_STS_IS_BITS        1

// Detected parity error
#define PCI_STS_DPE_MASK       ((1U << PCI_STS_DPE_BITS)-1)

// Signalled system error
#define PCI_STS_SSE_MASK       ((1U << PCI_STS_SSE_BITS)-1)

// Received master abort
#define PCI_STS_RMA_MASK       ((1U << PCI_STS_RMA_BITS)-1)

// Received target abort
#define PCI_STS_RTA_MASK       ((1U << PCI_STS_RTA_BITS)-1)

// Signaled target abort
#define PCI_STS_STO_MASK       ((1U << PCI_STS_STO_BITS)-1)

// DEVSEL# timing
#define PCI_STS_DEVT_MASK      ((1U << PCI_STS_DEVT_BITS)-1)

// Master data parity error detected
#define PCI_STS_DPD_MASK       ((1U << PCI_STS_DPD_BITS)-1)

// Fast back to back capable
#define PCI_STS_FBC_MASK       ((1U << PCI_STS_FBC_BITS)-1)

// 66MHz capable
#define PCI_STS_C66_MASK       ((1U << PCI_STS_C66_BITS)-1)

// Capabilities list present
#define PCI_STS_CL_MASK        ((1U << PCI_STS_CL_BITS)-1)

// Interrupt status (1=asserted)
#define PCI_STS_IS_MASK        ((1U << PCI_STS_IS_BITS)-1)

// Detected parity error
#define PCI_STS_DPE            (PCI_STS_DPE_MASK << PCI_STS_DPE_BIT)

// Signalled system error
#define PCI_STS_SSE            (PCI_STS_SSE_MASK << PCI_STS_SSE_BIT)

// Received master abort
#define PCI_STS_RMA            (PCI_STS_RMA_MASK << PCI_STS_RMA_BIT)

// Received target abort
#define PCI_STS_RTA            (PCI_STS_RTA_MASK << PCI_STS_RTA_BIT)

// Signaled target abort
#define PCI_STS_STO            (PCI_STS_STO_MASK << PCI_STS_STO_BIT)

// DEVSEL# timing
#define PCI_STS_DEVT           (PCI_STS_DEVT_MASK << PCI_STS_DEVT_BIT)

// Master data parity error detected
#define PCI_STS_DPD            (PCI_STS_DPD_MASK << PCI_STS_DPD_BIT)

// Fast back to back capable
#define PCI_STS_FBC            (PCI_STS_FBC_MASK << PCI_STS_FBC_BIT)

// 66MHz capable
#define PCI_STS_C66            (PCI_STS_C66_MASK << PCI_STS_C66_BIT)

// Capabilities list present
#define PCI_STS_CL             (PCI_STS_CL_MASK << PCI_STS_CL_BIT)

// Interrupt status (1=asserted)
#define PCI_STS_IS             (PCI_STS_IS_MASK << PCI_STS_IS_BIT)


// Detected parity error
#define PCI_STS_DPE_n(n)       ((n) << PCI_STS_DPE_BIT)

// Signalled system error
#define PCI_STS_SSE_n(n)       ((n) << PCI_STS_SSE_BIT)

// Received master abort
#define PCI_STS_RMA_n(n)       ((n) << PCI_STS_RMA_BIT)

// Received target abort
#define PCI_STS_RTA_n(n)       ((n) << PCI_STS_RTA_BIT)

// Signaled target abort
#define PCI_STS_STO_n(n)       ((n) << PCI_STS_STO_BIT)

// DEVSEL# timing
#define PCI_STS_DEVT_n(n)      ((n) << PCI_STS_DEVT_BIT)

// Master data parity error detected
#define PCI_STS_DPD_n(n)       ((n) << PCI_STS_DPD_BIT)

// Fast back to back capable
#define PCI_STS_FBC_n(n)       ((n) << PCI_STS_FBC_BIT)

// 66MHz capable
#define PCI_STS_C66_n(n)       ((n) << PCI_STS_C66_BIT)

// Capabilities list present
#define PCI_STS_CL_n(n)        ((n) << PCI_STS_CL_BIT)

// Interrupt status (1=asserted)
#define PCI_STS_IS_n(n)        ((n) << PCI_STS_IS_BIT)


// Detected parity error
#define PCI_STS_DPE_GET(n)     (((n) >> PCI_STS_DPE_BIT) & PCI_STS_DPE_MASK)

// Signalled system error
#define PCI_STS_SSE_GET(n)     (((n) >> PCI_STS_SSE_BIT) & PCI_STS_SSE_MASK)

// Received master abort
#define PCI_STS_RMA_GET(n)     (((n) >> PCI_STS_RMA_BIT) & PCI_STS_RMA_MASK)

// Received target abort
#define PCI_STS_RTA_GET(n)     (((n) >> PCI_STS_RTA_BIT) & PCI_STS_RTA_MASK)

// Signaled target abort
#define PCI_STS_STO_GET(n)     (((n) >> PCI_STS_STO_BIT) & PCI_STS_STO_MASK)

// DEVSEL# timing
#define PCI_STS_DEVT_GET(n)    (((n) >> PCI_STS_DEVT_BIT) & PCI_STS_DEVT_MASK)

// Master data parity error detected
#define PCI_STS_DPD_GET(n)     (((n) >> PCI_STS_DPD_BIT) & PCI_STS_DPD_MASK)

// Fast back to back capable
#define PCI_STS_FBC_GET(n)     (((n) >> PCI_STS_FBC_BIT) & PCI_STS_FBC_MASK)

// 66MHz capable
#define PCI_STS_C66_GET(n)     (((n) >> PCI_STS_C66_BIT) & PCI_STS_C66_MASK)

// Capabilities list present
#define PCI_STS_CL_GET(n)      (((n) >> PCI_STS_CL_BIT) & PCI_STS_CL_MASK)

// Interrupt status (1=asserted)
#define PCI_STS_IS_GET(n)      (((n) >> PCI_STS_IS_BIT) & PCI_STS_IS_MASK)


// Detected parity error
#define PCI_STS_DPE_SET(r,n) \
    ((r) = ((r) & ~PCI_STS_DPE) | PCI_STS_DPE_n((n)))

// Signalled system error
#define PCI_STS_SSE_SET(r,n) \
    ((r) = ((r) & ~PCI_STS_SSE) | PCI_STS_SSE_n((n)))

// Received master abort
#define PCI_STS_RMA_SET(r,n) \
    ((r) = ((r) & ~PCI_STS_RMA) | PCI_STS_RMA_n((n)))

// Received target abort
#define PCI_STS_RTA_SET(r,n) \
    ((r) = ((r) & ~PCI_STS_RTA) | PCI_STS_RTA_n((n)))

// Signaled target abort
#define PCI_STS_STO_SET(r,n) \
    ((r) = ((r) & ~PCI_STS_STO) | PCI_STS_STO_n((n)))

// DEVSEL# timing
#define PCI_STS_DEVT_SET(r,n) \
    ((r) = ((r) & ~PCI_STS_DEVT) | PCI_STS_DEVT_n((n)))

// Master data parity error detected
#define PCI_STS_DPD_SET(r,n) \
    ((r) = ((r) & ~PCI_STS_DPD) | PCI_STS_DPD_n((n)))

// Fast back to back capable
#define PCI_STS_FBC_SET(r,n) \
    ((r) = ((r) & ~PCI_STS_FBC) | PCI_STS_FBC_n((n)))

// 66MHz capable
#define PCI_STS_C66_SET(r,n) \
    ((r) = ((r) & ~PCI_STS_C66) | PCI_STS_C66_n((n)))

// Capabilities list present
#define PCI_STS_CL_SET(r,n)    ((r) = ((r) & ~PCI_STS_CL) | PCI_STS_CL_n((n)))

// Interrupt status (1=asserted)
#define PCI_STS_IS_SET(r,n)    ((r) = ((r) & ~PCI_STS_IS) | PCI_STS_IS_n((n)))

//
// PCI_RID: Revision ID
#define PCI_RID               0x08


// Revision ID
#define PCI_RID_RID_BIT       0


// Revision ID
#define PCI_RID_RID_BITS      8

// Revision ID
#define PCI_RID_RID_MASK      ((1U << PCI_RID_RID_BITS)-1)

// Revision ID
#define PCI_RID_RID           (PCI_RID_RID_MASK << PCI_RID_RID_BIT)


// Revision ID
#define PCI_RID_RID_n(n)      ((n) << PCI_RID_RID_BIT)


// Revision ID
#define PCI_RID_RID_GET(n)    (((n) >> PCI_RID_RID_BIT) & PCI_RID_RID_MASK)


// Revision ID
#define PCI_RID_RID_SET(r,n)  ((r) = ((r) & ~PCI_RID_RID) | PCI_RID_RID_n((n)))

//
// PCI_CC: Class code
#define PCI_CC               0x09


// Base class code
#define PCI_CC_BCC_BIT       16

// Sub class code
#define PCI_CC_SCC_BIT       8

// Programming interface
#define PCI_CC_PI_BIT        0


// Base class code
#define PCI_CC_BCC_BITS      8

// Sub class code
#define PCI_CC_SCC_BITS      8

// Programming interface
#define PCI_CC_PI_BITS       8

// Base class code
#define PCI_CC_BCC_MASK      ((1U << PCI_CC_BCC_BITS)-1)

// Sub class code
#define PCI_CC_SCC_MASK      ((1U << PCI_CC_SCC_BITS)-1)

// Programming interface
#define PCI_CC_PI_MASK       ((1U << PCI_CC_PI_BITS)-1)

// Base class code
#define PCI_CC_BCC           (PCI_CC_BCC_MASK << PCI_CC_BCC_BIT)

// Sub class code
#define PCI_CC_SCC           (PCI_CC_SCC_MASK << PCI_CC_SCC_BIT)

// Programming interface
#define PCI_CC_PI            (PCI_CC_PI_MASK << PCI_CC_PI_BIT)


// Base class code
#define PCI_CC_BCC_n(n)      ((n) << PCI_CC_BCC_BIT)

// Sub class code
#define PCI_CC_SCC_n(n)      ((n) << PCI_CC_SCC_BIT)

// Programming interface
#define PCI_CC_PI_n(n)       ((n) << PCI_CC_PI_BIT)


// Base class code
#define PCI_CC_BCC_GET(n)    (((n) >> PCI_CC_BCC_BIT) & PCI_CC_BCC_MASK)

// Sub class code
#define PCI_CC_SCC_GET(n)    (((n) >> PCI_CC_SCC_BIT) & PCI_CC_SCC_MASK)

// Programming interface
#define PCI_CC_PI_GET(n)     (((n) >> PCI_CC_PI_BIT) & PCI_CC_PI_MASK)


// Base class code
#define PCI_CC_BCC_SET(r,n)  ((r) = ((r) & ~PCI_CC_BCC) | PCI_CC_BCC_n((n)))

// Sub class code
#define PCI_CC_SCC_SET(r,n)  ((r) = ((r) & ~PCI_CC_SCC) | PCI_CC_SCC_n((n)))

// Programming interface
#define PCI_CC_PI_SET(r,n)   ((r) = ((r) & ~PCI_CC_PI) | PCI_CC_PI_n((n)))

//
// PCI_CLS: Cache line size
#define PCI_CLS               0x0C


// Cache line size
#define PCI_CLS_CLS_BIT       0


// Cache line size
#define PCI_CLS_CLS_BITS      8

// Cache line size
#define PCI_CLS_CLS_MASK      ((1U << PCI_CLS_CLS_BITS)-1)

// Cache line size
#define PCI_CLS_CLS           (PCI_CLS_CLS_MASK << PCI_CLS_CLS_BIT)


// Cache line size
#define PCI_CLS_CLS_n(n)      ((n) << PCI_CLS_CLS_BIT)


// Cache line size
#define PCI_CLS_CLS_GET(n)    (((n) >> PCI_CLS_CLS_BIT) & PCI_CLS_CLS_MASK)


// Cache line size
#define PCI_CLS_CLS_SET(r,n)  ((r) = ((r) & ~PCI_CLS_CLS) | PCI_CLS_CLS_n((n)))

//
// PCI_MLT: Master latency timer
#define PCI_MLT               0x0D


// Master latency timer
#define PCI_MLT_MLT_BIT       0


// Master latency timer
#define PCI_MLT_MLT_BITS      8

// Master latency timer
#define PCI_MLT_MLT_MASK      ((1U << PCI_MLT_MLT_BITS)-1)

// Master latency timer
#define PCI_MLT_MLT           (PCI_MLT_MLT_MASK << PCI_MLT_MLT_BIT)


// Master latency timer
#define PCI_MLT_MLT_n(n)      ((n) << PCI_MLT_MLT_BIT)


// Master latency timer
#define PCI_MLT_MLT_GET(n)    (((n) >> PCI_MLT_MLT_BIT) & PCI_MLT_MLT_MASK)


// Master latency timer
#define PCI_MLT_MLT_SET(r,n)  ((r) = ((r) & ~PCI_MLT_MLT) | PCI_MLT_MLT_n((n)))

//
// PCI_HTYPE: Header type
#define PCI_HTYPE               0x0E


// Multifunction device
#define PCI_HTYPE_MFD_BIT       7

// Header layout
#define PCI_HTYPE_HL_BIT        0


// Multifunction device
#define PCI_HTYPE_MFD_BITS      1

// Header layout
#define PCI_HTYPE_HL_BITS       7

// Multifunction device
#define PCI_HTYPE_MFD_MASK      ((1U << PCI_HTYPE_MFD_BITS)-1)

// Header layout
#define PCI_HTYPE_HL_MASK       ((1U << PCI_HTYPE_HL_BITS)-1)

// Multifunction device
#define PCI_HTYPE_MFD           (PCI_HTYPE_MFD_MASK << PCI_HTYPE_MFD_BIT)

// Header layout
#define PCI_HTYPE_HL            (PCI_HTYPE_HL_MASK << PCI_HTYPE_HL_BIT)


// Multifunction device
#define PCI_HTYPE_MFD_n(n)      ((n) << PCI_HTYPE_MFD_BIT)

// Header layout
#define PCI_HTYPE_HL_n(n)       ((n) << PCI_HTYPE_HL_BIT)


// Multifunction device
#define PCI_HTYPE_MFD_GET(n) \
    (((n) >> PCI_HTYPE_MFD_BIT) & PCI_HTYPE_MFD_MASK)

// Header layout
#define PCI_HTYPE_HL_GET(n)     (((n) >> PCI_HTYPE_HL_BIT) & PCI_HTYPE_HL_MASK)


// Multifunction device
#define PCI_HTYPE_MFD_SET(r,n) \
    ((r) = ((r) & ~PCI_HTYPE_MFD) | PCI_HTYPE_MFD_n((n)))

// Header layout
#define PCI_HTYPE_HL_SET(r,n) \
    ((r) = ((r) & ~PCI_HTYPE_HL) | PCI_HTYPE_HL_n((n)))

//
// PCI_BIST: Built in self test (optional)

// Built in self test capable
#define PCI_BIST_BC_BIT       7

// Start built in self test
#define PCI_BIST_SB_BIT       6

// Completion code
#define PCI_BIST_CC_BIT       0


// Built in self test capable
#define PCI_BIST_BC_BITS      1

// Start built in self test
#define PCI_BIST_SB_BITS      1

// Completion code
#define PCI_BIST_CC_BITS      4

// Built in self test capable
#define PCI_BIST_BC_MASK      ((1U << PCI_BIST_BC_BITS)-1)

// Start built in self test
#define PCI_BIST_SB_MASK      ((1U << PCI_BIST_SB_BITS)-1)

// Completion code
#define PCI_BIST_CC_MASK      ((1U << PCI_BIST_CC_BITS)-1)

// Built in self test capable
#define PCI_BIST_BC           (PCI_BIST_BC_MASK << PCI_BIST_BC_BIT)

// Start built in self test
#define PCI_BIST_SB           (PCI_BIST_SB_MASK << PCI_BIST_SB_BIT)

// Completion code
#define PCI_BIST_CC           (PCI_BIST_CC_MASK << PCI_BIST_CC_BIT)


// Built in self test capable
#define PCI_BIST_BC_n(n)      ((n) << PCI_BIST_BC_BIT)

// Start built in self test
#define PCI_BIST_SB_n(n)      ((n) << PCI_BIST_SB_BIT)

// Completion code
#define PCI_BIST_CC_n(n)      ((n) << PCI_BIST_CC_BIT)


// Built in self test capable
#define PCI_BIST_BC_GET(n)    (((n) >> PCI_BIST_BC_BIT) & PCI_BIST_BC_MASK)

// Start built in self test
#define PCI_BIST_SB_GET(n)    (((n) >> PCI_BIST_SB_BIT) & PCI_BIST_SB_MASK)

// Completion code
#define PCI_BIST_CC_GET(n)    (((n) >> PCI_BIST_CC_BIT) & PCI_BIST_CC_MASK)


// Built in self test capable
#define PCI_BIST_BC_SET(r,n)  ((r) = ((r) & ~PCI_BIST_BC) | PCI_BIST_BC_n((n)))

// Start built in self test
#define PCI_BIST_SB_SET(r,n)  ((r) = ((r) & ~PCI_BIST_SB) | PCI_BIST_SB_n((n)))

// Completion code
#define PCI_BIST_CC_SET(r,n)  ((r) = ((r) & ~PCI_BIST_CC) | PCI_BIST_CC_n((n)))

// PCI_BAR


// Base address
#define PCI_BAR_MMIO_BA_BIT         4

// Base address
#define PCI_BAR_IO_BA_BIT           2

// Prefetchable
#define PCI_BAR_MMIO_PF_BIT         3

// (0=32-bit, 2=64-bit, 1=reserved, 3=reserved)
#define PCI_BAR_MMIO_TYPE_BIT       1

// Resource type indicator (0=MMIO, 1=I/O)
#define PCI_BAR_RTE_BIT             0


// Base address
#define PCI_BAR_MMIO_BA_BITS        28

// Base address
#define PCI_BAR_IO_BA_BITS          30

// Prefetchable
#define PCI_BAR_MMIO_PF_BITS        1

// (0=32-bit, 2=64-bit, 1=reserved, 3=reserved)
#define PCI_BAR_MMIO_TYPE_BITS      2

// Resource type indicator (0=MMIO, 1=I/O)
#define PCI_BAR_RTE_BITS            1

// Base address
#define PCI_BAR_MMIO_BA_MASK        ((1U << PCI_BAR_MMIO_BA_BITS)-1)

// Base address
#define PCI_BAR_IO_BA_MASK          ((1U << PCI_BAR_IO_BA_BITS)-1)

// Prefetchable
#define PCI_BAR_MMIO_PF_MASK        ((1U << PCI_BAR_MMIO_PF_BITS)-1)

// (0=32-bit, 2=64-bit, 1=reserved, 3=reserved)
#define PCI_BAR_MMIO_TYPE_MASK      ((1U << PCI_BAR_MMIO_TYPE_BITS)-1)

// Resource type indicator (0=MMIO, 1=I/O)
#define PCI_BAR_RTE_MASK            ((1U << PCI_BAR_RTE_BITS)-1)

// Base address
#define PCI_BAR_MMIO_BA \
    (PCI_BAR_MMIO_BA_MASK << PCI_BAR_MMIO_BA_BIT)

// Base address
#define PCI_BAR_IO_BA               (PCI_BAR_IO_BA_MASK << PCI_BAR_IO_BA_BIT)

// Prefetchable
#define PCI_BAR_MMIO_PF \
    (PCI_BAR_MMIO_PF_MASK << PCI_BAR_MMIO_PF_BIT)

// (0=32-bit, 2=64-bit, 1=reserved, 3=reserved)
#define PCI_BAR_MMIO_TYPE \
    (PCI_BAR_MMIO_TYPE_MASK << PCI_BAR_MMIO_TYPE_BIT)

// Resource type indicator (0=MMIO, 1=I/O)
#define PCI_BAR_RTE                 (PCI_BAR_RTE_MASK << PCI_BAR_RTE_BIT)


// Base address
#define PCI_BAR_MMIO_BA_n(n)        ((n) << PCI_BAR_MMIO_BA_BIT)

// Base address
#define PCI_BAR_IO_BA_n(n)          ((n) << PCI_BAR_IO_BA_BIT)

// Prefetchable
#define PCI_BAR_MMIO_PF_n(n)        ((n) << PCI_BAR_MMIO_PF_BIT)

// (0=32-bit, 2=64-bit, 1=reserved, 3=reserved)
#define PCI_BAR_MMIO_TYPE_n(n)      ((n) << PCI_BAR_MMIO_TYPE_BIT)

// Resource type indicator (0=MMIO, 1=I/O)
#define PCI_BAR_RTE_n(n)            ((n) << PCI_BAR_RTE_BIT)


// Base address
#define PCI_BAR_MMIO_BA_GET(n) \
    (((n) >> PCI_BAR_MMIO_BA_BIT) & PCI_BAR_MMIO_BA_MASK)

// Base address
#define PCI_BAR_IO_BA_GET(n) \
    (((n) >> PCI_BAR_IO_BA_BIT) & PCI_BAR_IO_BA_MASK)

// Prefetchable
#define PCI_BAR_MMIO_PF_GET(n) \
    (((n) >> PCI_BAR_MMIO_PF_BIT) & PCI_BAR_MMIO_PF_MASK)

// (0=32-bit, 2=64-bit, 1=reserved, 3=reserved)
#define PCI_BAR_MMIO_TYPE_GET(n) \
    (((n) >> PCI_BAR_MMIO_TYPE_BIT) & PCI_BAR_MMIO_TYPE_MASK)

// Resource type indicator (0=MMIO, 1=I/O)
#define PCI_BAR_RTE_GET(n) \
    (((n) >> PCI_BAR_RTE_BIT) & PCI_BAR_RTE_MASK)


// Base address
#define PCI_BAR_MMIO_BA_SET(r,n) \
    ((r) = ((r) & ~PCI_BAR_MMIO_BA) | PCI_BAR_MMIO_BA_n((n)))

// Base address
#define PCI_BAR_IO_BA_SET(r,n) \
    ((r) = ((r) & ~PCI_BAR_IO_BA) | PCI_BAR_IO_BA_n((n)))

// Prefetchable
#define PCI_BAR_MMIO_PF_SET(r,n) \
    ((r) = ((r) & ~PCI_BAR_MMIO_PF) | PCI_BAR_MMIO_PF_n((n)))

// (0=32-bit, 2=64-bit, 1=reserved, 3=reserved)
#define PCI_BAR_MMIO_TYPE_SET(r,n) \
    ((r) = ((r) & ~PCI_BAR_MMIO_TYPE) | PCI_BAR_MMIO_TYPE_n((n)))

// Resource type indicator (0=MMIO, 1=I/O)
#define PCI_BAR_RTE_SET(r,n) \
    ((r) = ((r) & ~PCI_BAR_RTE) | PCI_BAR_RTE_n((n)))

// PCI_BAR_MMIO_RTE_MMIO
#define PCI_BAR_MMIO_RTE_MMIO 0

// PCI_BAR_MMIO_RTE_IO
#define PCI_BAR_MMIO_RTE_IO 1

// PCI_BAR_MMIO_TYPE_32BIT
#define PCI_BAR_MMIO_TYPE_32BIT 0

// PCI_BAR_MMIO_TYPE_BELOW1M
#define PCI_BAR_MMIO_TYPE_BELOW1M 1

// PCI_BAR_MMIO_TYPE_64BIT
#define PCI_BAR_MMIO_TYPE_64BIT 2

// PCI_BAR_MMIO_TYPE_RESERVED
#define PCI_BAR_MMIO_TYPE_RESERVED 3

//
// PCI_SS: Subsystem identifiers


// Subsystem identifier
#define PCI_SS_SSID_BIT        16

// Subsystem vendor identifier
#define PCI_SS_SSVID_BIT       0


// Subsystem identifier
#define PCI_SS_SSID_BITS       16

// Subsystem vendor identifier
#define PCI_SS_SSVID_BITS      16

// Subsystem identifier
#define PCI_SS_SSID_MASK       ((1U << PCI_SS_SSID_BITS)-1)

// Subsystem vendor identifier
#define PCI_SS_SSVID_MASK      ((1U << PCI_SS_SSVID_BITS)-1)

// Subsystem identifier
#define PCI_SS_SSID            (PCI_SS_SSID_MASK << PCI_SS_SSID_BIT)

// Subsystem vendor identifier
#define PCI_SS_SSVID           (PCI_SS_SSVID_MASK << PCI_SS_SSVID_BIT)


// Subsystem identifier
#define PCI_SS_SSID_n(n)       ((n) << PCI_SS_SSID_BIT)

// Subsystem vendor identifier
#define PCI_SS_SSVID_n(n)      ((n) << PCI_SS_SSVID_BIT)


// Subsystem identifier
#define PCI_SS_SSID_GET(n)     (((n) >> PCI_SS_SSID_BIT) & PCI_SS_SSID_MASK)

// Subsystem vendor identifier
#define PCI_SS_SSVID_GET(n)    (((n) >> PCI_SS_SSVID_BIT) & PCI_SS_SSVID_MASK)


// Subsystem identifier
#define PCI_SS_SSID_SET(r,n) \
    ((r) = ((r) & ~PCI_SS_SSID) | PCI_SS_SSID_n((n)))

// Subsystem vendor identifier
#define PCI_SS_SSVID_SET(r,n) \
    ((r) = ((r) & ~PCI_SS_SSVID) | PCI_SS_SSVID_n((n)))

//
// PCI_CAP: Capabilities pointer


// Capability pointer
#define PCI_CAP_CP_BIT       0


// Capability pointer
#define PCI_CAP_CP_BITS      8

// Capability pointer
#define PCI_CAP_CP_MASK      ((1U << PCI_CAP_CP_BITS)-1)

// Capability pointer
#define PCI_CAP_CP           (PCI_CAP_CP_MASK << PCI_CAP_CP_BIT)


// Capability pointer
#define PCI_CAP_CP_n(n)      ((n) << PCI_CAP_CP_BIT)


// Capability pointer
#define PCI_CAP_CP_GET(n)    (((n) >> PCI_CAP_CP_BIT) & PCI_CAP_CP_MASK)


// Capability pointer
#define PCI_CAP_CP_SET(r,n)  ((r) = ((r) & ~PCI_CAP_CP) | PCI_CAP_CP_n((n)))

// PCI_INTR


// Interrupt pin
#define PCI_INTR_IPIN_BIT        8

// Interrupt line
#define PCI_INTR_ILINE_BIT       0


// Interrupt pin
#define PCI_INTR_IPIN_BITS       8

// Interrupt line
#define PCI_INTR_ILINE_BITS      8

// Interrupt pin
#define PCI_INTR_IPIN_MASK       ((1U << PCI_INTR_IPIN_BITS)-1)

// Interrupt line
#define PCI_INTR_ILINE_MASK      ((1U << PCI_INTR_ILINE_BITS)-1)

// Interrupt pin
#define PCI_INTR_IPIN            (PCI_INTR_IPIN_MASK << PCI_INTR_IPIN_BIT)

// Interrupt line
#define PCI_INTR_ILINE           (PCI_INTR_ILINE_MASK << PCI_INTR_ILINE_BIT)


// Interrupt pin
#define PCI_INTR_IPIN_n(n)       ((n) << PCI_INTR_IPIN_BIT)

// Interrupt line
#define PCI_INTR_ILINE_n(n)      ((n) << PCI_INTR_ILINE_BIT)


// Interrupt pin
#define PCI_INTR_IPIN_GET(n) \
    (((n) >> PCI_INTR_IPIN_BIT) & PCI_INTR_IPIN_MASK)

// Interrupt line
#define PCI_INTR_ILINE_GET(n) \
    (((n) >> PCI_INTR_ILINE_BIT) & PCI_INTR_ILINE_MASK)


// Interrupt pin
#define PCI_INTR_IPIN_SET(r,n) \
    ((r) = ((r) & ~PCI_INTR_IPIN) | PCI_INTR_IPIN_n((n)))

// Interrupt line
#define PCI_INTR_ILINE_SET(r,n) \
    ((r) = ((r) & ~PCI_INTR_ILINE) | PCI_INTR_ILINE_n((n)))

//
// PCI_MGNT: Minimum grant


// Minimum grant
#define PCI_MGNT_GNT_BIT       0


// Minimum grant
#define PCI_MGNT_GNT_BITS      8

// Minimum grant
#define PCI_MGNT_GNT_MASK      ((1U << PCI_MGNT_GNT_BITS)-1)

// Minimum grant
#define PCI_MGNT_GNT           (PCI_MGNT_GNT_MASK << PCI_MGNT_GNT_BIT)


// Minimum grant
#define PCI_MGNT_GNT_n(n)      ((n) << PCI_MGNT_GNT_BIT)


// Minimum grant
#define PCI_MGNT_GNT_GET(n)    (((n) >> PCI_MGNT_GNT_BIT) & PCI_MGNT_GNT_MASK)


// Minimum grant
#define PCI_MGNT_GNT_SET(r,n) \
    ((r) = ((r) & ~PCI_MGNT_GNT) | PCI_MGNT_GNT_n((n)))

//
// PCI_MLAT: Maximum latency


// Maximum latency
#define PCI_MLAT_LAT_BIT       0


// Maximum latency
#define PCI_MLAT_LAT_BITS      8

// Maximum latency
#define PCI_MLAT_LAT_MASK      ((1U << PCI_MLAT_LAT_BITS)-1)

// Maximum latency
#define PCI_MLAT_LAT           (PCI_MLAT_LAT_MASK << PCI_MLAT_LAT_BIT)


// Maximum latency
#define PCI_MLAT_LAT_n(n)      ((n) << PCI_MLAT_LAT_BIT)


// Maximum latency
#define PCI_MLAT_LAT_GET(n)    (((n) >> PCI_MLAT_LAT_BIT) & PCI_MLAT_LAT_MASK)


// Maximum latency
#define PCI_MLAT_LAT_SET(r,n) \
    ((r) = ((r) & ~PCI_MLAT_LAT) | PCI_MLAT_LAT_n((n)))

// PCI_MSI_MSG_CTRL


// Per-vector mask capable
#define PCI_MSI_MSG_CTRL_VMASK_BIT       8

// 64-bit capable
#define PCI_MSI_MSG_CTRL_CAP64_BIT       7

// Multiple message enable
#define PCI_MSI_MSG_CTRL_MME_BIT         4

// Multiple message capable (log2 N)
#define PCI_MSI_MSG_CTRL_MMC_BIT         1

// Enable
#define PCI_MSI_MSG_CTRL_EN_BIT          0


// Per-vector mask capable
#define PCI_MSI_MSG_CTRL_VMASK_BITS      1

// 64-bit capable
#define PCI_MSI_MSG_CTRL_CAP64_BITS      1

// Multiple message enable
#define PCI_MSI_MSG_CTRL_MME_BITS        3

// Multiple message capable (log2 N)
#define PCI_MSI_MSG_CTRL_MMC_BITS        3

// Enable
#define PCI_MSI_MSG_CTRL_EN_BITS         1

// Per-vector mask capable
#define PCI_MSI_MSG_CTRL_VMASK_MASK \
    ((1U << PCI_MSI_MSG_CTRL_VMASK_BITS)-1)

// 64-bit capable
#define PCI_MSI_MSG_CTRL_CAP64_MASK \
    ((1U << PCI_MSI_MSG_CTRL_CAP64_BITS)-1)

// Multiple message enable
#define PCI_MSI_MSG_CTRL_MME_MASK        ((1U << PCI_MSI_MSG_CTRL_MME_BITS)-1)

// Multiple message capable (log2 N)
#define PCI_MSI_MSG_CTRL_MMC_MASK        ((1U << PCI_MSI_MSG_CTRL_MMC_BITS)-1)

// Enable
#define PCI_MSI_MSG_CTRL_EN_MASK         ((1U << PCI_MSI_MSG_CTRL_EN_BITS)-1)

// Per-vector mask capable
#define PCI_MSI_MSG_CTRL_VMASK \
    (PCI_MSI_MSG_CTRL_VMASK_MASK << PCI_MSI_MSG_CTRL_VMASK_BIT)

// 64-bit capable
#define PCI_MSI_MSG_CTRL_CAP64 \
    (PCI_MSI_MSG_CTRL_CAP64_MASK << PCI_MSI_MSG_CTRL_CAP64_BIT)

// Multiple message enable
#define PCI_MSI_MSG_CTRL_MME \
    (PCI_MSI_MSG_CTRL_MME_MASK << PCI_MSI_MSG_CTRL_MME_BIT)

// Multiple message capable (log2 N)
#define PCI_MSI_MSG_CTRL_MMC \
    (PCI_MSI_MSG_CTRL_MMC_MASK << PCI_MSI_MSG_CTRL_MMC_BIT)

// Enable
#define PCI_MSI_MSG_CTRL_EN \
    (PCI_MSI_MSG_CTRL_EN_MASK << PCI_MSI_MSG_CTRL_EN_BIT)


// Per-vector mask capable
#define PCI_MSI_MSG_CTRL_VMASK_n(n)      ((n) << PCI_MSI_MSG_CTRL_VMASK_BIT)

// 64-bit capable
#define PCI_MSI_MSG_CTRL_CAP64_n(n)      ((n) << PCI_MSI_MSG_CTRL_CAP64_BIT)

// Multiple message enable
#define PCI_MSI_MSG_CTRL_MME_n(n)        ((n) << PCI_MSI_MSG_CTRL_MME_BIT)

// Multiple message capable (log2 N)
#define PCI_MSI_MSG_CTRL_MMC_n(n)        ((n) << PCI_MSI_MSG_CTRL_MMC_BIT)

// Enable
#define PCI_MSI_MSG_CTRL_EN_n(n)         ((n) << PCI_MSI_MSG_CTRL_EN_BIT)


// Per-vector mask capable
#define PCI_MSI_MSG_CTRL_VMASK_GET(n) \
    (((n) >> PCI_MSI_MSG_CTRL_VMASK_BIT) & PCI_MSI_MSG_CTRL_VMASK_MASK)

// 64-bit capable
#define PCI_MSI_MSG_CTRL_CAP64_GET(n) \
    (((n) >> PCI_MSI_MSG_CTRL_CAP64_BIT) & PCI_MSI_MSG_CTRL_CAP64_MASK)

// Multiple message enable
#define PCI_MSI_MSG_CTRL_MME_GET(n) \
    (((n) >> PCI_MSI_MSG_CTRL_MME_BIT) & PCI_MSI_MSG_CTRL_MME_MASK)

// Multiple message capable (log2 N)
#define PCI_MSI_MSG_CTRL_MMC_GET(n) \
    (((n) >> PCI_MSI_MSG_CTRL_MMC_BIT) & PCI_MSI_MSG_CTRL_MMC_MASK)

// Enable
#define PCI_MSI_MSG_CTRL_EN_GET(n) \
    (((n) >> PCI_MSI_MSG_CTRL_EN_BIT) & PCI_MSI_MSG_CTRL_EN_MASK)


// Per-vector mask capable
#define PCI_MSI_MSG_CTRL_VMASK_SET(r,n) \
    ((r) = ((r) & ~PCI_MSI_MSG_CTRL_VMASK) | PCI_MSI_MSG_CTRL_VMASK_n((n)))

// 64-bit capable
#define PCI_MSI_MSG_CTRL_CAP64_SET(r,n) \
    ((r) = ((r) & ~PCI_MSI_MSG_CTRL_CAP64) | PCI_MSI_MSG_CTRL_CAP64_n((n)))

// Multiple message enable
#define PCI_MSI_MSG_CTRL_MME_SET(r,n) \
    ((r) = ((r) & ~PCI_MSI_MSG_CTRL_MME) | PCI_MSI_MSG_CTRL_MME_n((n)))

// Multiple message capable (log2 N)
#define PCI_MSI_MSG_CTRL_MMC_SET(r,n) \
    ((r) = ((r) & ~PCI_MSI_MSG_CTRL_MMC) | PCI_MSI_MSG_CTRL_MMC_n((n)))

// Enable
#define PCI_MSI_MSG_CTRL_EN_SET(r,n) \
    ((r) = ((r) & ~PCI_MSI_MSG_CTRL_EN) | PCI_MSI_MSG_CTRL_EN_n((n)))

// PCI_MSIX_MSG_CTRL


// Enable
#define PCI_MSIX_MSG_CTRL_EN_BIT          15

// Function mask
#define PCI_MSIX_MSG_CTRL_MASK_BIT        14

// Table size
#define PCI_MSIX_MSG_CTRL_TBLSZ_BIT       0


// Enable
#define PCI_MSIX_MSG_CTRL_EN_BITS         1

// Function mask
#define PCI_MSIX_MSG_CTRL_MASK_BITS       1

// Table size
#define PCI_MSIX_MSG_CTRL_TBLSZ_BITS      11

// Enable
#define PCI_MSIX_MSG_CTRL_EN_MASK         ((1U << PCI_MSIX_MSG_CTRL_EN_BITS)-1)

// Function mask
#define PCI_MSIX_MSG_CTRL_MASK_MASK \
    ((1U << PCI_MSIX_MSG_CTRL_MASK_BITS)-1)

// Table size
#define PCI_MSIX_MSG_CTRL_TBLSZ_MASK \
    ((1U << PCI_MSIX_MSG_CTRL_TBLSZ_BITS)-1)

// Enable
#define PCI_MSIX_MSG_CTRL_EN \
    (PCI_MSIX_MSG_CTRL_EN_MASK << PCI_MSIX_MSG_CTRL_EN_BIT)

// Function mask
#define PCI_MSIX_MSG_CTRL_MASK \
    (PCI_MSIX_MSG_CTRL_MASK_MASK << PCI_MSIX_MSG_CTRL_MASK_BIT)

// Table size
#define PCI_MSIX_MSG_CTRL_TBLSZ \
    (PCI_MSIX_MSG_CTRL_TBLSZ_MASK << PCI_MSIX_MSG_CTRL_TBLSZ_BIT)


// Enable
#define PCI_MSIX_MSG_CTRL_EN_n(n)         ((n) << PCI_MSIX_MSG_CTRL_EN_BIT)

// Function mask
#define PCI_MSIX_MSG_CTRL_MASK_n(n)       ((n) << PCI_MSIX_MSG_CTRL_MASK_BIT)

// Table size
#define PCI_MSIX_MSG_CTRL_TBLSZ_n(n)      ((n) << PCI_MSIX_MSG_CTRL_TBLSZ_BIT)


// Enable
#define PCI_MSIX_MSG_CTRL_EN_GET(n) \
    (((n) >> PCI_MSIX_MSG_CTRL_EN_BIT) & PCI_MSIX_MSG_CTRL_EN_MASK)

// Function mask
#define PCI_MSIX_MSG_CTRL_MASK_GET(n) \
    (((n) >> PCI_MSIX_MSG_CTRL_MASK_BIT) & PCI_MSIX_MSG_CTRL_MASK_MASK)

// Table size
#define PCI_MSIX_MSG_CTRL_TBLSZ_GET(n) \
    (((n) >> PCI_MSIX_MSG_CTRL_TBLSZ_BIT) & PCI_MSIX_MSG_CTRL_TBLSZ_MASK)


// Enable
#define PCI_MSIX_MSG_CTRL_EN_SET(r,n) \
    ((r) = ((r) & ~PCI_MSIX_MSG_CTRL_EN) | PCI_MSIX_MSG_CTRL_EN_n((n)))

// Function mask
#define PCI_MSIX_MSG_CTRL_MASK_SET(r,n) \
    ((r) = ((r) & ~PCI_MSIX_MSG_CTRL_MASK) | PCI_MSIX_MSG_CTRL_MASK_n((n)))

// Table size
#define PCI_MSIX_MSG_CTRL_TBLSZ_SET(r,n) \
    ((r) = ((r) & ~PCI_MSIX_MSG_CTRL_TBLSZ) | PCI_MSIX_MSG_CTRL_TBLSZ_n((n)))

// PCI_MSIX_TBL
#define PCI_MSIX_TBL               4


// Table offset
#define PCI_MSIX_TBL_OFS_BIT       3

// BAR indicator register
#define PCI_MSIX_TBL_BIR_BIT       0


// Table offset
#define PCI_MSIX_TBL_OFS_BITS      29

// BAR indicator register
#define PCI_MSIX_TBL_BIR_BITS      3

// Table offset
#define PCI_MSIX_TBL_OFS_MASK      ((1U << PCI_MSIX_TBL_OFS_BITS)-1)

// BAR indicator register
#define PCI_MSIX_TBL_BIR_MASK      ((1U << PCI_MSIX_TBL_BIR_BITS)-1)

// Table offset
#define PCI_MSIX_TBL_OFS \
    (PCI_MSIX_TBL_OFS_MASK << PCI_MSIX_TBL_OFS_BIT)

// BAR indicator register
#define PCI_MSIX_TBL_BIR \
    (PCI_MSIX_TBL_BIR_MASK << PCI_MSIX_TBL_BIR_BIT)


// Table offset
#define PCI_MSIX_TBL_OFS_n(n)      ((n) << PCI_MSIX_TBL_OFS_BIT)

// BAR indicator register
#define PCI_MSIX_TBL_BIR_n(n)      ((n) << PCI_MSIX_TBL_BIR_BIT)


// Table offset
#define PCI_MSIX_TBL_OFS_GET(n) \
    (((n) >> PCI_MSIX_TBL_OFS_BIT) & PCI_MSIX_TBL_OFS_MASK)

// BAR indicator register
#define PCI_MSIX_TBL_BIR_GET(n) \
    (((n) >> PCI_MSIX_TBL_BIR_BIT) & PCI_MSIX_TBL_BIR_MASK)


// Table offset
#define PCI_MSIX_TBL_OFS_SET(r,n) \
    ((r) = ((r) & ~PCI_MSIX_TBL_OFS) | PCI_MSIX_TBL_OFS_n((n)))

// BAR indicator register
#define PCI_MSIX_TBL_BIR_SET(r,n) \
    ((r) = ((r) & ~PCI_MSIX_TBL_BIR) | PCI_MSIX_TBL_BIR_n((n)))

// PCI_MSIX_PBA
#define PCI_MSIX_PBA               8


// Table offset
#define PCI_MSIX_PBA_OFS_BIT       3

// BAR indicator register
#define PCI_MSIX_PBA_BIR_BIT       0


// Table offset
#define PCI_MSIX_PBA_OFS_BITS      29

// BAR indicator register
#define PCI_MSIX_PBA_BIR_BITS      3

// Table offset
#define PCI_MSIX_PBA_OFS_MASK      ((1U << PCI_MSIX_PBA_OFS_BITS)-1)

// BAR indicator register
#define PCI_MSIX_PBA_BIR_MASK      ((1U << PCI_MSIX_PBA_BIR_BITS)-1)

// Table offset
#define PCI_MSIX_PBA_OFS \
    (PCI_MSIX_PBA_OFS_MASK << PCI_MSIX_PBA_OFS_BIT)

// BAR indicator register
#define PCI_MSIX_PBA_BIR \
    (PCI_MSIX_PBA_BIR_MASK << PCI_MSIX_PBA_BIR_BIT)


// Table offset
#define PCI_MSIX_PBA_OFS_n(n)      ((n) << PCI_MSIX_PBA_OFS_BIT)

// BAR indicator register
#define PCI_MSIX_PBA_BIR_n(n)      ((n) << PCI_MSIX_PBA_BIR_BIT)


// Table offset
#define PCI_MSIX_PBA_OFS_GET(n) \
    (((n) >> PCI_MSIX_PBA_OFS_BIT) & PCI_MSIX_PBA_OFS_MASK)

// BAR indicator register
#define PCI_MSIX_PBA_BIR_GET(n) \
    (((n) >> PCI_MSIX_PBA_BIR_BIT) & PCI_MSIX_PBA_BIR_MASK)


// Table offset
#define PCI_MSIX_PBA_OFS_SET(r,n) \
    ((r) = ((r) & ~PCI_MSIX_PBA_OFS) | PCI_MSIX_PBA_OFS_n((n)))

// BAR indicator register
#define PCI_MSIX_PBA_BIR_SET(r,n) \
    ((r) = ((r) & ~PCI_MSIX_PBA_BIR) | PCI_MSIX_PBA_BIR_n((n)))

// PCI_MSIX_VEC_CTL
#define PCI_MSIX_VEC_CTL                   12


// 1=masked
#define PCI_MSIX_VEC_CTL_MASKIRQ_BIT       0


// 1=masked
#define PCI_MSIX_VEC_CTL_MASKIRQ_BITS      1

// 1=masked
#define PCI_MSIX_VEC_CTL_MASKIRQ_MASK \
    ((1U << PCI_MSIX_VEC_CTL_MASKIRQ_BITS)-1)

// 1=masked
#define PCI_MSIX_VEC_CTL_MASKIRQ \
    (PCI_MSIX_VEC_CTL_MASKIRQ_MASK << PCI_MSIX_VEC_CTL_MASKIRQ_BIT)


// 1=masked
#define PCI_MSIX_VEC_CTL_MASKIRQ_n(n) \
    ((n) << PCI_MSIX_VEC_CTL_MASKIRQ_BIT)


// 1=masked
#define PCI_MSIX_VEC_CTL_MASKIRQ_GET(n) \
    (((n) >> PCI_MSIX_VEC_CTL_MASKIRQ_BIT) & PCI_MSIX_VEC_CTL_MASKIRQ_MASK)


// 1=masked
#define PCI_MSIX_VEC_CTL_MASKIRQ_SET(r,n) \
    ((r) = ((r) & ~PCI_MSIX_VEC_CTL_MASKIRQ) | PCI_MSIX_VEC_CTL_MASKIRQ_n((n)))

// PCI_MSIX_EXTCAP

#define PCI_MSIX_EXTCAP_CAPID_BIT         0
#define PCI_MSIX_EXTCAP_CAPVER_BIT        16
#define PCI_MSIX_EXTCAP_NEXTOFS_BIT       20

#define PCI_MSIX_EXTCAP_CAPID_BITS        16
#define PCI_MSIX_EXTCAP_CAPVER_BITS       4
#define PCI_MSIX_EXTCAP_NEXTOFS_BITS      12
#define PCI_MSIX_EXTCAP_CAPID_MASK \
    ((1U << PCI_MSIX_EXTCAP_CAPID_BITS)-1)
#define PCI_MSIX_EXTCAP_CAPVER_MASK \
    ((1U << PCI_MSIX_EXTCAP_CAPVER_BITS)-1)
#define PCI_MSIX_EXTCAP_NEXTOFS_MASK \
    ((1U << PCI_MSIX_EXTCAP_NEXTOFS_BITS)-1)
#define PCI_MSIX_EXTCAP_CAPID \
    (PCI_MSIX_EXTCAP_CAPID_MASK << PCI_MSIX_EXTCAP_CAPID_BIT)
#define PCI_MSIX_EXTCAP_CAPVER \
    (PCI_MSIX_EXTCAP_CAPVER_MASK << PCI_MSIX_EXTCAP_CAPVER_BIT)
#define PCI_MSIX_EXTCAP_NEXTOFS \
    (PCI_MSIX_EXTCAP_NEXTOFS_MASK << PCI_MSIX_EXTCAP_NEXTOFS_BIT)

#define PCI_MSIX_EXTCAP_CAPID_n(n)        ((n) << PCI_MSIX_EXTCAP_CAPID_BIT)
#define PCI_MSIX_EXTCAP_CAPVER_n(n)       ((n) << PCI_MSIX_EXTCAP_CAPVER_BIT)
#define PCI_MSIX_EXTCAP_NEXTOFS_n(n)      ((n) << PCI_MSIX_EXTCAP_NEXTOFS_BIT)

#define PCI_MSIX_EXTCAP_CAPID_GET(n) \
    (((n) >> PCI_MSIX_EXTCAP_CAPID_BIT) & PCI_MSIX_EXTCAP_CAPID_MASK)
#define PCI_MSIX_EXTCAP_CAPVER_GET(n) \
    (((n) >> PCI_MSIX_EXTCAP_CAPVER_BIT) & PCI_MSIX_EXTCAP_CAPVER_MASK)
#define PCI_MSIX_EXTCAP_NEXTOFS_GET(n) \
    (((n) >> PCI_MSIX_EXTCAP_NEXTOFS_BIT) & PCI_MSIX_EXTCAP_NEXTOFS_MASK)

#define PCI_MSIX_EXTCAP_CAPID_SET(r,n) \
    ((r) = ((r) & ~PCI_MSIX_EXTCAP_CAPID) | PCI_MSIX_EXTCAP_CAPID_n((n)))
#define PCI_MSIX_EXTCAP_CAPVER_SET(r,n) \
    ((r) = ((r) & ~PCI_MSIX_EXTCAP_CAPVER) | PCI_MSIX_EXTCAP_CAPVER_n((n)))
#define PCI_MSIX_EXTCAP_NEXTOFS_SET(r,n) \
    ((r) = ((r) & ~PCI_MSIX_EXTCAP_NEXTOFS) | PCI_MSIX_EXTCAP_NEXTOFS_n((n)))

#define VIRTIO_STATUS_ACKNOWLEDGE   1
#define VIRTIO_STATUS_DRIVER        2
#define VIRTIO_STATUS_DRIVER_OK     4
#define VIRTIO_STATUS_FEATURES_OK   8
#define VIRTIO_STATUS_NEED_RESET    64
#define VIRTIO_STATUS_FAILED        128

// Common configuration
#define VIRTIO_PCI_CAP_COMMON_CFG   1

// Notifications
#define VIRTIO_PCI_CAP_NOTIFY_CFG   2

// ISR Status
#define VIRTIO_PCI_CAP_ISR_CFG      3

// Device specific configuration
#define VIRTIO_PCI_CAP_DEVICE_CFG   4

// PCI configuration access
#define VIRTIO_PCI_CAP_PCI_CFG      5

struct pci_cap_hdr_t {
    uint8_t cap_vendor;
    uint8_t cap_next;
    uint8_t cap_len;
    uint8_t type;
    uint8_t bar;
    uint8_t padding[3];
    uint32_t offset;
    uint32_t length;
};

#endif /* _DEV_PCI_PCIREG_H_ */
