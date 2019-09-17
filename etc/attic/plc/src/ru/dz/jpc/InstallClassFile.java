/* $Id: InstallClassFile.java,v 1.2 1998/07/24 23:19:36 pab Exp $
 * Wrapper around class-file installer.  This reads each class (one per
 * line of stdin) using the current CLASSPATH, then writes a corresponding
 * classfile into the proper hierarchy location in the destination
 * directory.  We use this to put things into package class directories
 * before we build the whole package.
 */
 
package ru.dz.jpc;
import java.io.*;
import ru.dz.jpc.tanslator.ClassInstall;

public class
InstallClassFile
{
    public static void
    main (String args[])
    {
        BufferedReader stdin;   // Where we get class lists
        String destDir;         // Where we put class files

        if (0 >= args.length) {
            System.err.println ("usage: InstallClassFile destDir");
            System.exit (1);
        }
        destDir = args [0];
        stdin = new BufferedReader (new InputStreamReader (System.in));
        while (true) {
            String str = null;

            try {
                str = stdin.readLine ();
            } catch (IOException e) {
            }
            if (null == str) {
                break;
            }
            int ist = 0;
            while (ist < str.length ()) {
                String cname;

                int nist = str.indexOf (' ', ist);
                if (0 < nist) {
                    cname = str.substring (ist, nist);
                    ist = nist;
                    while ((ist < str.length ()) &&
                           ' ' == str.charAt (ist)) {
                        ++ist;
                    }
                } else {
                    cname = str.substring (ist);
                    ist = str.length ();
                }
                try {
                    ClassInstall.install (cname, cname, destDir);
                    System.out.println (cname + " -> " + destDir);
                } catch (Exception e) {
                    System.err.println ("Failed to copy " + cname + ": " + e);
                }
            }
        }
    }
}
