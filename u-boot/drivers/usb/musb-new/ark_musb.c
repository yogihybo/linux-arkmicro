#include "musb_core.h"

static int ark_musb_init(struct musb *musb)
{
	return 0;
}

static int ark_musb_enable(struct musb *musb)
{
	return 0;
}

static void ark_musb_disable(struct musb *musb)
{
}

static int ark_musb_exit(struct musb *musb)
{
	return 0;
}

const struct musb_platform_ops ark_musb_ops = {
	.init		= ark_musb_init,
	.exit		= ark_musb_exit,
	.enable		= ark_musb_enable,
	.disable	= ark_musb_disable,
};
