package ru.dz.phantom.snapdump;

import java.nio.MappedByteBuffer;


/**
 * Goes through all the address space jumping on object headers.
 * Checks if we have all allocated objects 
 * @author dz
 *
 */
public class Scanner {

	private final MappedByteBuffer snap;
	
	private int totalCount = 0;
	private int freeCount = 0;
	private int notFreeCount = 0;
	private int freeAndVisitedCount = 0;
	private int notFreeAndNotVisitedCount = 0;

	public Scanner(MappedByteBuffer bb) {
		this.snap = bb;
	}

	public void scan(VisitedMap visitedMap) throws InvalidPhantomMemoryException {
		int remaining = snap.remaining();
		
		System.out.println("Scanner.scan() will scan "+remaining/1024+" Kbytes");
		
		for( int addr = 0; addr < remaining; )
		{
			System.out.println("Scanner.scan() at "+addr);
			
			snap.position(addr);
			
			AllocHeader allocHeader = new AllocHeader(snap);
			VisitedState visitedState = visitedMap.get(addr);
			
			boolean free = allocHeader.isFree();

			totalCount++;
			
			if(free) freeCount++;
			else notFreeCount++;
			
			if(free && (visitedState != null))
			{
				freeAndVisitedCount++;
				System.out.println("Scanner.scan() free and visited: "+addr);
			}

			if( !free && (visitedState == null))
			{
				notFreeAndNotVisitedCount++;
				System.out.println("Scanner.scan() allocated and not visited: "+addr);
			}
			
			if(allocHeader.getSize() <= 0)
				throw new InvalidPhantomMemoryException("allocHeader.getSize() <= 0");
			
			addr += allocHeader.getSize();			
		}
		
	}

	public void printStats()
	{
		System.out.println( String.format("Total %d objects: %d free, %d not free", totalCount, freeCount, notFreeCount ));
		System.out.println( String.format("Errors: %d free and visited, %d not free and not visited", freeAndVisitedCount, notFreeAndNotVisitedCount ));
	}
	
	public int getTotalCount() {		return totalCount;	}
	public int getFreeCount() {		return freeCount;	}
	public int getNotFreeCount() {		return notFreeCount;	}
	public int getFreeAndVisitedCount() {		return freeAndVisitedCount;	}
	public int getNotFreeAndNotVisitedCount() {		return notFreeAndNotVisitedCount;	}

}
