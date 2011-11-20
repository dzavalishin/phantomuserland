include ../../../config.mk

all: ru.dz.phantom.tetris.mainmodule.pc ru.dz.phantom.tetris.boot.pc

ru.dz.phantom.tetris.figure.ph: ru.dz.phantom.tetris.playingfield.pc

ru.dz.phantom.tetris.io.ph: ru.dz.phantom.tetris.utilites.pc

ru.dz.phantom.tetris.mainmodule.ph: ru.dz.phantom.tetris.figure.pc ru.dz.phantom.tetris.imagesmanager.pc \
	ru.dz.phantom.tetris.io.pc ru.dz.phantom.tetris.playingfield.pc	ru.dz.phantom.tetris.utilites.pc

clean:
	rm -f *.pc
