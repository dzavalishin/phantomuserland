package ru.dz.pdb.debugger;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

import phantom.data.ObjectRef;
import ru.dz.pdb.Main;
import ru.dz.pdb.phantom.ClassObject;
import ru.dz.pdb.phantom.InvalidObjectOperationException;
import ru.dz.pdb.phantom.ObjectHeader;

/*
 * Cache of class info objects to present class info quickly
 */
public class ClassMap {
	private Map<Long,ClassObject> map = new HashMap<Long,ClassObject>(128);
	
	public void put( long address, ObjectHeader o ) throws InvalidObjectOperationException
	{
		put( address, new ClassObject(o));
	}
	
	public void put( long address, ClassObject o )
	{
		map.put(address, o);
	}
	
	public ClassObject get( long address ) 
	{
		 ClassObject classObject = map.get(address);
		 
		 if(classObject == null)
		 {
			 ObjectHeader object = Main.getPhantomObject(address);
			 try {
				classObject = new ClassObject(object);
			} catch (InvalidObjectOperationException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
				return null;
			}
			 put( address, classObject );
		 }
		 
		 return classObject;
	}
	
	public ClassObject get( ObjectRef classRef )
	{
		return get( classRef.getDataAddr() );
	}

	public Collection<ClassObject> getList() { return map.values(); }
}
