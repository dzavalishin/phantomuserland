package ru.dz.jpc.pcdump;

import phantom.code.opcode_ids;

/**
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * Date: 13.10.2009 17:42:06
 * @author Irek
 */
public class CodeLoader extends opcode_ids {
    public String[] opcodeNames = new String[256];

    public CodeLoader() {
        opcodeNames[opcode_nop] = "nop";
        opcodeNames[opcode_debug] = "debug";
        //opcodeNames[opcode_off_skipz] = "off_skipz";
        //opcodeNames[opcode_off_skipnz] = "off_skipnz";
        opcodeNames[opcode_djnz] = "djnz";
        opcodeNames[opcode_jz] = "jz";
        opcodeNames[opcode_jmp] = "jmp";
        opcodeNames[opcode_switch] = "switch";
        opcodeNames[opcode_ret] = "ret";
        opcodeNames[opcode_short_call_0] = "short_call_0";
        opcodeNames[opcode_short_call_1] = "short_call_1";
        opcodeNames[opcode_short_call_2] = "short_call_2";
        opcodeNames[opcode_short_call_3] = "short_call_3";
        opcodeNames[opcode_call_8bit] = "call_8bit";
        opcodeNames[opcode_call_32bit] = "call_32bit";


        opcodeNames[opcode_is_dup] = "is_dup";
        opcodeNames[opcode_is_drop] = "is_drop";
        opcodeNames[opcode_os_dup] = "os_dup";
        opcodeNames[opcode_os_drop] = "os_drop";
        opcodeNames[opcode_os_load8] = "os_load8";
        opcodeNames[opcode_os_save8] = "os_save8";
        opcodeNames[opcode_os_load32] = "os_load32";
        opcodeNames[opcode_os_save32] = "os_save32";

        opcodeNames[opcode_new] = "new";
        opcodeNames[opcode_copy] = "copy";

        opcodeNames[opcode_os_compose32] = "os_compose32";
        opcodeNames[opcode_os_decompose] = "os_decompose";

        opcodeNames[opcode_os_pull32] = "os_pull32";

        opcodeNames[opcode_os_get32] = "os_get32";
        opcodeNames[opcode_os_set32] = "os_set32";

        opcodeNames[opcode_iconst_0] = "iconst_0";
        opcodeNames[opcode_iconst_1] = "iconst_1";
        opcodeNames[opcode_iconst_8bit] = "iconst_8bit";
        opcodeNames[opcode_iconst_32bit] = "iconst_32bit";
        opcodeNames[opcode_sconst_bin] = "sconst_bin";

        opcodeNames[opcode_is_get32] = "is_get32";
        opcodeNames[opcode_is_set32] = "is_set32";

        opcodeNames[opcode_push_catcher] = "push_catcher";
        opcodeNames[opcode_pop_catcher] = "pop_catcher";
        opcodeNames[opcode_throw] = "throw";


        opcodeNames[opcode_summon_thread] = "summon_thread";
        opcodeNames[opcode_summon_this] = "summon_this";

        opcodeNames[opcode_summon_null] = "summon_null";

        opcodeNames[opcode_summon_class_class] = "summon_class_class";
        opcodeNames[opcode_summon_int_class] = "summon_int_class";
        opcodeNames[opcode_summon_string_class] = "summon_string_class";
        opcodeNames[opcode_summon_interface_class] = "summon_interface_class";
        opcodeNames[opcode_summon_code_class] = "summon_code_class";
        opcodeNames[opcode_summon_array_class] = "summon_array_class";

        opcodeNames[opcode_summon_by_name] = "summon_by_name";


        opcodeNames[opcode_i2o] = "i2o";
        opcodeNames[opcode_o2i] = "o2i";
        opcodeNames[opcode_isum] = "isum";
        opcodeNames[opcode_imul] = "imul";
        opcodeNames[opcode_isubul] = "isubul";
        opcodeNames[opcode_isublu] = "isublu";
        opcodeNames[opcode_idivul] = "idivul";
        opcodeNames[opcode_idivlu] = "idivlu";

        opcodeNames[opcode_ior] = "ior";
        opcodeNames[opcode_iand] = "iand";
        opcodeNames[opcode_ixor] = "ixor";
        opcodeNames[opcode_inot] = "inot";

        opcodeNames[opcode_log_or] = "log_or";
        opcodeNames[opcode_log_and] = "log_and";
        opcodeNames[opcode_log_xor] = "log_xor";
        opcodeNames[opcode_log_not] = "log_not";

        opcodeNames[opcode_is_load8] = "is_load8";
        opcodeNames[opcode_is_save8] = "is_save8";

        opcodeNames[opcode_ige] = "ige";
        opcodeNames[opcode_ile] = "ile";
        opcodeNames[opcode_igt] = "igt";
        opcodeNames[opcode_ilt] = "ilt";

        opcodeNames[opcode_os_eq] = "os_eq";
        opcodeNames[opcode_os_neq] = "os_neq";
        opcodeNames[opcode_os_isnull] = "os_isnull";
        //opcodeNames[opcode_os_push_null] = "os_push_null";


        opcodeNames[opcode_call_00 & 0xff] = "call_00";
        opcodeNames[opcode_call_01 & 0xff] = "call_01";
        opcodeNames[opcode_call_02 & 0xff] = "call_02";
        opcodeNames[opcode_call_03 & 0xff] = "call_03";
        opcodeNames[opcode_call_04 & 0xff] = "call_04";
        opcodeNames[opcode_call_05 & 0xff] = "call_05";
        opcodeNames[opcode_call_06 & 0xff] = "call_06";
        opcodeNames[opcode_call_07 & 0xff] = "call_07";
        opcodeNames[opcode_call_08 & 0xff] = "call_08";
        opcodeNames[opcode_call_09 & 0xff] = "call_09";
        opcodeNames[opcode_call_0A & 0xff] = "call_0A";
        opcodeNames[opcode_call_0B & 0xff] = "call_0B";
        opcodeNames[opcode_call_0C & 0xff] = "call_0C";
        opcodeNames[opcode_call_0D & 0xff] = "call_0D";
        opcodeNames[opcode_call_0E & 0xff] = "call_0E";
        opcodeNames[opcode_call_0F & 0xff] = "call_0F";


        opcodeNames[opcode_call_10 & 0xff] = "call_10";
        opcodeNames[opcode_call_11 & 0xff] = "call_11";
        opcodeNames[opcode_call_12 & 0xff] = "call_12";
        opcodeNames[opcode_call_13 & 0xff] = "call_13";
        opcodeNames[opcode_call_14 & 0xff] = "call_14";
        opcodeNames[opcode_call_15 & 0xff] = "call_15";
        opcodeNames[opcode_call_16 & 0xff] = "call_16";
        opcodeNames[opcode_call_17 & 0xff] = "call_17";
        opcodeNames[opcode_call_18 & 0xff] = "call_18";
        opcodeNames[opcode_call_19 & 0xff] = "call_19";
        opcodeNames[opcode_call_1A & 0xff] = "call_1A";
        opcodeNames[opcode_call_1B & 0xff] = "call_1B";
        opcodeNames[opcode_call_1C & 0xff] = "call_1C";
        opcodeNames[opcode_call_1D & 0xff] = "call_1D";
        opcodeNames[opcode_call_1E & 0xff] = "call_1E";
        opcodeNames[opcode_call_1F & 0xff] = "call_1F";


        for (int i = 0; i<opcodeNames.length; i++) {
            if (opcodeNames[i]==null) opcodeNames[i] = "unused_opcode";
        }
    }

    public void decompile(byte[] code) {
        for (int codePointer=0; codePointer<code.length; codePointer++) {
            byte opcode = code[codePointer];
            int opcodeAddress;

            int argInt32, arg2Int32;
            int argByte;
            byte[] argBin;

            if ((opcode & 0xa0) == 0xa0) {
                opcodeAddress = codePointer;
                int method_index = 0x0F & opcode;
                int n_param = code[++codePointer] & 0xff;
                System.out.print("        " + opcodeAddress + ":\t");
                System.out.println(opcodeNames[opcode & 0xff] + " " +method_index + " " + n_param);
                continue;
            }

            switch (opcode) {
                case opcode_nop :                       // single opcode
                case opcode_ret :
                case opcode_is_dup :
                case opcode_is_drop :
                case opcode_os_dup :
                case opcode_os_drop :
                case opcode_iconst_0 :
                case opcode_iconst_1 :
                case opcode_summon_thread :
                case opcode_summon_this :
                case opcode_summon_null :
                case opcode_i2o :
                case opcode_o2i :
                case opcode_isum :
                case opcode_imul :
                case opcode_isubul :
                case opcode_isublu :
                case opcode_idivul :
                case opcode_idivlu :
                case opcode_new :
                case opcode_copy :
                case opcode_ior :
                case opcode_iand :
                case opcode_ixor :
                case opcode_inot :
                case opcode_log_or :
                case opcode_log_and :
                case opcode_log_xor :
                case opcode_log_not :
                case opcode_igt :
                case opcode_ilt :
                case opcode_ige :
                case opcode_ile :
                case opcode_summon_class_class :
                case opcode_summon_interface_class :
                case opcode_summon_code_class :
                case opcode_summon_int_class :
                case opcode_summon_string_class :
                case opcode_summon_array_class :
                case opcode_os_decompose :
                case opcode_throw :
                case opcode_pop_catcher :
                case opcode_os_eq :
                case opcode_os_neq :
                //case opcode_os_push_null :
                case opcode_os_isnull :
                case opcode_short_call_0 :
                case opcode_short_call_1 :
                case opcode_short_call_2 :
                case opcode_short_call_3 :
                    opcodeAddress = codePointer;
                    System.out.print("        " + opcodeAddress + ":\t");
                    System.out.println(opcodeNames[opcode]);
                    break;

                case opcode_jmp :                       // int32 argument
                case opcode_djnz :
                case opcode_jz :
                    opcodeAddress = codePointer;
                    argInt32 = getInt32(code, codePointer+1);
                    codePointer +=4;
                    System.out.print("        " + opcodeAddress + ":\t");
                    System.out.print(opcodeNames[opcode] + " " + argInt32);
                    System.out.println("  // " + (opcodeAddress+1+argInt32) + ":");
                    break;

                case opcode_iconst_32bit :
                case opcode_os_load32 :
                case opcode_os_save32 :
                case opcode_os_compose32 :
                case opcode_push_catcher :
                case opcode_os_pull32 :
                case opcode_os_get32 :
                case opcode_os_set32 :
                case opcode_is_get32 :
                case opcode_is_set32 :
                    opcodeAddress = codePointer;
                    argInt32 = getInt32(code, codePointer+1);
                    codePointer +=4;
                    System.out.print("        " + opcodeAddress + ":\t");
                    System.out.println(opcodeNames[opcode] + " " + argInt32);
                    break;

                case opcode_iconst_8bit :               // byte argument
                case opcode_os_load8 :
                case opcode_os_save8 :
                    opcodeAddress = codePointer;
                    argByte = code[++codePointer] & 0xff;
                    System.out.print("        " + opcodeAddress + ":\t");
                    System.out.println(opcodeNames[opcode] + " " + argByte);
                    break;

                case opcode_call_8bit :                 // byte and int32 arguments
                    opcodeAddress = codePointer;
                    argByte = code[++codePointer] & 0xff;
                    argInt32 = getInt32(code, codePointer+1);
                    codePointer +=4;
                    System.out.print("        " + opcodeAddress + ":\t");
                    System.out.println(opcodeNames[opcode] + " " + argByte + " " + argInt32);
                    break;

                case opcode_call_32bit :                // int32 and int32 arguments
                    opcodeAddress = codePointer;
                    argInt32 = getInt32(code, codePointer+1);
                    codePointer +=4;
                    arg2Int32 = getInt32(code, codePointer+1);
                    codePointer +=4;
                    System.out.print("        " + opcodeAddress + ":\t");
                    System.out.println(opcodeNames[opcode] + " " + argInt32 + " " + arg2Int32);
                    break;

                case opcode_switch:
                    opcodeAddress = codePointer;
                    int tableSize = getInt32(code, codePointer+1);
                    codePointer +=4;
                    int shift = getInt32(code, codePointer+1);
                    codePointer +=4;
                    int divisor = getInt32(code, codePointer+1);
                    codePointer +=4;
                    System.out.print("        " + opcodeAddress + ":\t");
                    System.out.println(opcodeNames[opcode] + " table-size:" + tableSize +
                            " " + shift + "/" +divisor);
                    for (int i=0; i<tableSize; i++) {
                        argInt32 = getInt32(code, codePointer+1);
                        codePointer +=4;
                        System.out.println("            " + argInt32);
                    }
                    break;

                case opcode_sconst_bin :                // string argument
                case opcode_summon_by_name :
                    opcodeAddress = codePointer;
                    argBin = getBinData(code, codePointer+1);
                    codePointer += 4 + argBin.length;
                    System.out.print("        " + opcodeAddress + ":\t");
                    System.out.println(opcodeNames[opcode] + " ["+ argBin.length + "]\"" + new String(argBin) + "\"" /*+ "  // ["
                            + argBin.length + "] " + getHexStr(argBin)*/);
                    break;

                default:
                    System.out.println("Unknown opcode");
            }

        }
    }

    protected int getInt32(byte[] code, int ptr) {
        int v = code[ptr + 3] & 0xff;
        v |= (code[ptr + 2] & 0xff) << 8;
        v |= (code[ptr + 1] & 0xff) << 16;
        v |= (code[ptr] & 0xff) << 24;
        return v;
    }

//    public String getString(byte[] code, int ptr) {
//      int len = getInt32(code, ptr);
//      return new String(code, ptr+4, len);
//    }
    public byte[] getBinData(byte[] code, int ptr) {
      int len = getInt32(code, ptr);
      byte[] data = new byte[len];
      for (int i=ptr+4, z=0; i<ptr+4+len; data[z++] = code[i++]);
      return data;
    }

    public String getHexStr(byte[] data) {
        StringBuffer result = new StringBuffer();
        for(byte b : data) {
            result.append(Integer.toHexString(b& 0xff));
            result.append(' ');
        }
        return result.toString().toUpperCase();
    }
}
