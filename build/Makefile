include ../config.mk 

install: tools boot

boot: classes menu.lst #phantom
	cp $^ $(PHANTOM_TFTP)

tools: $(INST_BIN)/pfsextract.exe $(INST_BIN)/pfsformat.exe \
       $(INST_BIN)/plc.cmd $(INST_BIN)/mkbulk.exe

$(INST_BIN)/pfsextract.exe: bin/pfsextract.exe
	cp $^ $@

$(INST_BIN)/pfsformat.exe: bin/pfsformat.exe 
	cp $^ $@

$(INST_BIN)/mkbulk.exe: bin/mkbulk.exe
	cp $^ $@

$(INST_BIN)/plc.cmd: bin/plc.cmd
	cp $^ $@
