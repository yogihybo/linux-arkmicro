/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_ARK_TOOL_H
#define _LINUX_ARK_TOOL_H

/*
 * Hook for drivers/tty/serial/ark_uart.c: stock's kernel triggers
 * arktool_reg_init() the first time the "ark1680-uart" platform
 * device's sub-port index 0 (uart0, 0xe4200000 -- the debug console,
 * ttyS0 on this tree) completes a successful .startup(). See
 * drivers/misc/ark_tool.c for the full port and rationale.
 */
#ifdef CONFIG_ARK_TOOL
void ark_tool_notify_uart_open(int line);
#else
static inline void ark_tool_notify_uart_open(int line) {}
#endif

#endif /* _LINUX_ARK_TOOL_H */
