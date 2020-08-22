package phantom.gg;

import phantom.data.AllocHeader;
import phantom.data.ObjectFlags;
import phantom.data.ObjectHeader;

import java.nio.ByteBuffer;

/**
 * @author Anton Zuev(anzuev@bk.ru) on 23/06/2017.
 */
public class PhantomObjectsBuilder {

    private static final int OBJECT_DA_SIZE = 100; // default size for da[] field, in bytes


    /**
     * creates ObjectHeader for free object
     * @return object header
     */
    private static ObjectHeader createFreeObject(){
        // fill fields in AllocHeader
        ObjectHeader objectHeader = new ObjectHeader();
        objectHeader.setAllocFlags((byte)0);
        objectHeader.setGcFlags((byte)0);
        objectHeader.setRefCount(0);

        return objectHeader;
    }

    /**
     * creates ObjectHeader for free object with specific size
     * @param size size of da field size
     * @return objectHeader
     */
    public static ObjectHeader getFreeObject(int size){
        if(size < 0)
            throw new IllegalArgumentException(String.format("Size mustn't be less than 0, found %d", size));

        ObjectHeader objectHeader = createFreeObject();
        objectHeader.setDaSize(size);
        objectHeader.setDataArea(ByteBuffer.allocate(size));
        objectHeader.setExactSize(
                AllocHeader.PHANTOM_OBJECT_HEADER_SIZE + // AH size
                        8 + // oClass
                        8 + // oSatellites
                        4 + // objectFlags
                        4 + // daSize
                        objectHeader.getDaSize()
        );
        return objectHeader;
    }

    /**
     * creates ObjectHeader for free object with default size
     * @return objectHeader
     */
    public static ObjectHeader getFreeObject(){
        return getFreeObject(OBJECT_DA_SIZE);
    }

    /**
     * creates ObjectHeader for allocated object
     * @return object header
     */
    private static ObjectHeader createAllocatedObject(){
        ObjectHeader objectHeader = createFreeObject();
        objectHeader.setAllocFlag(ObjectFlags.PVM_OBJECT_AH_ALLOCATOR_FLAG_ALLOCATED);
        return objectHeader;
    }



    /**
     * builds some simple internal objects like string, int, long,
     * class, double, float;
     * INTERFACE and CODE objects has no size yet -> not recommended to use;
     * Size of string is hardcoded and equals to 14(4 bytes per size + 10 bytes each per char);
     * @param type
     * return ObjectHeader
     */
    public static ObjectHeader buildInternalObject(InternalObjectType type){
        ObjectHeader objectHeader = createAllocatedObject();
        objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL);

        switch (type){
            case VOID:
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE);
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE);
                objectHeader.setDataArea(ByteBuffer.allocate(type.getSize()));
                objectHeader.setDaSize(type.getSize());
                break;
            case CLASS:
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS);
                objectHeader.setDataArea(ByteBuffer.allocate(type.getSize()));
                objectHeader.setDaSize(type.getSize());
                break;
            case INTERFACE:
                //since interface hasn't internal flag
                objectHeader.removeAllocFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL);
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE);
                break;
            case CODE:
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE);
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_CODE);
                break;
            case INT:
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE);
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE);
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_INT);
                objectHeader.setDaSize(type.getSize());
                objectHeader.setDataArea(ByteBuffer.allocate(type.getSize()));
                break;
            case LONG:
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE);
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE);
                objectHeader.setDaSize(type.getSize());
                objectHeader.setDataArea(ByteBuffer.allocate(type.getSize()));
                break;
            case STRING:
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE);
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE);
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_STRING);
                objectHeader.setDaSize(type.getSize());
                objectHeader.setDataArea(ByteBuffer.allocate(type.getSize()));
                break;
            case FLOAT:
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE);
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE);
                objectHeader.setDaSize(type.getSize());
                objectHeader.setDataArea(ByteBuffer.allocate(type.getSize()));
                break;
            case DOUBLE:
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_CHILDFREE);
                objectHeader.setObjectFlag(ObjectFlags.PHANTOM_OBJECT_STORAGE_FLAG_IS_IMMUTABLE);
                objectHeader.setDaSize(type.getSize());
                objectHeader.setDataArea(ByteBuffer.allocate(type.getSize()));
                break;
            default:
                throw new RuntimeException("default");
        }

        objectHeader.setExactSize(
                AllocHeader.PHANTOM_OBJECT_HEADER_SIZE + // AH size
                8 + // oClass
                8 + // oSatellites
                4 + // objectFlags
                4 + // daSize
                objectHeader.getDaSize()
        );
        return objectHeader;
    }


    /**
     * build external object with daSize specified in size param
     * @param size Mustn't be less than 0 and
     *              should be represented in a way n*4(since object reference is 4 bytes)
     * @return created object
     * @throws IllegalArgumentException if size is less than 0;
     */
    public static ObjectHeader buildExternalObject(int size){
        if(size < 0)
            throw new IllegalArgumentException(String.format("Size mustn't be less than 0, found %d", size));
        if(size % 4 != 0)
            System.out.println("Warning: size doesn't equals n*4 (n is natural)");

        ObjectHeader objectHeader = createAllocatedObject();
        objectHeader.setDaSize(size);
        objectHeader.setDataArea(ByteBuffer.allocate(size));
        objectHeader.setExactSize(
                AllocHeader.PHANTOM_OBJECT_HEADER_SIZE + // AH size
                        8 + // oClass
                        8 + // oSatellites
                        4 + // objectFlags
                        4 + // daSize
                        objectHeader.getDaSize()
        );
        return objectHeader;
    }



    /**
     * For external objects only!
     * Added reference to da[] field starting from index
     * @param objectHeader object to modify
     * @param ref reference to add
     * @param index index to start adding from
     * @throws IndexOutOfBoundsException if reference can't fit in da field
     * @throws IllegalArgumentException if objectHeader is internal
     */
    public static void setRefByIndex(ObjectHeader objectHeader, int ref, int index){
        if(objectHeader.isInternal())
            throw new IllegalArgumentException("Method couldn't be used for internal objects");

        ByteBuffer da = objectHeader.getDataArea();
        if((index + 1)*4 > da.capacity())
            throw new IndexOutOfBoundsException("Can't add reference: not enough space in byte buffer");
        da.putInt(index*4, ref);
    }





}


