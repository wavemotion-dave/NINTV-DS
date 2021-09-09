GRAPHICS	:=	.
NITROFILES	:=	../nitrofiles

PNGFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.png)))

OUTPUTS		:=	$(addprefix $(NITROFILES)/,$(PNGFILES:.png=.grf))

.PHONY: $(BUILD) all clean

all	:	$(OUTPUTS)

clean:
	@echo clean ...
	@rm -rf $(OUTPUTS)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data 
#---------------------------------------------------------------------------------
$(NITROFILES)/%.grf	:	%.png
	@[ -d $(NITROFILES) ] || mkdir -p $(NITROFILES)
	@grit $^ -o $@ -gt -mrt -mR8 -mLs -gzl -mzl -fh!
