#define ARK_PLLREF_REG				0x14C


#define ARK_PLL_DIV_MASK			0xFF
#define ARK_PLL_DIV_OFFSET			0
#define ARK_PLL_NO_MASK				0x3
#define ARK_PLL_NO_OFFSET			12
#define ARK_PLL_ENA					(1 << 14)


#define ARK_DDS_DIV_MASK			0x7
#define ARK_DDS_DIV_OFFSET			10
#define ARK_DDS_DTOINC_MASK			0x3FFFFF
#define ARK_DDS_DTOINC_OFFSET		0
#define ARK_DDS_MUL_MASK			1
#define ARK_DDS_MUL_OFFSET			2
#define ARK_DDS_ENA					(1 << 18)
#define ARK_DDS_MIN_FREQ			200000000
#define ARK_DDS_MAX_FREQ			300000000


#define ARKE_SSCG_OD_MASK			0x7
#define ARKE_SSCG_OD_OFFSET			24
#define ARKE_SSCG_NR_MASK			0x7f
#define ARKE_SSCG_NR_OFFSET			15
#define ARKE_SSCG_NFF_MASK			0x7fff
#define ARKE_SSCG_NFF_OFFSET		0
#define ARKE_SSCG_NFX_MASK			0x1ff
#define ARKE_SSCG_NFX_OFFSET		15
#define ARKE_SSCG_ENA				(1 << 27)

#define ARKE_PLL_PS_MASK			0x1f
#define ARKE_PLL_PS_OFFSET			12
#define ARKE_PLL_NS_MASK			0x1ff
#define ARKE_PLL_NS_OFFSET			3
#define ARKE_PLL_MS_MASK			0x7
#define ARKE_PLL_MS_OFFSET			0
#define ARKE_PLL_ENA				(1 << 17)


#define ARK_CLK_MAX_ENABLE_NUM		4u

enum ARK_CLK_TYPE {
	ARK_CLK_TYPE_PLL = 0,
	ARK_CLK_TYPE_DDS,
	ARK_CLK_TYPE_DDR,
	ARK_CLK_TYPE_SYS,
	ARK_CLK_TYPE_APB,
	ARKE_CLK_TYPE_SSCG,
	ARKE_CLK_TYPE_PLL,
};

enum ARK_PLAT_TYPE {
	PLAT_ARK1668,
	PLAT_ARKN141,
};

struct ark_clk {
    struct clk_hw hw;
	void __iomem	 *reg;
	void __iomem	 *reg2;
	bool can_change;
	int div;
	int divmode;
	int divmask;
	int divoffset;
	int enable_num;
	void __iomem	*enable_reg[ARK_CLK_MAX_ENABLE_NUM];
	int	enable_offset[ARK_CLK_MAX_ENABLE_NUM];
	char *parent_name;
};
#define to_ark_clk(p) container_of(p, struct ark_clk, hw)

