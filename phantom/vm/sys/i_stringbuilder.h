#ifndef _I_stringbuilder
#define _I_stringbuilder

/**
 * @struct data_area_4_stringbuilder
 * 
 * @brief Mutable string
 * 
 * Size of buffer is usually more than size of string.
 * 
 * Guarnteed to have binary zero after string end, not counted in len.
 * 
 * @todo use me for line edit control as buffer.
 * 
 * 
**/

struct data_area_4_stringbuilder
{
    char *                              str;          //< String
    uint32_t                            len;          //< Length of value
    uint32_t                            bufsize;      //< Length of buffer
    uint32_t                            update_count; //< Times we increased len
    pvm_object_t                        buffer;       //< Usual string we keep as a storage
};



#endif // _I_stringbuilder
