package phantom.gg;



import phantom.data.ObjectHeader;
import phantom.gc.Main;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.*;
import java.util.concurrent.ThreadLocalRandom;
import java.util.logging.Handler;
import java.util.stream.Collectors;
import java.util.stream.Stream;


/**
 * @author Anton Zuev(anzuev@bk.ru) on 23/06/2017.
 */
public class GarbageGenerator {

    private static final long OBJECT_VMEM_SHIFT = 0x80000000L;
    // default values, could be configured
    // using setters below
    private int freeObjectsNumber = 1000;
    private int allocatedObjectsNumber = 1000;

    //TODO: not implemented
    private int minDepth = 2;

    //TODO: not implemented
    private int maxDepth = 5;

    private int objectMinSize = 20;
    private int objectMaxSize = 40;

    // if true -> some cycles would be added
    private boolean isGarbageCycled = true;


    // what is the max size for garbage cycles
    // has to be more than 1
    private int maxCycleSize = 4;


    //TODO: not implemented
    // if false -> snapshot will have some errors
    private boolean isConsistent = true;


    public int getFreeObjectsNumber() {
        return freeObjectsNumber;
    }

    public void setFreeObjectsNumber(int freeObjectsNumber) {
        this.freeObjectsNumber = freeObjectsNumber;
    }

    public int getAllocatedObjectsNumber() {
        return allocatedObjectsNumber;
    }

    public void setAllocatedObjectsNumber(int allocatedObjectsNumber) {
        this.allocatedObjectsNumber = allocatedObjectsNumber;
    }

    public int getMinDepth() {
        return minDepth;
    }

    public void setMinDepth(int minDepth) {
        this.minDepth = minDepth;
    }

    public int getMaxDepth() {
        return maxDepth;
    }

    public void setMaxDepth(int maxDepth) {
        this.maxDepth = maxDepth;
    }

    public int getObjectMinSize() {
        return objectMinSize;
    }

    public void setObjectMinSize(int objectMinSize) {
        this.objectMinSize = objectMinSize;
    }

    public int getObjectMaxSize() {
        return objectMaxSize;
    }

    public void setObjectMaxSize(int objectMaxSize) {
        this.objectMaxSize = objectMaxSize;
    }

    public boolean isConsistent() {
        return isConsistent;
    }

    public void setConsistency(boolean isConsistent) {
        this.isConsistent = isConsistent;
    }

    public boolean isGarbageCycled() {
        return isGarbageCycled;
    }

    public void setGarbageCycled(boolean garbageCycled) {
        isGarbageCycled = garbageCycled;
    }

    public int getMaxCycleSize() {
        return maxCycleSize;
    }

    public void setMaxCycleSize(int maxCycleSize) {
        if(this.maxCycleSize < 1){
            throw new IllegalArgumentException();
        }
        this.maxCycleSize = maxCycleSize;
    }

    public static void main(String[] args){
        GarbageGenerator garbageGenerator = new GarbageGenerator();
        Map<Long, ObjectHeader> objects = garbageGenerator.generate();
        Main.printStatisticsPerSystem(objects);
        objects = Main.collectGarbage(objects);
        Main.printStatisticsPerSystem(objects);
    }


    /**
     * Generate garbage objects
     * @return
     */
    public Map<Long, ObjectHeader> generate(){
        int freeObjectsGenerated = 0,
                freeObjectsSize = 0,
                allocatedObjectsGenerated = 0,
                allocatedObjectsSize = 0;

        // generating objectHeaders for free objects
        List<ObjectHeader> freeObjects = generateObjectHeaders(AllocationObjectType.FREE);
        freeObjectsGenerated = freeObjects.size();

        //compute size of generated free objects
        for(ObjectHeader objectHeader : freeObjects){
            freeObjectsSize += objectHeader.getExactSize();
        }

        // generating objectHeaders for allocated objects
        List<ObjectHeader> allocatedObjects = generateObjectHeaders(AllocationObjectType.ALLOCATED);
        allocatedObjectsGenerated = allocatedObjects.size();

        //compute size of generated allocated objects
        for(ObjectHeader objectHeader : allocatedObjects){
            allocatedObjectsSize += objectHeader.getExactSize();
        }


        System.out.println("Generated:\n" +
                String.format("    freeObjectsGenerated: %d, freeObjectsSize: %d\n", freeObjectsGenerated, freeObjectsSize) +
                String.format("    allocatedObjectGenerated: %d, allocatedObjectsSize: %d", allocatedObjectsGenerated, allocatedObjectsSize));

        // add addresses for objects.
        Map<Long, ObjectHeader> objects = fillWithAddresses(freeObjects, allocatedObjects);

        //add references
        addReferences(objects, freeObjects, allocatedObjects);

        // no we have objects without garbage
        // all objects either referenced by someone
        // or has refCount equals to 0
        addGarbageCycles(objects);

        return objects;
    }


    /**
     *
     * @param type type of object(see ObjectType)
     * @return list of objects
     */
    private List<ObjectHeader> generateObjectHeaders(AllocationObjectType type){
        System.out.println(String.format("Generating object headers type %s", type));
        int needToGenerate = getNumberOfObjectsToGenerate(type);

        List<ObjectHeader> objects = new ArrayList<>();

        // generate free objects
        if(type == AllocationObjectType.FREE){
            while(needToGenerate > 0){
                objects.add(PhantomObjectsBuilder.getFreeObject(getNextObjectSize()));
                needToGenerate--;
            }
            return objects;
        }

        // generate allocated objects
        while(needToGenerate > 0){
            if(((int) (Math.random()*1000)) % 2 == 0){
                objects.add(PhantomObjectsBuilder.buildExternalObject(getNextObjectSize()));
            }else{
                ObjectHeader obj = PhantomObjectsBuilder.buildInternalObject(InternalObjectType.LONG);
                objects.add(obj);
            }
            needToGenerate--;
        }
        return objects;
    }



    private int getNumberOfObjectsToGenerate(AllocationObjectType type){
        switch (type){
            case FREE:
                return freeObjectsNumber;
            case ALLOCATED:
                return allocatedObjectsNumber;
            default:
                return -1;
        }
    }


    /**
     * computes size of next object to create
     * in a range from objectMinSize to objectMaxSize
     * @return size;
     */
    private int getNextObjectSize(){
        // int now can be represented as 4*n where n is natural number
        return (int)(ThreadLocalRandom.current().nextInt(objectMinSize, objectMaxSize + 1)/4.0)*4;
    }


    /**
     * add addresses to objects
     * @param freeObjects list of free objects
     * @param allocatedObjects list of allocated objects
     * @return objects with addresses
     */
    private Map<Long, ObjectHeader> fillWithAddresses(List<ObjectHeader> freeObjects, List<ObjectHeader> allocatedObjects){

        System.out.println("Filling addresses");
        //set root object
        ObjectHeader root = allocatedObjects.remove(0);
        while(root.isInternal()){
            allocatedObjects.add(root);
            Collections.shuffle(allocatedObjects);
            root = allocatedObjects.remove(0);
        }


        List<ObjectHeader> allObjects =  Stream.concat(freeObjects.stream(), allocatedObjects.stream()).collect(Collectors.toList());
        Collections.shuffle(allObjects);

        Map<Long, ObjectHeader> map = new HashMap<>();
        map.put(OBJECT_VMEM_SHIFT, root);

        long curAddress = OBJECT_VMEM_SHIFT + root.getExactSize();
        for (ObjectHeader objectHeader : allObjects){
            map.put(curAddress, objectHeader);
            curAddress += objectHeader.getExactSize();
        }
        return map;
    }


    /**
     * add references between objects
     * @param objects map of objects(with addresses)
     * @param freeObjects list of free objects
     * @param allocatedObjects list of allocated objects
     */
    private void addReferences(Map<Long, ObjectHeader> objects,List<ObjectHeader> freeObjects, List<ObjectHeader> allocatedObjects) {

        System.out.println("Adding references");

        Map<ObjectHeader, Long> objectHeaderToAddressMap = new HashMap<>();

        List<ObjectHeader> allObjects = Stream.concat(freeObjects.stream(), allocatedObjects.stream()).collect(Collectors.toList());
        Collections.shuffle(allObjects);
        Map<Long, Integer> pointersAdded = new HashMap<>();

        for (Map.Entry<Long, ObjectHeader> entry : objects.entrySet()) {
            objectHeaderToAddressMap.put(entry.getValue(), entry.getKey());
            pointersAdded.put(entry.getKey(), 0);
        }

        Set<Long> visited = new HashSet<>();
        Queue<Long> queue = new LinkedList<>();
        queue.add(OBJECT_VMEM_SHIFT);


        while(queue.size() > 0){
            long address = queue.remove();
            ObjectHeader objectHeader = objects.get(address);
            int pointers = pointersAdded.get(address);

            // no need to add references to internal object
            if(objectHeader.isInternal()){
                continue;
            }

            // each pointer is 4 bytes
            int pointersCapacity = objectHeader.getDaSize()/4;

            ByteBuffer da = objectHeader.getDataArea();
            da.order(ByteOrder.LITTLE_ENDIAN);

            while(pointersCapacity > pointers) {
                // get random object from list
                ObjectHeader objToAdd = allObjects.get(ThreadLocalRandom.current().nextInt(0, allObjects.size()));
                long objToAddAddress = objectHeaderToAddressMap.get(objToAdd);

                if (!visited.contains(objToAddAddress)) {
                    visited.add(objToAddAddress);
                    queue.add(objToAddAddress);
                }
                da.putInt((int)((long) objToAddAddress));
                objToAdd.setRefCount(objToAdd.getRefCount() + 1);
                pointers++;
            }
        }

    }


    /**
     * For some objects that are not being referenced but allocated
     * change refcount value to non zero one
     * @param objects all objects
     */
    private void addSimpleGarbage(Map<Long, ObjectHeader> objects){

    }

    /**
     * Add cycles of objects with refcount 0
     * @param objects objects
     */
    private void addGarbageCycles(Map<Long, ObjectHeader> objects){
        List <Long> nonReferencedObjects = new ArrayList<>();

        // get all external objects with refCount equals to 0
        // (don't add root objects)
        for(Map.Entry<Long, ObjectHeader> entry: objects.entrySet()){
            if(entry.getValue().getRefCount() == 0 && !entry.getValue().isInternal()
                    && entry.getKey() != OBJECT_VMEM_SHIFT)
                nonReferencedObjects.add(entry.getKey());
        }


        ObjectHeader prevObject = null;
        ObjectHeader nextObject;
        ObjectHeader baseObject = null;
        long baseObjectAddress = 0;
        long counter = 0;
        int cycleSize = 0;

        for(long address: nonReferencedObjects){
            if(cycleSize == 0){
                cycleSize = ThreadLocalRandom.current().nextInt(2,  this.maxCycleSize);
            }

            if(baseObject == null){
                baseObject = objects.get(address);
                baseObjectAddress = address;
                prevObject = baseObject;
                counter = 1;
                continue;
            }else{
                nextObject = objects.get(address);
                ByteBuffer da = prevObject.getDataArea();
                da.order(ByteOrder.LITTLE_ENDIAN);
                da.putInt((int) address);
                nextObject.setRefCount(nextObject.getRefCount() + 1);
                counter++;
                prevObject = nextObject;
            }

            if(counter == cycleSize){
                System.out.println(String.format("Added cycle of size %d", cycleSize));
                cycleSize = 0;
                ByteBuffer da = nextObject.getDataArea();
                da.order(ByteOrder.LITTLE_ENDIAN);
                da.putInt((int)((long) baseObjectAddress));
                baseObject.setRefCount(baseObject.getRefCount() + 1);
                baseObject = null;
            }
        }
        System.out.println(String.format("Size of nonreferenced objects: %d", nonReferencedObjects.size()));

        int a = 4;

    }




}

