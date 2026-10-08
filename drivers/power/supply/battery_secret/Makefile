# SPDX-License-Identifier: GPL-2.0-only

# Downstream raises the frame limit for Maxim's SHA3-HMAC helper, whose
# HMAC staging buffers need ~2.9k of stack.
ccflags-y += -Wframe-larger-than=2945

obj-$(CONFIG_ONEWIRE_GPIO) += onewire_gpio.o
obj-$(CONFIG_BATT_VERIFY_BY_DS28E16) += ds28e16.o ucl_sha3.o sha384_software.o
