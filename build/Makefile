include ../config.mk 

install: tools boot

boot: classes menu.lst #phantom
	cp $^ $(TFTP_PATH)

tools: $(INST_BIN)/pfsextract.exe $(INST_BIN)/pfsformat.exe \
       $(INST_BIN)/plc.cmd $(INST_BIN)/mkbulk.exe $(INST_BIN)/pvm_test.exe \
       $(INST_BIN)/pbitmap.cmd 
# $(INST_BIN)/bin2hex.cmd 

$(INST_BIN)/pbitmap.cmd: bin/pbitmap.cmd
	cp $^ $@

$(INST_BIN)/bin2hex.cmd: bin/bin2hex.cmd
	cp $^ $@

$(INST_BIN)/pfsextract.exe: bin/pfsextract.exe
	cp $^ $@

$(INST_BIN)/pfsformat.exe: bin/pfsformat.exe 
	cp $^ $@

$(INST_BIN)/pvm_test.exe: bin/pvm_test.exe
	cp $^ $@

$(INST_BIN)/mkbulk.exe: bin/mkbulk.exe
	cp $^ $@

$(INST_BIN)/plc.cmd: bin/plc.cmd
	cp $^ $@

$(INST_BIN)/plc.cmd: bin/jpc.cmd
	cp $^ $@
