/*
 * Copyright (C) 2017 Technologic Systems
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/boot_mode.h>
#include <asm/imx-common/mxc_i2c.h>
#include <asm/io.h>
#include <common.h>
#include <fsl_esdhc.h>
#include <i2c.h>
#include <lattice.h>
#include <linux/sizes.h>
#include <linux/fb.h>
#include <miiphy.h>
#include <mmc.h>
#include <mxsfb.h>
#include <netdev.h>
#include <usb.h>
#include <usb/ehci-fsl.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_22K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL_WP (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_DOWN | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define ENET_PAD_CTRL  (PAD_CTL_PUS_100K_UP | PAD_CTL_PUE |     \
	PAD_CTL_SPEED_HIGH   |                                   \
	PAD_CTL_DSE_48ohm   | PAD_CTL_SRE_FAST)

#define ENET_CLK_PAD_CTRL  (PAD_CTL_SPEED_MED | \
	PAD_CTL_DSE_120ohm   | PAD_CTL_SRE_FAST)

#define ENET_RX_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |          \
	PAD_CTL_SPEED_HIGH   | PAD_CTL_SRE_FAST)

#define I2C_PAD_CTRL    (PAD_CTL_PKE | PAD_CTL_PUE |            \
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |               \
	PAD_CTL_DSE_40ohm | PAD_CTL_HYS |			\
	PAD_CTL_ODE)

#define LCD_PAD_CTRL    (PAD_CTL_HYS | PAD_CTL_PUS_100K_UP | PAD_CTL_PUE | \
	PAD_CTL_PKE | PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm)

#define OTG_ID_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_47K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)

#define U_BOOT_JMPN		IMX_GPIO_NR(3, 19)
#define SD_BOOT_JMPN		IMX_GPIO_NR(3, 17)
#define PUSH_SW_CPUN		IMX_GPIO_NR(3, 18)
#define NO_CHRG_JMPN		IMX_GPIO_NR(3, 11)
#define EN_ETH_PHY_PWR 		IMX_GPIO_NR(2, 10)
#define ETH_PHY_RSTN 		IMX_GPIO_NR(1, 4)
#define PHY1_DUPLEX 		IMX_GPIO_NR(2, 0)
#define PHY1_PHYADDR2 		IMX_GPIO_NR(2, 1)
#define PHY1_CONFIG_2 		IMX_GPIO_NR(2, 2)
#define PHY1_ISOLATE		IMX_GPIO_NR(2, 7)
#define EN_SD_PWR		IMX_GPIO_NR(3, 12)
#define USDHC1_VSELECT		IMX_GPIO_NR(1, 5)

/* I2C1 for Silabs */
struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = MX6_PAD_GPIO1_IO02__I2C1_SCL | PC,
		.gpio_mode = MX6_PAD_GPIO1_IO02__GPIO1_IO02 | PC,
		.gp = IMX_GPIO_NR(1, 2),
	},
	.sda = {
		.i2c_mode = MX6_PAD_GPIO1_IO03__I2C1_SDA | PC,
		.gpio_mode = MX6_PAD_GPIO1_IO03__GPIO1_IO03 | PC,
		.gp = IMX_GPIO_NR(1, 3),
	},
};

/* I2C3 for FPGA/offbd */
struct i2c_pads_info i2c_pad_info3 = {
	.scl = {
		.i2c_mode = MX6_PAD_LCD_DATA01__I2C3_SCL | PC,
		.gpio_mode = MX6_PAD_LCD_DATA01__GPIO3_IO06 | PC,
		.gp = IMX_GPIO_NR(3, 6),
	},
	.sda = {
		.i2c_mode = MX6_PAD_LCD_DATA00__I2C3_SDA | PC,
		.gpio_mode = MX6_PAD_LCD_DATA00__GPIO3_IO05 | PC,
		.gp = IMX_GPIO_NR(3, 5),
	},
};

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_SIZE;

	return 0;
}

static iomux_v3_cfg_t const uart1_pads[] = {
	MX6_PAD_UART1_TX_DATA__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_UART1_RX_DATA__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const misc_pads[] = {
	MX6_PAD_LCD_DATA14__GPIO3_IO19 | MUX_PAD_CTRL(NO_PAD_CTRL), /* U_BOOT_JMP# */
	MX6_PAD_LCD_DATA12__GPIO3_IO17 | MUX_PAD_CTRL(NO_PAD_CTRL), /* SD_BOOT_JMP# */
	MX6_PAD_LCD_DATA13__GPIO3_IO18 | MUX_PAD_CTRL(NO_PAD_CTRL), /* PUSH_SW_CPU# */
	MX6_PAD_LCD_DATA06__GPIO3_IO11 | MUX_PAD_CTRL(NO_PAD_CTRL), /* NO_CHRG_JMP# */
};

static iomux_v3_cfg_t const usdhc1_sd_pads[] = {
	MX6_PAD_SD1_CLK__USDHC1_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_CMD__USDHC1_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA0__USDHC1_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA1__USDHC1_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA2__USDHC1_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA3__USDHC1_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_GPIO1_IO05__USDHC1_VSELECT | MUX_PAD_CTRL(NO_PAD_CTRL), /* VSELECT */
};

static iomux_v3_cfg_t const usdhc2_emmc_pads[] = {
	MX6_PAD_NAND_RE_B__USDHC2_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_WE_B__USDHC2_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA00__USDHC2_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA01__USDHC2_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA02__USDHC2_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA03__USDHC2_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

static struct fsl_esdhc_cfg usdhc_cfg[2] = {
	{USDHC1_BASE_ADDR, 0, 4},
	{USDHC2_BASE_ADDR, 0, 4},
};

int board_mmc_getcd(struct mmc *mmc)
{
	return 1;
}

int board_mmc_init(bd_t *bis)
{
	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-boot device node)    (Physical Port)
	 * mmc0                    USDHC1 (SD/WIFI)
	 * mmc1                    USDHC2 (eMMC)
	 */
	gpio_direction_output(EN_SD_PWR, 1);

	/* For the SD Select 3.3V instead of 1.8V */
	gpio_direction_output(USDHC1_VSELECT, 1);

	imx_iomux_v3_setup_multiple_pads(
		usdhc1_sd_pads, ARRAY_SIZE(usdhc1_sd_pads));
	usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);

	imx_iomux_v3_setup_multiple_pads(
		usdhc2_emmc_pads, ARRAY_SIZE(usdhc2_emmc_pads));
	usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);

	if (fsl_esdhc_initialize(bis, &usdhc_cfg[0]))
		printf("Warning: failed to initialize sd dev\n");

	if (fsl_esdhc_initialize(bis, &usdhc_cfg[1]))
		printf("Warning: failed to initialize emmc dev\n");

	return 0;
}

#ifdef CONFIG_FEC_MXC

static iomux_v3_cfg_t const fec_enet_pads[] = {
	MX6_PAD_GPIO1_IO06__ENET1_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_GPIO1_IO07__ENET1_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_DATA0__ENET1_TDATA00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_DATA1__ENET1_TDATA01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_EN__ENET1_TX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_CLK__ENET1_REF_CLK1 | MUX_PAD_CTRL(ENET_CLK_PAD_CTRL),
	MX6_PAD_ENET1_RX_DATA0__ENET1_RDATA00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_RX_DATA1__ENET1_RDATA01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_RX_ER__ENET1_RX_ER | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_RX_EN__ENET1_RX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
};

static iomux_v3_cfg_t const fec_enet_pads1[] = {
	MX6_PAD_GPIO1_IO06__ENET1_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_GPIO1_IO07__ENET1_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	/* DUPLEX */
	MX6_PAD_ENET1_RX_DATA0__GPIO2_IO00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	/* PHYADDR2 */
	MX6_PAD_ENET1_RX_DATA1__GPIO2_IO01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	/* CONFIG_2 */
	MX6_PAD_ENET1_RX_EN__GPIO2_IO02 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	/* Isolate */
	MX6_PAD_ENET1_RX_ER__GPIO2_IO07 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	/* EN_ETH_PHY_PWR */
	MX6_PAD_ENET2_RX_EN__GPIO2_IO10 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	/* ETH_PHY_RSTN */
	MX6_PAD_GPIO1_IO04__GPIO1_IO04 | MUX_PAD_CTRL(ENET_PAD_CTRL),
};

int board_eth_init(bd_t *bis)
{
	/* Set pins to strapping GPIO modes */
	imx_iomux_v3_setup_multiple_pads(fec_enet_pads1,
					 ARRAY_SIZE(fec_enet_pads1));

	/* Set up strapping pins */
	gpio_direction_output(PHY1_DUPLEX, 0);
	gpio_direction_output(PHY1_PHYADDR2, 0);
	gpio_direction_output(PHY1_CONFIG_2, 0);
	gpio_direction_output(PHY1_ISOLATE, 0);

	/* Reset */
	gpio_direction_output(ETH_PHY_RSTN, 0);
	gpio_direction_output(EN_ETH_PHY_PWR, 0);
	mdelay(5); // falls in ~2ms
	gpio_direction_output(EN_ETH_PHY_PWR, 1);

	mdelay(10);
	gpio_direction_output(ETH_PHY_RSTN, 1);

	/* Once reset is deasserted, strapping pins are latched in withing
	 * 1 ms.
	 */
	mdelay(1);

	/* Set pins to enet modes */
	imx_iomux_v3_setup_multiple_pads(fec_enet_pads,
					 ARRAY_SIZE(fec_enet_pads));

	return fecmxc_initialize_multi(bis, CONFIG_FEC_ENET_DEV,
				       CONFIG_FEC_MXC_PHYADDR, IMX_FEC_BASE);
}

static int setup_fec(int fec_id)
{
	struct iomuxc *const iomuxc_regs = (struct iomuxc *)IOMUXC_BASE_ADDR;
	int ret;

	if (check_module_fused(MX6_MODULE_ENET1))
		return -1;

	/*
	 * Use 50M anatop loopback REF_CLK1 for ENET1,
	 * clear gpr1[13], set gpr1[17].
	 */
	clrsetbits_le32(&iomuxc_regs->gpr[1], IOMUX_GPR1_FEC1_MASK,
			IOMUX_GPR1_FEC1_CLOCK_MUX1_SEL_MASK);

	ret = enable_fec_anatop_clock(0, ENET_50MHZ);
	if (ret)
		return ret;

	enable_enet_clk(1);

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	/* PHY addressing is a little odd, phy 2 is the only PHY */

	/* Reset PHY */
	phydev->bus->write(phydev->bus, 0x2, MDIO_DEVAD_NONE, 0x0, 0x8000);
	/* Disable BCAST, select RMII */
	phydev->bus->write(phydev->bus, 0x2, MDIO_DEVAD_NONE, 0x16, 0x202);
	/* Enable 50MHZ Clock */
	phydev->bus->write(phydev->bus, 0x2, MDIO_DEVAD_NONE, 0x1f, 0x8180);

	return 0;
}
#endif

int board_early_init_f(void)
{
	setup_iomux_uart();

	/* Enable LVDS clock output.
	 * Writing CCM_ANALOG_MISC1 to use output from 24M OSC */
	setbits_le32(0x020C8160, 0x412);

	return 0;
}

int misc_init_r(void)
{
	int jpr;

	/* Onboard jumpers to boot to SD or break in u-boot */
	gpio_direction_input(SD_BOOT_JMPN);
	gpio_direction_input(U_BOOT_JMPN);

	jpr = gpio_get_value(SD_BOOT_JMPN);
	if(jpr) setenv("jpsdboot", "off");
	else setenv("jpsdboot", "on");

	jpr = gpio_get_value(U_BOOT_JMPN);
	if(jpr) setenv("jpuboot", "off");
	else setenv("jpuboot", "on");

	setenv("model", "7553");

	return 0;
}

int board_init(void)
{
	/* Address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	#ifdef CONFIG_FEC_MXC
	setup_fec(CONFIG_FEC_ENET_DEV);
	#endif

	return 0;
}

int board_late_init(void)
{
	set_wdog_reset((struct wdog_regs *)WDOG1_BASE_ADDR);

	red_led_on();
	green_led_off();

	return 0;
}

u32 get_board_rev(void)
{
	return get_cpu_rev();
}

int checkboard(void)
{
	puts("Board: Technologic Systems TS-7553-V2\n");
	return 0;
}

#ifdef CONFIG_USB_EHCI_MX6
#define USB_OTHERREGS_OFFSET	0x800
#define UCTRL_PWR_POL		(1 << 9)
iomux_v3_cfg_t const usb_otg1_pads[] = {
	//MX6_PAD_GPIO1_IO04__USB_OTG1_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_GPIO1_IO00__ANATOP_OTG1_ID | MUX_PAD_CTRL(OTG_ID_PAD_CTRL),
};

int board_usb_phy_mode(int port)
{
	return USB_INIT_HOST;
}

int board_ehci_hcd_init(int port)
{
	u32 *usbnc_usb_ctrl;

	imx_iomux_v3_setup_multiple_pads(usb_otg1_pads,
					 ARRAY_SIZE(usb_otg1_pads));

	usbnc_usb_ctrl = (u32 *)(USB_BASE_ADDR + USB_OTHERREGS_OFFSET +
				 port * 4);

	/* Set Power polarity */
	setbits_le32(usbnc_usb_ctrl, UCTRL_PWR_POL);
	return 0;
}
#endif
