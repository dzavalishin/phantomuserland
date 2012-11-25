package ru.dz.phantom.code;

import java.io.*;

/**
 * <p>Class file accessor.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class ClassFileInfo extends ru.dz.phantom.code.FileInfo {
	String class_name;
	String class_parent_name;
	int n_object_slots, n_method_slots;
	private final String classVersion;

	public ClassFileInfo(
			RandomAccessFile os,
			String class_name,
			String class_parent_name,
			int n_object_slots, int n_method_slots,
			String classVersion
	)
	{
		super( os, (byte)'C' );
		this.class_name = class_name;
		this.class_parent_name = class_parent_name;
		this.n_object_slots = n_object_slots;
		this.n_method_slots = n_method_slots;
		this.classVersion = classVersion;
	}

	protected void do_write_specific() throws IOException {
		Fileops.put_string_bin( os, class_name );
		Fileops.put_int32(os, n_object_slots );
		Fileops.put_int32(os, n_method_slots );
		Fileops.put_string_bin( os, class_parent_name );
		Fileops.put_string_bin( os, classVersion );
	}

}
