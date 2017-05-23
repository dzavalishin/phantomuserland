package phantom.data;


// TODO must generate all of this from C headers or both from same def file
public class ObjectFlags {

	protected static final int REF_BYTES = 8; // TODO - hardcoded size

	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_HAS_WEAKREF 		= 0x100000;

	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE       = 0x2000;

	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_FINALIZER       = 0x1000;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE       = 0x800;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_THREAD          = 0x400;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_STACK_FRAME     = 0x200;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_CALL_FRAME      = 0x100;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL        = 0x80;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE      = 0x40;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING          = 0x20;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_INT             = 0x10;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_DECOMPOSEABLE   = 0x08;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS           = 0x04;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE       = 0x02;
	public static final int  PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE            = 0x01;

	public static final int PVM_OBJECT_START_MARKER = 0x7FAA7F55;
	
	public static  String getFlagsList(int objectFlags) 
	{
		StringBuilder sb = new StringBuilder();

		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_HAS_WEAKREF))       sb.append("Weak?! ");
		
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE))       sb.append("Imm?! ");

		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_FINALIZER))       sb.append("Fin "); 		
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE))       sb.append("NoChild ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_THREAD))          sb.append("Thread ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STACK_FRAME))     sb.append("SFrame ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CALL_FRAME))      sb.append("CFrame ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL))        sb.append("Internal ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_RESIZEABLE))      sb.append("Resize ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING))          sb.append("String ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INT))             sb.append("Int ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_DECOMPOSEABLE))   sb.append("Decomp ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS))           sb.append("Class ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE))       sb.append("IFace ");
		if(0 != (objectFlags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE))            sb.append("Code ");

		return sb.toString();
	}

	public static final int  PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED		= 0x01;

	// This object has zero reference count, but objects it references are not yet
	// processed. All the children refcounts must be decremented and then this object
	// can be freed.
	public static final int  PVM_OBJECT_AH_ALLOCATOR_FLAG_REFZERO		= 0x02;



	// Flags for cycle detection candidates (noninternal objects only),
	// - can be joined with 0x00, 0x01 or 0x02
	//
	// this one set when refcounter goes down to nonzero value, and clears otherwise
	public static final int  PVM_OBJECT_AH_ALLOCATOR_FLAG_WENT_DOWN		= 0x04;
	// and this is for objects already in cycle candidates buffer -
	public static final int  PVM_OBJECT_AH_ALLOCATOR_FLAG_IN_BUFFER		= 0x08;
	
	
}
