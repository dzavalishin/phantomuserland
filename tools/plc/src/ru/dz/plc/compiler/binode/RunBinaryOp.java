package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.plc.util.PlcException;

@FunctionalInterface
public interface RunBinaryOp {

    void run() throws PlcException, IOException;

}
