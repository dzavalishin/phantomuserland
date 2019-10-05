include ../config.mk 

install: tools boot

boot: classes menu.lst #phantom
	cp $^ $(TFTP_PATH)


ALL_TOOLS=mkpfs mkbulk pfsextract

ifeq ($(OSTYPE),cygwin)
ALL_TOOLS += mkpfs.cmd mkbulk.cmd pfsextract.cmd
endif

# rm $(INST_BIN)/pfsextract.exe $(INST_BIN)/pfsformat.exe \
# $(INST_BIN)/pvm_test.exe

tools: \
	$(INST_BIN)/plc.cmd $(INST_BIN)/plc \
	$(INST_BIN)/jpc.cmd $(INST_BIN)/jpc \
	$(INST_BIN)/pcdump.cmd $(INST_BIN)/pcdump \
	$(INST_BIN)/pvm_test.cmd $(INST_BIN)/pvm_test \
	$(INST_BIN)/pbitmap.cmd  \
		$(INST_BIN)/mkbulk.cmd $(INST_BIN)/mkbulk $(INST_BIN)/mkpfs $(INST_BIN)/mkpfs.cmd $(INST_BIN)/pfsextract.cmd $(INST_BIN)/pfsextract

# $(INST_BIN)/bin2hex.cmd 

$(INST_BIN)/pbitmap.cmd: bin/pbitmap.cmd
	cp $^ $@

$(INST_BIN)/bin2hex.cmd: bin/bin2hex.cmd
	cp $^ $@


$(INST_BIN)/plc.cmd: bin/plc.cmd
	cp $^ $@

$(INST_BIN)/plc: bin/plc
	cp $^ $@

$(INST_BIN)/jpc.cmd: bin/jpc.cmd
	cp $^ $@

$(INST_BIN)/jpc: bin/jpc
	cp $^ $@

$(INST_BIN)/mkbulk.cmd: bin/mkbulk.cmd
	cp $^ $@

$(INST_BIN)/mkbulk: bin/mkbulk
	cp $^ $@

$(INST_BIN)/mkpfs: bin/mkpfs
	cp $^ $@

$(INST_BIN)/mkpfs.cmd: bin/mkpfs.cmd
	cp $^ $@

$(INST_BIN)/pfsextract.cmd: bin/pfsextract.cmd
	cp $^ $@

$(INST_BIN)/pfsextract: bin/pfsextract
	cp $^ $@

$(INST_BIN)/pvm_test.cmd: bin/pvm_test.cmd
	cp $^ $@

$(INST_BIN)/pvm_test: bin/pvm_test
	cp $^ $@

$(INST_BIN)/pcdump.cmd: bin/pcdump.cmd
	cp $^ $@

$(INST_BIN)/pcdump: bin/pcdump
	cp $^ $@


#$(INST_BIN)/pvm_test.exe: bin/pvm_test.exe
#	cp $^ $@

#$(INST_BIN)/mkbulk.exe: bin/mkbulk.exe
#	cp $^ $@

#$(INST_BIN)/pfsextract.exe: bin/pfsextract.exe
#	cp $^ $@

#$(INST_BIN)/pfsformat.exe: bin/pfsformat.exe 
#	cp $^ $@

