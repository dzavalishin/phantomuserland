package ru.dz.phantom.code;

import java.io.FileWriter;
import java.io.IOException;
import java.io.RandomAccessFile;

import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.PhantomField;
import ru.dz.plc.util.PlcException;

/**
 * Used to save field info (name, ordinal, type) to class file.
 * @author dz
 *
 */
public class FieldFileInfo extends FileInfo {

	private final FileWriter lst;
	private PhantomField f;

	public FieldFileInfo(RandomAccessFile os, FileWriter lst, PhantomField f) {
		super( os, (byte)'f' );
		this.lst = lst;
		this.f = f;
	}

	@Override
	protected void do_write_specific() throws IOException, PlcException {
		Fileops.put_string_bin( os, f.getName());
		Fileops.put_int32( os, f.getOrdinal() );
		f.getType().save_to_file(os);
	}

}
