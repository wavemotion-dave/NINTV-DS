#export DEVKITPRO=/opt/devkitpro
#export DEVKITARM=/opt/devkitpro/devkitARM
#
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
.SECONDARY:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules

export TARGET		:=	NINTV-DS
export TOPDIR		:=	$(CURDIR)
export NITRODATA	:=	$(CURDIR)/nitrofiles
export VERSION		:=  1.0c

ICON 		:= -b $(CURDIR)/logo.bmp "NINTV-DS $(VERSION);wavemotion-dave;https://github.com/wavemotion-dave/NINTV-DS"

.PHONY: arm7/$(TARGET).elf arm9/$(TARGET).elf overlays

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: $(TARGET).nds

#---------------------------------------------------------------------------------
$(TARGET).nds	:	arm7/$(TARGET).elf arm9/$(TARGET).elf overlays
	ndstool	-c $(TARGET).nds -7 arm7/$(TARGET).elf -9 arm9/$(TARGET).elf -d $(NITRODATA) $(ICON)
  
#---------------------------------------------------------------------------------
arm7/$(TARGET).elf:
	$(MAKE) -C arm7
	
#---------------------------------------------------------------------------------
arm9/$(TARGET).elf:
	$(MAKE) -C arm9

#---------------------------------------------------------------------------------
overlays:
	$(MAKE) -C overlays

#---------------------------------------------------------------------------------
clean:
	$(MAKE) -C arm9 clean
	$(MAKE) -C arm7 clean
	$(MAKE) -C overlays clean
	rm -f $(TARGET).nds $(TARGET).arm7 $(TARGET).arm9
