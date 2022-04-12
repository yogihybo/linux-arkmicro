#ifndef __MUSB_ARK_H__
#define __MUSB_ARK_H__

#define	ARK_SYSTEM_MODULE_BASE	(0xe0100000)

/* Integrated highspeed/otg PHY */
#define USBPHY_CTL_PADDR	(ARK_SYSTEM_MODULE_BASE + 0x34)
#define USBPHY_DATAPOL		BIT(11)	/* (ark) switch D+/D- */
#define USBPHY_PHYCLKGD		BIT(8)
#define USBPHY_SESNDEN		BIT(7)	/* v(sess_end) comparator */
#define USBPHY_VBDTCTEN		BIT(6)	/* v(bus) comparator */
#define USBPHY_VBUSSENS		BIT(5)	/* (ark,ro) is vbus > 0.5V */
#define USBPHY_PHYPLLON		BIT(4)	/* override pll suspend */
#define USBPHY_CLKO1SEL		BIT(3)
#define USBPHY_OSCPDWN		BIT(2)
#define USBPHY_OTGPDWN		BIT(1)
#define USBPHY_PHYPDWN		BIT(0)

#define ARK_DEEPSLEEP_PADDR	(ARK_SYSTEM_MODULE_BASE + 0x48)
#define DRVVBUS_FORCE		BIT(2)
#define DRVVBUS_OVERRIDE	BIT(1)

/* For now include usb OTG module registers here */
#define ARK_USB_VERSION_REG			0x00
#define ARK_USB_CTRL_REG			0x04
#define ARK_USB_STAT_REG			0x08
#define ARK_RNDIS_REG				0x10
#define ARK_AUTOREQ_REG				0x14
#define ARK_USB_INT_SOURCE_REG		0x20
#define ARK_USB_INT_SET_REG			0x24
#define ARK_USB_INT_SRC_CLR_REG		0x28
#define ARK_USB_INT_MASK_REG		0x2c
#define ARK_USB_INT_MASK_SET_REG	0x30
#define ARK_USB_INT_MASK_CLR_REG	0x34
#define ARK_USB_INT_SRC_MASKED_REG	0x38
#define ARK_USB_EOI_REG				0x3c
#define ARK_USB_EOI_INTVEC			0x40

/* BEGIN CPPI-generic (?) */

/* CPPI related registers */
#define ARK_TXCPPI_CTRL_REG		0x80
#define ARK_TXCPPI_TEAR_REG		0x84
#define ARK_CPPI_EOI_REG		0x88
#define ARK_CPPI_INTVEC_REG		0x8c
#define ARK_TXCPPI_MASKED_REG	0x90
#define ARK_TXCPPI_RAW_REG		0x94
#define ARK_TXCPPI_INTENAB_REG	0x98
#define ARK_TXCPPI_INTCLR_REG	0x9c

#define ARK_RXCPPI_CTRL_REG		0xC0
#define ARK_RXCPPI_MASKED_REG	0xD0
#define ARK_RXCPPI_RAW_REG		0xD4
#define ARK_RXCPPI_INTENAB_REG	0xD8
#define ARK_RXCPPI_INTCLR_REG	0xDC

#define ARK_RXCPPI_BUFCNT0_REG	0xE0
#define ARK_RXCPPI_BUFCNT1_REG	0xE4
#define ARK_RXCPPI_BUFCNT2_REG	0xE8
#define ARK_RXCPPI_BUFCNT3_REG	0xEC

/* CPPI state RAM entries */
#define ARK_CPPI_STATERAM_BASE_OFFSET   0x100

#define ARK_TXCPPI_STATERAM_OFFSET(chnum) \
	(ARK_CPPI_STATERAM_BASE_OFFSET +       ((chnum) * 0x40))
#define ARK_RXCPPI_STATERAM_OFFSET(chnum) \
	(ARK_CPPI_STATERAM_BASE_OFFSET + 0x20 + ((chnum) * 0x40))

/* CPPI masks */
#define ARK_DMA_CTRL_ENABLE		1
#define ARK_DMA_CTRL_DISABLE	0

#define ARK_DMA_ALL_CHANNELS_ENABLE	0xF
#define ARK_DMA_ALL_CHANNELS_DISABLE 0xF

/* END CPPI-generic (?) */

#define ARK_USB_TX_ENDPTS_MASK	0x1f		/* ep0 + 4 tx */
#define ARK_USB_RX_ENDPTS_MASK	0x1e		/* 4 rx */

#define ARK_USB_USBINT_SHIFT	16
#define ARK_USB_TXINT_SHIFT		0
#define ARK_USB_RXINT_SHIFT		8

#define ARK_INTR_DRVVBUS		0x0100

#define ARK_USB_USBINT_MASK		0x01ff0000	/* 8 Mentor, DRVVBUS */
#define ARK_USB_TXINT_MASK \
	(ARK_USB_TX_ENDPTS_MASK << ARK_USB_TXINT_SHIFT)
#define ARK_USB_RXINT_MASK \
	(ARK_USB_RX_ENDPTS_MASK << ARK_USB_RXINT_SHIFT)

#define ARK_BASE_OFFSET		0x0	

#endif	/* __MUSB_CORE_H__ */
