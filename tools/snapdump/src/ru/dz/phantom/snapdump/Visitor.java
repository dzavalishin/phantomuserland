package ru.dz.phantom.snapdump;

import java.nio.MappedByteBuffer;
import java.util.Vector;

public class Visitor {
	private VisitedMap map = new VisitedMap();
	private ClassMap classMap = new ClassMap();

	private final MappedByteBuffer snap;

	public Visitor(MappedByteBuffer snap) {
		this.snap = snap;
	}

	public void go()
	{
		visit(0);
	}

	private ObjectHeader visit(int addr) {
		if( addr == -1 )
			return null;
		
		if( map.isVisited(addr) || addr == -1 )
			return map.get(addr).getObjectHeader();

		System.out.println("Visitor.visit("+addr+")");

		try {
			snap.position(addr);

			VisitedState visit = map.visit(addr);
			AllocHeader allocHeader;

			try {
				allocHeader = new AllocHeader(snap);
			} catch (InvalidPhantomMemoryException e) {
				visit.setInvalidMemory(true);
				System.out.println("Visitor.visit() - dead ref to "+addr);
				return null;
			}


			ObjectHeader objectHeader = new ObjectHeader(snap);

			visit.setAllocHeader(allocHeader);
			visit.setObjectHeader(objectHeader);

			Vector<ObjectRef> refs = objectHeader.getDaRefs();

			for( ObjectRef r : refs )
				visit(r);


			ObjectRef classRef = objectHeader.getClassRef();
			classMap.add( classRef );
			visit(classRef);

			return objectHeader;
		}
		catch(java.nio.BufferUnderflowException e)
		{
			System.out.println("Visitor.visit() buf underflow at "+addr);
			return null;
		}
		catch(IllegalArgumentException e)
		{
			System.out.println("Visitor.visit() illegal address at "+addr);
			return null;
		}
	}

	private void visit(ObjectRef r) {
		visit(r.getDataAddr());
		visit(r.getInterfaceAddr());		
	}

	public VisitedMap getMap() {
		return map;
	}

	public ClassMap getClassMap() {
		return classMap;
	}

}
