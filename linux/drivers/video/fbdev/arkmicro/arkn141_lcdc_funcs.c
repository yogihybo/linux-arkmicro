/*
 * Arkmicro arkn141_lcdc_funcs.c driver
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/kernel.h>
#include <linux/fb.h>
#include <linux/poll.h>
#include "arkn141_lcdc.h"

#define ARKN141_LCDC_DEBUG(fmt,arg...)		printk(KERN_DEBUG "### %s--> "fmt,__FUNCTION__,##arg)
#define ARKN141_LCDC_ERR(fmt,arg...)		printk(KERN_ERR "###ERR %s--> "fmt,__FUNCTION__,##arg)

static unsigned int arkn141_lcdc_width = 0;
static unsigned int arkn141_lcdc_height = 0;
static int arkn141_lcdc_mode[OSD_LAYER_MAX] = {-1, -1, -1};
static void *lcdc_base = NULL;

static struct arkn141_lcdfb_info *lcdfb_info = NULL;
extern volatile int arkn141_lcdc_frame_sync;

/* synchronizing lcdc osd register value */
static int arkn141_lcdc_osd_set_coef_sync(unsigned int layer)
{
	unsigned int val = -1;

	if (!ARKN141_LAYER_IS_VALID(layer)) {
		ARKN141_LCDC_ERR("invalid osd layer:%d\n", layer);
		return -1;
	}

	val = readl(lcdc_base + ARKN141_LCDC_OSD_COEF_SYNC);

	switch(layer)
	{
		case OSD_LAYER0: {
			val |= (1<<0);
			//val |= (1<<0) | (1<<3);
			break;
		}
		case OSD_LAYER1: {
			val |= (1<<1);
			//val |= (1<<1) | (1<<4);
			break;
		}
		case OSD_LAYER2: {
			val |= (1<<2) ;
			//val |= (1<<2) | (1<<4);
			break;
		}
		case OSD_LAYER_MAX: {
			val |= 0x7;
			//val |= 0x3F;
			break;
		}
		default: {
			return -1;
			//break;
		}
	}
	if(val > 0)
		writel(val, lcdc_base + ARKN141_LCDC_OSD_COEF_SYNC);

	return 0;
}

static void arkn141_lcdc_set_osd_size(int layer, int width, int height)
{
	unsigned int val;
	int size_reg = -1, source_size_reg = -1;

	if (!ARKN141_LAYER_IS_VALID(layer)) {
		ARKN141_LCDC_ERR("invalid osd layer:%d\n", layer);
		return;
	}

	if((width <= 0) || (width > 0xFFF)) {
		ARKN141_LCDC_ERR("invalid width:%d\n", width);
		return ;
	}

	if((height <= 0) || (height > 0xFFF)) {
		ARKN141_LCDC_ERR("invalid height:%d\n", height);
		return ;
	}

	ARKN141_LCDC_DEBUG("width:%d height:%d\n", width, height);
	switch (layer) {
		case OSD_LAYER0:
			size_reg = ARKN141_LCDC_OSD0_PARAM0;
			source_size_reg = ARKN141_LCDC_OSD0_PARAM5;
			break;
		case OSD_LAYER1:
			size_reg = ARKN141_LCDC_OSD1_PARAM0;
			source_size_reg = ARKN141_LCDC_OSD1_PARAM5;
			break;
		case OSD_LAYER2:
			size_reg = ARKN141_LCDC_OSD2_PARAM0;
			source_size_reg = ARKN141_LCDC_OSD2_PARAM5;
			break;
		default:
			break;
	}
	if(size_reg > 0) {
		val = readl(lcdc_base + size_reg);
		val &= ~((0xFFF<<6) | (0xFFF<<18));
		val |= ((width<<6) | (height<<18));
		writel(val, lcdc_base + size_reg);
	}
	if(source_size_reg > 0) {
		val = readl(lcdc_base + source_size_reg);
		val &= ~(0xFFF);
		val |= width;
		writel(val, lcdc_base + source_size_reg);
	}
}

static int arkn141_lcdc_get_osd_size(int layer, int *width, int *height)
{
	unsigned int val;
	int reg = -1;
	int ret = -1;

	if (!ARKN141_LAYER_IS_VALID(layer)) {
		ARKN141_LCDC_ERR("invalid osd layer:%d\n", layer);
		return ret;
	}

	switch (layer) {
		case OSD_LAYER0:
			reg = ARKN141_LCDC_OSD0_PARAM0;
			break;
		case OSD_LAYER1:
			reg = ARKN141_LCDC_OSD1_PARAM0;
			break;
		case OSD_LAYER2:
			reg = ARKN141_LCDC_OSD2_PARAM0;
			break;
		default:
			break;
	}
	if(reg > 0) {
		val = readl(lcdc_base + reg);
		*width = (val>>6)&0xFFF;
		*height = (val>>18)&0xFFF;
		ret = 0;
	}

	return ret;
}


static void arkn141_lcdc_set_osd_pos(int layer, int x, int y)
{
	int reg = -1;
	unsigned int val;

	if (!ARKN141_LAYER_IS_VALID(layer)) {
		ARKN141_LCDC_ERR("invalid osd layer:%d\n", layer);
		return;
	}

	if ((x < 0) || (x > 0xfff)) {
		ARKN141_LCDC_ERR("invalid h_position:%d\n", x);
		return ;
	}

	if ((y < 0) || (y > 0xfff)) {
		ARKN141_LCDC_ERR("invalid h_position:%d\n", y);
		return ;
	}

	switch (layer) {
		case OSD_LAYER0:
			reg = ARKN141_LCDC_OSD0_PARAM1;
			break;
		case OSD_LAYER1:
			reg = ARKN141_LCDC_OSD1_PARAM1;
			break;
		case OSD_LAYER2:
			reg = ARKN141_LCDC_OSD2_PARAM1;
			break;
		default:
			break;
	}

	if(reg > 0) {
		val =  readl(lcdc_base + reg);
		val &= 0xFF000000;
		val |= (y << 12) | x;
		writel(val, lcdc_base + reg);
	}
}

static void arkn141_lcdc_set_alpha_blend_enable(int layer, int enable)
{
	int reg = -1;
	unsigned int val;

	if (!ARKN141_LAYER_IS_VALID(layer)) {
		ARKN141_LCDC_ERR("invalid osd layer:%d\n", layer);
		return;
	}

	switch (layer) {
		case OSD_LAYER0:
			reg = ARKN141_LCDC_OSD0_PARAM0;
			break;
		case OSD_LAYER1:
			reg = ARKN141_LCDC_OSD1_PARAM0;
			break;
		case OSD_LAYER2:
			reg = ARKN141_LCDC_OSD2_PARAM0;
			break;
		default:
			break;
	}

	if(reg > 0) {
		val = readl(lcdc_base + reg);
		if(enable)
			val |= (1<<0);
		else
			val &= ~(1<<0);
		writel(val, lcdc_base + reg);
	}
}

static void arkn141_lcdc_set_alpha_blend_value(int layer, int value)
{
	int reg = -1;
	unsigned int val;

	if (!ARKN141_LAYER_IS_VALID(layer)) {
		ARKN141_LCDC_ERR("invalid osd layer:%d\n", layer);
		return;
	}

	switch (layer) {
		case OSD_LAYER0:
			reg = ARKN141_LCDC_OSD0_PARAM1;
			break;
		case OSD_LAYER1:
			reg = ARKN141_LCDC_OSD1_PARAM1;
			break;
		case OSD_LAYER2:
			reg = ARKN141_LCDC_OSD2_PARAM1;
			break;
		default:
			break;
	}

	if(reg > 0) {
		val = readl(lcdc_base + reg);
		val &= ~(0x7F<<24);
		val |= (val << 24);
		writel(val, lcdc_base + reg);
	}
}

static char* arkn141_lcdc_get_osd_fomat_string(int format)
{
	switch(format) {
		case ARKN141_LCDC_FORMAT_YUV420:
			return "YUV420";
		case ARKN141_LCDC_FORMAT_ARGB888:
			return "ARGB888";
		case ARKN141_LCDC_FORMAT_RGB565:
			return "RGB565";
		case ARKN141_LCDC_FORMAT_RGB454:
			return "RGB454";
		case ARKN141_LCDC_FORMAT_AYUV444:
			return "AYUV444";
		case ARKN141_LCDC_FORMAT_Y_UV420:
			return "Y_UV420";
		default:
			break;
	}
	return "NONE";
}

static void arkn141_lcdc_set_osd_format(int layer, int format, int yuv_order, int rgb_order)
{
	int reg = -1;
	unsigned int val;

	if (!ARKN141_LAYER_IS_VALID(layer)) {
		ARKN141_LCDC_ERR("invalid osd layer:%d\n", layer);
		return;
	}

	switch(format) {
		case ARK_LCDC_FORMAT_YUV420:
			format = ARKN141_LCDC_FORMAT_YUV420;
			break;
		case ARK_LCDC_FORMAT_RGB888:
		case ARK_LCDC_FORMAT_RGBA888:
			format = ARKN141_LCDC_FORMAT_ARGB888;
			break;
		case ARK_LCDC_FORMAT_R5G6B5:
			format = ARKN141_LCDC_FORMAT_RGB565;
			break;
		case ARK_LCDC_FORMAT_Y_UV420:
			format = ARKN141_LCDC_FORMAT_Y_UV420;
			break;
		//case ARK_LCDC_FORMAT_YUV422:
		//case ARK_LCDC_FORMAT_VYUY:
		//case ARK_LCDC_FORMAT_YUV:
		//case ARK_LCDC_FORMAT_RGBI555:
		//case ARK_LCDC_FORMAT_Y_UV422:
		default:
			format = ARKN141_LCDC_FORMAT_END;
			break;
	}

	if((format < ARKN141_LCDC_FORMAT_YUV420) || (format >= ARKN141_LCDC_FORMAT_END)) {
		ARKN141_LCDC_ERR("invalid osd format:%d\n", format);
		return;
	}

	ARKN141_LCDC_DEBUG("osd[%d] format:%s\n", layer, arkn141_lcdc_get_osd_fomat_string(format));

	switch (layer) {
		case OSD_LAYER0:
			reg = ARKN141_LCDC_OSD0_PARAM0;
			arkn141_lcdc_mode[layer] = format;
			break;
		case OSD_LAYER1:
			reg = ARKN141_LCDC_OSD1_PARAM0;
			arkn141_lcdc_mode[layer] = format;
			if(format == ARKN141_LCDC_FORMAT_ARGB888) {
				arkn141_lcdc_set_alpha_blend_enable(layer, 0);
				//arkn141_lcdc_set_alpha_blend_value(layer, 0xFF);
			}
			break;
		case OSD_LAYER2:
			reg = ARKN141_LCDC_OSD2_PARAM0;
			arkn141_lcdc_mode[layer] = format;
			if(format == ARKN141_LCDC_FORMAT_ARGB888) {
				arkn141_lcdc_set_alpha_blend_enable(layer, 0);
				//arkn141_lcdc_set_alpha_blend_value(layer, 0xFF);
			}
			break;
		default:
			break;
	}

	if(format == ARKN141_LCDC_FORMAT_Y_UV420)
		format = ARKN141_LCDC_FORMAT_YUV420;
	if(reg >= 0) {
		val = readl(lcdc_base + reg);
		val &= ~(0x3<<3);
		val |= ((format&0x3)<<3);
		writel(val, lcdc_base + reg);
	}

	val = readl(lcdc_base + ARKN141_LCDC_OSD_012_PARAM);
	//yuv order
	val &= ~(0x3<<(12+(layer<<1)));
	if((yuv_order == ARK_LCDC_ORDER_UYVY) || (yuv_order == ARK_LCDC_ORDER_YUYV)) {
		if(arkn141_lcdc_mode[layer] == ARKN141_LCDC_FORMAT_Y_UV420)
			val |= (0x3<<(12+(layer<<1)));// 如设为此值， 红蓝交换颜色(交换UV数据的位置)  Y_UV420
		else if(arkn141_lcdc_mode[layer] == ARKN141_LCDC_FORMAT_YUV420)
			val |= (0x2<<(12+(layer<<1)));
	} else if((yuv_order == ARK_LCDC_ORDER_VYUY) || (yuv_order == ARK_LCDC_ORDER_YVYU)) {
		if(arkn141_lcdc_mode[layer] == ARKN141_LCDC_FORMAT_Y_UV420)
			val |= (0x1<<(12+(layer<<1)));
		else if(arkn141_lcdc_mode[layer] == ARKN141_LCDC_FORMAT_YUV420)
			val |= (0x0<<(12+(layer<<1)));
	}
	//rgb order
	val &= ~(0xF<<(layer*4));
	val |= rgb_order;
	writel(val, lcdc_base + ARKN141_LCDC_OSD_012_PARAM);
}

static void arkn141_lcdc_set_osd_addr(int layer, struct ark_disp_addr addr)
{
	unsigned int y = 0,u = 0,v = 0;

	if (!ARKN141_LAYER_IS_VALID(layer)) {
		ARKN141_LCDC_ERR("invalid osd layer:%d\n", layer);
		return;
	}

	if(!addr.yaddr) {
		ARKN141_LCDC_ERR("invalid y_addr:0x%x\n", addr.yaddr);
		return;
	}

	y = addr.yaddr;
	if(arkn141_lcdc_mode[layer] == ARKN141_LCDC_FORMAT_Y_UV420) {
		if(!addr.cbaddr) {
			int w, h;
			if(arkn141_lcdc_get_osd_size(layer, &w, &h) == 0) {
				u = addr.yaddr + w * h;
				ARKN141_LCDC_DEBUG("osd(%d) mode(%s) invalid cbaddr:0x%x, calculate new addr:0x%x\n",layer, arkn141_lcdc_get_osd_fomat_string(arkn141_lcdc_mode[layer]), addr.cbaddr, u);
			} else {
				ARKN141_LCDC_ERR("osd(%d) mode(%s) invalid cbaddr:0x%x\n",layer, arkn141_lcdc_get_osd_fomat_string(arkn141_lcdc_mode[layer]), addr.cbaddr);
			}
		} else {
			u = addr.cbaddr;
		}
		v = 0;
	} else if(arkn141_lcdc_mode[layer] == ARKN141_LCDC_FORMAT_YUV420) {
		if(!addr.cbaddr) {
			int w, h;
			if(arkn141_lcdc_get_osd_size(layer, &w, &h) == 0) {
				u = addr.yaddr + w * h;
				ARKN141_LCDC_DEBUG("osd(%d) mode(%s) invalid cbaddr:0x%x, calculate new addr:0x%x\n",layer, arkn141_lcdc_get_osd_fomat_string(arkn141_lcdc_mode[layer]), addr.cbaddr, u);
			} else {
				ARKN141_LCDC_ERR("osd(%d) mode(%s) invalid cbaddr:0x%x\n",layer, arkn141_lcdc_get_osd_fomat_string(arkn141_lcdc_mode[layer]), addr.cbaddr);
			}
		} else {
			u = addr.cbaddr;
		}
		if(!addr.craddr) {
			int w, h;
			if(arkn141_lcdc_get_osd_size(layer, &w, &h) == 0) {
				v = addr.yaddr + w * h *5 /4;
				ARKN141_LCDC_DEBUG("osd(%d) mode(%s) invalid craddr:0x%x, calculate new addr:0x%x\n",layer, arkn141_lcdc_get_osd_fomat_string(arkn141_lcdc_mode[layer]), addr.craddr, v);
			} else {
				ARKN141_LCDC_ERR("osd(%d) mode(%s) invalid craddr:0x%x\n",layer, arkn141_lcdc_get_osd_fomat_string(arkn141_lcdc_mode[layer]), addr.craddr);
			}
		} else {
			v = addr.craddr;
		}
	} else {
		u = 0;
		v = 0;
	}

	switch (layer)
	{
		case OSD_LAYER0:
			writel(y, lcdc_base + ARKN141_LCDC_OSD0_PARAM2);
			writel(u, lcdc_base + ARKN141_LCDC_OSD0_PARAM3);
			writel(v, lcdc_base + ARKN141_LCDC_OSD0_PARAM4);
			break;
		case OSD_LAYER1:
			writel(y, lcdc_base + ARKN141_LCDC_OSD1_PARAM2);
			writel(u, lcdc_base + ARKN141_LCDC_OSD1_PARAM3);
			writel(v, lcdc_base + ARKN141_LCDC_OSD1_PARAM4);
			break;
		case OSD_LAYER2:
			writel(y, lcdc_base + ARKN141_LCDC_OSD2_PARAM2);
			writel(u, lcdc_base + ARKN141_LCDC_OSD2_PARAM3);
			writel(v, lcdc_base + ARKN141_LCDC_OSD2_PARAM4);
			break;
		default:
			break;
	}
}


static int arkn141_lcdc_get_osd_addr(int layer, unsigned int *y, unsigned int *u, unsigned int *v)
{
	if (!ARKN141_LAYER_IS_VALID(layer)) {
		ARKN141_LCDC_ERR("invalid osd layer:%d\n", layer);
		return -EFAULT;
	}
    
	switch (layer) {
		case OSD_LAYER0:
			*y = readl(lcdc_base + ARKN141_LCDC_OSD0_PARAM2);
			*u = readl(lcdc_base + ARKN141_LCDC_OSD0_PARAM3);
			*v = readl(lcdc_base + ARKN141_LCDC_OSD0_PARAM4);
			break;
		case OSD_LAYER1:
			*y = readl(lcdc_base + ARKN141_LCDC_OSD1_PARAM2);
			*u = readl(lcdc_base + ARKN141_LCDC_OSD1_PARAM3);
			*v = readl(lcdc_base + ARKN141_LCDC_OSD1_PARAM4);
			break;
		case OSD_LAYER2:
			*y = readl(lcdc_base + ARKN141_LCDC_OSD2_PARAM2);
			*u = readl(lcdc_base + ARKN141_LCDC_OSD2_PARAM3);
			*v = readl(lcdc_base + ARKN141_LCDC_OSD2_PARAM4);
			break;
		default:
			break;
	}

	return 0;
}

static void  arkn141_lcdc_set_enbale(int enable)
{
	unsigned int val;

	val = readl(lcdc_base + ARKN141_LCDC_PARAM0);
	if(enable) {
		val |= (1<<0);
	} else {
		val &= ~(1<<0);
	}
	writel(val, lcdc_base + ARKN141_LCDC_PARAM0);
}


static void arkn141_lcdc_set_osd_enable(int layer, int enable)
{
	unsigned int val;
	int reg;	

	if (!ARKN141_LAYER_IS_VALID(layer)) {
		ARKN141_LCDC_ERR("invalid osd layer:%d\n", layer);
		return;
	}

	switch(layer)
	{
		case OSD_LAYER0: {
			reg = ARKN141_LCDC_OSD0_PARAM0;
			break;
		}
		case OSD_LAYER1: {
			reg = ARKN141_LCDC_OSD1_PARAM0;
			break;
		}
		case OSD_LAYER2: {
			reg = ARKN141_LCDC_OSD2_PARAM0;
			break;
		}
		default: {
			break;
		}
	}

	//lcd osd enable/disable
	val = readl(lcdc_base + reg);
	if(enable)
		val |= (1<<1);
	else
		val &= ~(1<<1);
	writel(val, lcdc_base + reg);

#if 0
	//lcdc enable/disable
	if(enable) {
		arkn141_lcdc_set_enbale(1);
	} else {
		int lcdc_disable = 0;
		lcdc_disable += (readl(lcdc_base + ARKN141_LCDC_OSD0_PARAM0)>>1) & 0x1;
		lcdc_disable += (readl(lcdc_base + ARKN141_LCDC_OSD1_PARAM0)>>1) & 0x1;
		lcdc_disable += (readl(lcdc_base + ARKN141_LCDC_OSD2_PARAM0)>>1) & 0x1;
		if(lcdc_disable == 0) {
			arkn141_lcdc_set_enbale(0);
		}
	}
#endif
}

void arkn141_lcdc_display_update_atomic(struct arkn141_lcdfb_info* sinfo)
{
	unsigned int format, yuv_order, rgb_order, i, layer;
	struct ark_disp_atomic* p = NULL;

	if(!sinfo || !sinfo->atomic_flag) {
		ARKN141_LCDC_ERR("!sinfo || !sinfo->atomic_flag\n");
		return;
	}

	for(i = 0; i < OSD_LAYER_MAX; i++){
		if(!(sinfo->atomic_flag & (1 << i)))
			continue;

		p = &sinfo->patomic[i];
		if(!p->atomic_stat || !ARKN141_LAYER_IS_VALID(p->layer)) {
			sinfo->atomic_flag &= ~(1 << i);
			memset(&sinfo->patomic[i], 0 ,sizeof(struct ark_disp_atomic));
			continue;
		}

		//printk(KERN_ALERT "%s: atomic_stat=0x%0x, layer=%d.\n ",__func__, p->atomic_stat, p->layer);
		if(ARKN141_LAYER_IS_VALID(p->layer)){
			layer = p->layer;
			if(p->atomic_stat & ATOMIC_SET_LAYER_POS)
				arkn141_lcdc_set_osd_pos(layer, p->pos_x, p->pos_y);
			if(p->atomic_stat & ATOMIC_SET_LAYER_SIZE)
				arkn141_lcdc_set_osd_size(layer, p->width, p->height);
			if(p->atomic_stat & ATOMIC_SET_LAYER_FMT){
				format    = (p->format >> 0)  & 0xFF;
				yuv_order = (p->format >> 16) & 0xF;
				rgb_order = (p->format >> 24) & 0xF;
				arkn141_lcdc_set_osd_format(layer, format, yuv_order, rgb_order);
				ARKN141_LCDC_DEBUG("format=%d, yuv_order=%d, rgb_order=%d\n", format, yuv_order, rgb_order);
			}
			if(p->atomic_stat & ATOMIC_SET_LAYER_ADDR)
				arkn141_lcdc_set_osd_addr(layer, p->addr);
			arkn141_lcdc_osd_set_coef_sync(layer);
		}
		sinfo->atomic_flag &= ~(1 << i);
		memset(&sinfo->patomic[i], 0 ,sizeof(struct ark_disp_atomic));
	}
}

int arkn141_lcdc_wait_for_vsync(void)
{
	struct arkn141_lcdfb_info *sinfo = lcdfb_info;
	int ret;

	if(!sinfo)
		return -EINVAL;

	sinfo->vsync_flag = 0;
	ret = wait_event_interruptible_timeout(sinfo->vsync_waitq,
			sinfo->vsync_flag != 0,
			msecs_to_jiffies(100)); // 100ms at most
	if (ret < 0)
		return ret;
	if (ret == 0)
		return -ETIMEDOUT;

	if(sinfo->atomic_flag)
		arkn141_lcdc_display_update_atomic(sinfo);

	return 0;
}
EXPORT_SYMBOL(arkn141_lcdc_wait_for_vsync);

int arkn141_lcdfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct arkn141_lcdfb_info *sinfo = NULL;
	int error = 0;
	int layer;
	
	if(!info) {
		ARKN141_LCDC_ERR("fb_info == NULL\n");
		return -1;
	}
	sinfo = info->par;
	layer = info->node;

	mutex_lock(&sinfo->mutex_lock);
	switch (cmd) {
		case ARKFB_GET_VSYNC_STATUS: {
			u32 vsync;
			spin_lock(&sinfo->lock);
			vsync = sinfo->frame_vsync;
			spin_unlock(&sinfo->lock);
			if(copy_to_user((void  *)arg, &vsync, sizeof(u32))) {
				ARKN141_LCDC_ERR("ARKFB_GET_REG_VALUE copy to user para error\n");
				error = -EFAULT;
				goto end;
			}
			break;
		}
		case ARKFB_WAITFORVSYNC: {
			error = arkn141_lcdc_wait_for_vsync();
			break;
		}
		case ARKFB_SHOW_WINDOW: {
			arkn141_lcdc_set_osd_enable(layer, 1);
			arkn141_lcdc_osd_set_coef_sync(layer);
			ARKN141_LCDC_DEBUG("ARKFB_SHOW_WINDOW osd layer=%d: show window.\n", layer);
			break;
		}
		case ARKFB_HIDE_WINDOW: {
			arkn141_lcdc_set_osd_enable(layer, 0);
			arkn141_lcdc_osd_set_coef_sync(layer);
			ARKN141_LCDC_DEBUG("ARKFB_HIDE_WINDOW osd layer=%d: hide window.\n", layer);
			break;
		}
		case ARKFB_SET_WINDOW_POS: {
			unsigned int x,y,data;
			if(copy_from_user(&data, (void *)arg, sizeof(unsigned int))) {
				ARKN141_LCDC_ERR("ARKFB_SET_WINDOW_POS copy from user para error\n");
				error = -EFAULT;
				goto end;
			}
			x =  data & 0xFFFF;
			y = (data >> 16) & 0xFFFF;
			arkn141_lcdc_set_osd_pos(layer, x, y);
			arkn141_lcdc_osd_set_coef_sync(layer);
			ARKN141_LCDC_DEBUG("osd layer=%d, x=%d, y=%d.\n", layer, x, y);
			break;
		}
		case ARKFB_SET_WINDOW_SIZE:
		{
			unsigned int width, height, data;
			if(copy_from_user(&data, (void *)arg, sizeof(unsigned int))) {
				ARKN141_LCDC_ERR("ARKFB_SET_WINDOW_SIZE copy from user para error\n");
				error = -EFAULT;
				goto end;
			}
			width  = data & 0xFFFF;
			height = (data >> 16) & 0xFFFF;
			arkn141_lcdc_set_osd_size(layer, width, height);
			arkn141_lcdc_osd_set_coef_sync(layer);
			ARKN141_LCDC_DEBUG("ARKFB_SET_WINDOW_SIZE osd layer=%d, width=%d, height=%d.\n", layer, width, height);
			break;
		}
		case ARKFB_SET_WINDOW_FORMAT:
		{
			unsigned int data, format, yuv_order, rgb_order;
			if(copy_from_user(&data, (void *)arg, sizeof(unsigned int))){
				ARKN141_LCDC_ERR("ARKFB_SET_WINDOW_FORMAT copy from user para error\n");
				error = -EFAULT;
				goto end;
			}

			format    = (data >> 0)  & 0xFF;
			yuv_order = (data >> 16) & 0xF;
			rgb_order = (data >> 24) & 0xF;
			arkn141_lcdc_set_osd_format(layer, format, yuv_order, rgb_order);
			arkn141_lcdc_osd_set_coef_sync(layer);
			ARKN141_LCDC_DEBUG("osd layer=%d, format=%d, yuv_order:%d, rgb_order:%d\n", layer, format, yuv_order, rgb_order);
			break;
		}
		case ARKFB_SET_WINDOW_ADDR:
		{
			struct ark_disp_addr addr;
			if(copy_from_user(&addr, (void *)arg, sizeof(struct ark_disp_addr))){
				ARKN141_LCDC_ERR("ARKFB_SET_WINDOW_ADDR copy from user para error\n");
				error = -EFAULT;
				goto end;
			}
			arkn141_lcdc_set_osd_addr(layer, addr);
			arkn141_lcdc_osd_set_coef_sync(layer);
			spin_lock(&sinfo->lock);
			sinfo->frame_vsync = 0;
			spin_unlock(&sinfo->lock);
			//ARKN141_LCDC_DEBUG("ARKFB_SET_WINDOW_ADDR osd layer=%d, yaddr=0x%0x, cbaddr=0x%0x, craddr=0x%0x\n", layer, addr.yaddr, addr.cbaddr, addr.craddr);
			break;
		}
		case ARKFB_SET_WINDOW_SCALER:
		{
			error = -ENXIO;
			ARKN141_LCDC_ERR("ARKFB_SET_WINDOW_SCALER Not support window scaler\n");
			break;
		}
		case ARKFB_SET_WINDOW_ATOMIC:
		{
			struct ark_disp_atomic atomic;

			if(copy_from_user(&atomic, (void *)arg, sizeof(struct ark_disp_atomic))){
				ARKN141_LCDC_ERR("ARKFB_SET_WINDOW_ATOMIC copy from user para error\n");
				error = -EFAULT;
				goto end;
			}
			if(!atomic.atomic_stat || atomic.layer != layer){
				ARKN141_LCDC_ERR("ARKFB_SET_WINDOW_ATOMIC atomic_stat or layer  error\n");
				error = -EFAULT;
				goto end;
			}
			ARKN141_LCDC_DEBUG("ARKFB_SET_WINDOW_ATOMIC osd layer=%d, atomic_stat=0x%0x\n", layer, atomic.atomic_stat);
			sinfo->atomic_flag |= (1 << layer);
			memcpy(&sinfo->patomic[layer], &atomic, sizeof(struct ark_disp_atomic));
			arkn141_lcdc_wait_for_vsync();
			break;
		}
		case ARKFB_SET_REG_VALUE:
		{
			struct ark_disp_reg reg;

			if(copy_from_user(&reg, (void *)arg, sizeof(struct ark_disp_reg))){
				ARKN141_LCDC_ERR("ARKFB_SET_REG_VALUE copy from user para error\n");
				error = -EFAULT;
				goto end;
			}

			if((reg.addr & 0xffff0000) == 0x70190000){
				writel(reg.value, lcdc_base + (reg.addr&0xfff));
				ARKN141_LCDC_DEBUG("ARKFB_SET_REG_VALUE arkfb write reg:0x%0x=0x%0x\n", reg.addr, reg.value);
			}else{
				error = -EINVAL;
				goto end;
			}
			break;
		}
		case ARKFB_GET_REG_VALUE:
		{
			struct ark_disp_reg reg;

			if(copy_from_user(&reg, (void *)arg, sizeof(struct ark_disp_reg))) {
				ARKN141_LCDC_ERR("ARKFB_GET_REG_VALUE copy from user para error\n");
				error = -EFAULT;
				goto end;
			}

			if((reg.addr & 0xffff0000) == 0x70190000) {        
				reg.value = readl(lcdc_base + (reg.addr&0xfff));
				ARKN141_LCDC_DEBUG("ARKFB_GET_REG_VALUE arkfb read reg:0x%0x=0x%0x\n", reg.addr, reg.value);
			} else {
				error = -EINVAL;
				goto end;
			}

			if(copy_to_user((void  *)arg, &reg, sizeof(struct ark_disp_reg))) {
				ARKN141_LCDC_ERR("ARKFB_GET_REG_VALUE copy to user para error\n");
				error = -EFAULT;
				goto end;
			}
			break;
		}
		case ARKFB_GET_WINDOW_ADDR:
		{
			struct ark_disp_addr addr;
			memset(&addr, 0, sizeof(struct ark_disp_addr));

			error = arkn141_lcdc_get_osd_addr(layer, &addr.yaddr, &addr.cbaddr, &addr.craddr);
			if(error < 0) {
				ARKN141_LCDC_ERR("ARKFB_GET_WINDOW_ADDR arkn141_lcdc_get_osd_addr error\n");
		        goto end;
			}
			if(copy_to_user((void  *)arg, &addr, sizeof(struct ark_disp_addr))){
				ARKN141_LCDC_ERR("ARKFB_GET_WINDOW_ADDR copy to user para error\n");
		        error = -EFAULT;
		        goto end;
			}
			//ARKN141_LCDC_DEBUG("ARKFB_GET_WINDOW_ADDR osd layer=%d: yaddr=0x%0x, cbaddr=0x%0x, craddr=0x%0x\n", layer, addr.yaddr, addr.cbaddr, addr.craddr);
			break;
		}
		case ARKFB_GET_SCREEN_INFO:
		{
			struct ark_screen screen;

			memset(&screen, 0, sizeof(struct ark_screen));
			
			if(lcdfb_info && lcdfb_info->info) {
				arkn141_lcdc_width = screen.width  = screen.disp_width = lcdfb_info->info->var.xres;
				arkn141_lcdc_height = screen.height = screen.disp_height = lcdfb_info->info->var.yres;
			} else {
				screen.width  = screen.disp_width  = arkn141_lcdc_width;
				screen.height = screen.disp_height = arkn141_lcdc_height;
			}
			if(copy_to_user((void  *)arg, &screen, sizeof(struct ark_screen))){
				ARKN141_LCDC_ERR("ARKFB_GET_SCREEN_INFO copy to user para error\n");
				error = -EFAULT;
				goto end;
			}
			ARKN141_LCDC_DEBUG("ARKFB_GET_SCREEN_INFO type=%d, width=%d, height=%d\n", screen.type, screen.width,screen.height);
			break;
		}
#if 0
		case ARKFB_SET_SCREEN_INFO:	//not use
		{
			struct ark_screen screen;

			if(copy_from_user(&screen, (void *)arg, sizeof(struct ark_screen))) {
				ARKN141_LCDC_ERR("ARKFB_SET_SCREEN_INFO copy from user para error\n");
				error = -EFAULT;
				goto end;
			}
			if((screen.width > 0xFFF) || (screen.height > 0xFFF)) {
				ARKN141_LCDC_ERR("ARKFB_SET_SCREEN_INFO invalid width:%d, height:%d\n", screen.width, screen.height);
				error = -EFAULT;
				goto end;
			}
			arkn141_lcdc_width = screen.width;
			arkn141_lcdc_height = screen.height;
			ARKN141_LCDC_DEBUG("ARKFB_SET_SCREEN_INFO type=%d, width=%d, height=%d\n", screen.type, screen.width,screen.height);
			break;
		}
#endif
        case ARKFB_GET_PLATFORM_INFO:
        {
			struct ark_platform_info platform;

			memset(&platform, 0, sizeof(struct ark_platform_info));
			platform.type = ARK_PLATFORM_ARKN141;
			if(copy_to_user((void  *)arg, &platform, sizeof(struct ark_platform_info))){
				ARKN141_LCDC_ERR("ARKFB_GET_PLATFORM_INFO copy to user para error\n");
				error = -EFAULT;
				goto end;
			}
        	break;
        }
		default:
			ARKN141_LCDC_ERR("unknown ioctl %08x\n", cmd);
			error = -EFAULT;
			break;
	}

end:
	mutex_unlock(&sinfo->mutex_lock);

	return error;
}
EXPORT_SYMBOL(arkn141_lcdfb_ioctl);

int arkn141_lcdc_funcs_init(struct arkn141_lcdfb_info *sinfo)
{
	struct fb_info *info = sinfo->info;
	struct fb_var_screeninfo *var = &info->var;

	lcdfb_info = sinfo;
	arkn141_lcdc_width = var->xres;
	arkn141_lcdc_height = var->yres;
	lcdc_base = sinfo->mmio;

	return 0;
}
EXPORT_SYMBOL(arkn141_lcdc_funcs_init);

