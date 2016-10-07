

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 *
 * @author vassaeve
 */
public class CreateDisk {

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        if (args.length < 3) {
            System.out.println("Usage: folder  number_of_disk  number_of_block");
            System.exit(-1);
        }
        String baseDir = args[0];

        File dir = new File(baseDir);
        if (dir.exists() && !dir.isDirectory()) {
            System.out.println(baseDir + " isn't a folder.");
            System.exit(-4);
        } else if (!dir.exists() && dir.mkdirs()) {
            System.out.println("Can't create folder " + baseDir);
            System.exit(-4);
        }

        int count = 0;
        try {
            count = Integer.parseInt(args[1]);
        } catch (NumberFormatException ex) {
            System.out.println("Second parameter is not recognized. Please set number of disks.");
            System.exit(-2);
        }
        int size = 0;
        try {
            size = Integer.parseInt(args[2]); 
        } catch (NumberFormatException ex) {
            System.out.println("Third parameter is not recognized. Please set size of disk.");
            System.exit(-3);
        }
        byte[] buf = new byte[4096];
        int exitCode = 0;
        String fileName;
        for (int i = 0; i < count; i++) {
            fileName = baseDir + File.separator + "disk" + (i == 0 ? "" : i) + ".img";
            File file = new File(fileName);
            System.out.print(fileName);
            if (file.exists()) {
                file.delete();
            }
            try {
                file.createNewFile();
            } catch (IOException ex) {
                System.out.println(" --> can't create file." + ex.getMessage());
                exitCode++;
                continue;
            }
            try {
                FileOutputStream out = new FileOutputStream(file);
                for (int blk = 0; blk < size; blk++) {
                    out.write(buf);
                }
                out.close();
                System.out.println(" --> OK");
            } catch (IOException exe) {
                System.out.println(" --> FAIL. " + exe.getMessage());
                exitCode++;
            }
        }
        System.exit(exitCode);
    }

}
