package ru.dz.phantom.code;

import java.io.*;

import ru.dz.plc.compiler.*;
import ru.dz.plc.util.*;

import java.util.*;

/**
 * <p>Class file loader.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class ClassInfoLoader {
  boolean debug_print = false;

  RandomAccessFile is;
  String class_name, class_parent_name;
  PhantomClass my_class;

  public ClassInfoLoader(RandomAccessFile is)
  {
    this.is = is;
  }

  public String get_class_name() { return class_name; }
  public PhantomClass get_class() { return my_class; }

  public boolean load_class_file() throws IOException, PlcException {

    long fsize = is.length();
    //printf("fsize %d\n", fsize );

    //byte data[] = new byte[fsize];
    //is.read(data,0,fsize);

    // parse it
    long record_size = 0;
    boolean got_class_header = false;

    for( long rec_start = 0; rec_start < fsize; rec_start = rec_start+record_size )
    {
      if(debug_print) System.out.print("\n");

      is.seek(rec_start);
      long ptr = rec_start;
      //printf("%d bytes left\n", contents + fsize - ptr );

      {
        byte mrk[] = new byte[5];
        //is.read(mrk,(int)rec_start,5);
        is.read(mrk);

        if(
        mrk[0] != 'p' ||
        mrk[1] != 'h' ||
        mrk[2] != 'f' ||
        mrk[3] != 'r' ||
        mrk[4] != ':' )
          throw new PlcException("import","No record marker in class file");
      }

      ptr += 5;

      char record_type;

      {
        ptr++;
        byte rt = is.readByte();
        record_type = (char)rt;
      }

      record_size = is.readInt();

      if(debug_print)
      {
        System.out.print("type '" + record_type + "', size ");
        System.out.print(record_size);
        System.out.print(": ");
      }

      if( record_size < 6+8 )
        throw new PlcException("import","Invalid record size");

      // start of record-specific info
      ptr = is.getFilePointer();

      switch( record_type )
      {
          case 'C': // class
      {
        if( my_class != null )
          throw new PlcException("import","more than one class in class file", is.toString() );

        loader_handler h = new loader_handler( is, (int)(record_size - (ptr-rec_start)));
        class_name = Fileops.get_string(is);
        my_class = new PhantomClass(class_name);
        if(debug_print) System.out.println( "Class is: "+class_name+"\n" );
        int n_object_slots = Fileops.get_int32(is);
        //if(debug_print) System.out.println(", %d fileds", n_object_slots );
        int n_method_slots = Fileops.get_int32(is);
        //if(debug_print) System.out.println(", %d methods\n", n_method_slots );
        class_parent_name = Fileops.get_string(is);

        if(class_parent_name != null && class_parent_name.length() > 0 && !class_parent_name.equals(".internal.object"))
          my_class.addParent(class_parent_name,null);

        if(class_name != null && class_name.equals(".internal.object"))
        {
          // Hack! Hack!
          ru.dz.plc.compiler.ClassMap.get_map().imported_add_hack(my_class);
          // or else ww will die on adding Method signatures,
          // as they need types too
        }

        got_class_header = true;
      }
      break;

      case 'M': // Method
      {
        method_loader_handler mh = new method_loader_handler( is, (int)(record_size - (ptr-rec_start)));
        //methods.push_back(mh);
      }
      break;

      case 'S': // Method signature
      {
        method_signature_loader_handler mh = new method_signature_loader_handler( is, (int)(record_size - (ptr-rec_start)));
        //methods.push_back(mh);

        my_class.addMethod(mh.me);
      }
      break;

      }

    }

    if( !got_class_header )
      return false;

    return true;
  }




}


/*

class code_handler
{
  int get_int32( RandomAccessFile is ) throws IOException {
    byte bb[] = new byte[4];

    is.read(bb);

    int v;

    v = bb[3];
    v |= ((int)bb[2]) << 8;
    v |= ((int)bb[1]) << 16;
    v |= ((int)bb[0]) << 24;
    return (int)v;
  }

  String get_string( RandomAccessFile is ) throws IOException {
    int len = get_int32(is);

    byte data[] = new byte[len];
    is.read(data);

    return new String(data);
  }

};
*/



 class loader_handler
 {
   RandomAccessFile is;
   int size;

   public loader_handler(  RandomAccessFile is, int size )
   {
     this.is = is;
     this.size = size;
   };
 };



 class method_loader_handler extends loader_handler
 {
   String    my_name;
   int       my_ordinal;
   //phantom_object my_code;

   public method_loader_handler( RandomAccessFile is, int in_size ) throws
      IOException {
     super( is, in_size );

     my_name = Fileops.get_string(is);
     //if(debug_print) System.out.println("Method is: ", my_name );

     my_ordinal = Fileops.get_int32(is);
     //if(debug_print) System.out.println(", ordinal: ", my_ordinal, "\n" );
   }

   public int get_ordinal() { return my_ordinal; }
   public String get_name() { return my_name; }
 };


 class method_signature_loader_handler extends loader_handler
  {
    boolean debug_print = false;

    String    my_name;
    int       my_ordinal;
    int       my_args_count = 0;

    Method    me = null;

    LinkedList<ArgDefinition> args = new LinkedList<ArgDefinition>();


    public method_signature_loader_handler( RandomAccessFile is, int in_size ) throws
      IOException, PlcException {
      super( is, in_size );

      my_name = Fileops.get_string(is);

      my_ordinal = Fileops.get_int32(is);
      //if(debug_print) System.out.println(", ordinal: ", my_ordinal, "\n" );

      my_args_count = Fileops.get_int32(is);
      if(debug_print) System.out.println(my_args_count);

      PhantomType my_return_type = new PhantomType(is);

      me = new Method( my_name, my_return_type );
      me.ordinal = my_ordinal;

      if(debug_print) System.out.print("Method is: " + my_name + ", ret type is '"+my_return_type.toString()+"', arg count = " );

      for( int arg_no = 0; arg_no < my_args_count; arg_no++ )
      {
        String arg_name = Fileops.get_string(is);
        PhantomType arg_type = new PhantomType(is);

        if(debug_print) System.out.print("Arg '"+arg_name+"' ");
        if(debug_print) System.out.print(": '"+arg_type.get_main_class_name()+"' ");
        if(debug_print && arg_type.is_container())
        {
          System.out.print("( "+arg_type.get_contained_class_name()+"[])");
        }
        if(debug_print) System.out.println("");

        ArgDefinition ad = new ArgDefinition( arg_name, arg_type );

        args.add(ad);
      }



      if(debug_print) System.out.println("");
    }

    public int get_ordinal() { return my_ordinal; }
    public int get_args_count() { return my_args_count; }
    public String get_name() { return my_name; }


  };


