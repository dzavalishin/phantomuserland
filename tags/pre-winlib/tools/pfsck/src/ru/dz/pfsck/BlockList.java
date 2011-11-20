package ru.dz.pfsck;

//class УпакованныйСписокБлоков
//{
//    Dictionary<disk_page_no_t, disk_page_no_t> СписокИнтерваловБлоков;
//    List<u_int32_t> СписокОтдельныхБлоков = new List<uint>();
//    List<Интервал> СписокИнтерваловБлоков = new List<Интервал>();

//    public void Fill(FileStream stream, disk_page_no_t ПервыйБлок)
//    {
//        //reader.BaseStream.Seek(, SeekOrigin.
//    }

//    class Интервал
//    {
//        public u_int32_t ПервыйБлок;
//        public u_int32_t ПоследнийБлок;
//    }

//    public void FromList(List<u_int32_t> Список)
//    {
//        for(
//    }
//}

@SuppressWarnings("serial")
public class BlockList extends java.util.ArrayList<Integer>
{
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: public uint getКоличествоНулевыхСсылок()
	public final int getNullCount()
	{
//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: uint counter = 0;
		int counter = 0;

//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: foreach (System.UInt32 value in this)
		for (int value : this)
		{
			if (value == 0)
			{
				counter++;
			}
		}

		return counter;
	}
}
