# SPDX-License-Identifier: GPL-2.0
# ARKMICRO Platform Support
#ark1668
snd-soc-ark1668-i2s-objs := ark1668_i2s.o
obj-$(CONFIG_SND_SOC_ARK1668_I2S) += snd-soc-ark1668-i2s.o

#arkn141
snd-soc-ark-i2s-objs := ark_i2s.o
obj-$(CONFIG_SND_SOC_ARK_I2S) += snd-soc-ark-i2s.o

#ark1668e
snd-soc-ark1668e-i2s-objs := ark1668e_i2s.o
obj-$(CONFIG_SND_SOC_ARK1668E_I2S) += snd-soc-ark1668e-i2s.o

#ark1668
obj-$(CONFIG_SND_SOC_ARK1668_ADC)	+= ark1668-sdadc-codec.o
obj-$(CONFIG_SND_SOC_ARK1668_DAC)	+= ark1668-sddac-codec.o

#arkn141
snd-soc-ark-internal-dac-objs := ark_dac_codec.o
snd-soc-ark-internal-adc-objs := ark_adc_codec.o
obj-$(CONFIG_SND_SOC_ARK_INTERNAL_DAC) += snd-soc-ark-internal-dac.o
obj-$(CONFIG_SND_SOC_ARK_INTERNAL_ADC) += snd-soc-ark-internal-adc.o

#ark1668e
obj-$(CONFIG_SND_SOC_ARK1668E_INTERNAL_ADAC) += ark1668e_audio_codec.o

#arkn141
snd-soc-ark-arkn141-objs := arkn141_audio.o
obj-$(CONFIG_SND_SOC_ARK_ARKN141) += snd-soc-ark-arkn141.o

snd-soc-ark-bd37033-objs                  := BD37033.o
obj-$(CONFIG_SND_SOC_BD37033) += snd-soc-ark-bd37033.o