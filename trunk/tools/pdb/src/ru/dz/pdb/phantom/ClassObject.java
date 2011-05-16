package ru.dz.pdb.phantom;

import java.awt.GridBagConstraints;
import java.nio.ByteBuffer;

import javax.swing.JLabel;
import javax.swing.JPanel;

import ru.dz.pdb.Main;
import ru.dz.pdb.ui.bits.RefButton;

public class ClassObject implements IInternalObject {
	private byte [] data;
	
	private int instance_object_flags;
	private int instance_object_da_size;
	private ObjectRef default_interface;
	private int sys_table_id;

	private ObjectRef class_name;
	private ObjectRef class_parent;
	private ObjectRef ip2line_maps;
	private ObjectRef method_names;

	public ClassObject(ObjectHeader oh) throws InvalidObjectOperationException {
		if( (oh.getObjectFlags() & ObjectHeader.PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS) == 0 )
			throw new InvalidObjectOperationException("Not a class object");

		data = oh.getDaData();
		loadMe();
	}
	
	private void loadMe()
	{
		ByteBuffer bb = ByteBuffer.wrap(data);

		instance_object_flags = bb.getInt();
		instance_object_da_size = bb.getInt();
		default_interface = new ObjectRef(bb);
		sys_table_id = bb.getInt();
		class_name = new ObjectRef(bb);
		class_parent = new ObjectRef(bb);

		ip2line_maps = new ObjectRef(bb);
		method_names = new ObjectRef(bb);
	}
	
	public String getClassName() { return class_name.getObject().getAsString(); }

	@Override
	public void populatePanel(JPanel panel, GridBagConstraints gbc) {
		panel.add( new JLabel("Name:      \t'"+Main.getPhantomString(class_name)+"'"), gbc );

		panel.add( new JLabel("SysId:     \t"+sys_table_id+""), gbc );
		panel.add( new JLabel("InstDa:    \t"+instance_object_da_size+""), gbc );
		panel.add( new JLabel("InstFlags: \t"+instance_object_flags+""), gbc );

		panel.add( new RefButton(class_parent,"Parent"), gbc );				

		
	}
}
