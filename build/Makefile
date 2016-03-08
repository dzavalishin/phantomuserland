include ../config.mk 

install: tools boot

boot: classes menu.lst #phantom
	cp $^ $(TFTP_PATH)


ALL_TOOLS=mkpfs mkbulk 

ifeq ($(OSTYPE),cygwin)
ALL_TOOLS += mkpfs.cmd mkbulk.cmd
endif


tools: $(INST_BIN)/pfsextract.exe $(INST_BIN)/pfsformat.exe \
       $(INST_BIN)/plc.cmd $(INST_BIN)/pvm_test.exe \
       $(INST_BIN)/pbitmap.cmd \
		$(INST_BIN)/mkbulk.cmd $(INST_BIN)/mkbulk $(INST_BIN)/mkpfs $(INST_BIN)/mkpfs.cmd
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

$(INST_BIN)/plc.cmd: bin/plc.cmd
	cp $^ $@

$(INST_BIN)/jpc.cmd: bin/jpc.cmd
	cp $^ $@

$(INST_BIN)/mkbulk.cmd: bin/mkbulk.cmd
	cp $^ $@

$(INST_BIN)/mkbulk: bin/mkbulk
	cp $^ $@

$(INST_BIN)/mkpfs: bin/mkpfs
	cp $^ $@

$(INST_BIN)/mkpfs.cmd: bin/mkpfs.cmd
	cp $^ $@

#$(INST_BIN)/mkbulk.exe: bin/mkbulk.exe
#	cp $^ $@

