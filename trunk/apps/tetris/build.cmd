rem Run it from ..\ directory

cd tetris
call plc ru.dz.phantom.tetris.imagesmanager.ph
call plc ru.dz.phantom.tetris.utilites.ph
call plc ru.dz.phantom.tetris.io.ph
call plc ru.dz.phantom.tetris.playingfield.ph
call plc ru.dz.phantom.tetris.figure.ph
call plc ru.dz.phantom.tetris.mainmodule.ph
cd ..

rem call plc compiler_regression_test.ph ru.dz.phantom.system.boot.ph
