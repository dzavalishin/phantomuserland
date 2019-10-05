package phantom.gg;


/**
 * @author Anton Zuev(anzuev@bk.ru) on 23/06/2017.
 */
enum AllocationObjectType{
    FREE,
    ALLOCATED
}

enum InternalObjectType{
    VOID(0),
    CLASS(76),
    INTERFACE(-1),
    CODE(-1),
    INT(4),
    LONG(8),
    // size is dynamic, 4 bytes for size
    // and 10 more bytes for char[] itself
    // TODO: hardcoded size
    STRING(14),
    CONTAINER_ARRAY(16),
    CONTAINER_PAGE(-1),
    THREAD(-1),
    CALL_FRAME(49),
    ISTACK(-1),
    OSTACK(-1),
    ESTACK(-1),
    IO_TTY(-1),
    MUTEX(-1),
    COND(-1),
    SEMA(-1),
    BINARY(-1),
    BITMAP(-1),
    CLOJURE(-1),
    WORLD(-1),
    WINDOW(-1),
    DIRECTORY(-1),
    CONNECTION(-1),
    FLOAT(4),
    DOUBLE(8);

    //in bytes
    private final int size;

    InternalObjectType(int size){
        this.size = size;
    }

    public int getSize() {
        if(size < 0)
            throw new RuntimeException("getSize");
        return size;
    }
}

