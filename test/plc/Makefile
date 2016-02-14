include ../../config.mk

.SILENT:

BINDIR := bin
SRCDIRS := src
FILES := $(foreach DIR,$(SRCDIRS),$(wildcard $(DIR)/*))
PHFILES := $(filter %.ph,$(FILES))
PCFILES := $(patsubst %.ph,%.pc,$(notdir $(PHFILES)))

# clear vpaths
vpath %.ph
vpath %.pc


vpath %.ph $(SRCDIRS)
vpath %.pc $(BINDIR)
	

%.pc: %.ph
	@echo :-- $@
	@$(PLC) -o$(BINDIR) -I$(PHANTOM_HOME)/plib/bin $<
	sed -e 1d < bin/$(patsubst %.pc,%.lstc,$@)  > $(patsubst %.pc,%.lstc,$@).bin
	sed -e 1d < good/$(patsubst %.pc,%.lstc,$@) > $(patsubst %.pc,%.lstc,$@).good
	diff -w $(patsubst %.pc,%.lstc,$@).bin $(patsubst %.pc,%.lstc,$@).good
	diff -w bin/$(patsubst %.pc,%.lst,$@) good/$(patsubst %.pc,%.lst,$@)
	-rm $(patsubst %.pc,%.lstc,$@).bin $(patsubst %.pc,%.lstc,$@).good

#	cmp $(patsubst %.pc,%.lstc,$@).bin $(patsubst %.pc,%.lstc,$@).good
#	cmp bin/$(patsubst %.pc,%.lst,$@) good/$(patsubst %.pc,%.lst,$@)

test::
# clean $(PCFILES) done
	@$(MAKE) clean
	-@echo Make sure plib is made
	@$(MAKE) $(PCFILES)
	@$(MAKE) done
	@$(MAKE) clean
	

clean::
	-rm bin/*


done::
	@echo	
	@echo All tests passed
	@echo
