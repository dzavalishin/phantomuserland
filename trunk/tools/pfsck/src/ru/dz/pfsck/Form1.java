package ru.dz.pfsck;

import java.nio.MappedByteBuffer;

//import javax.swing.*;

//using System.Data;
//using System.Linq;
//using System.Text;

//using u_int32_t = System.UInt32;
//using disk_page_no_t = System.UInt32;

public class Form1 //extends Form
{
	private Phantom_FS_Image m_Image;
	private final MappedByteBuffer map;
	private Phantom_Disk_Superblock superBlock;

	/*
	public Form1()
	{
		InitializeComponent();

//C# TO JAVA CONVERTER TODO TASK: Java has no equivalent to C#-style event wireups:
		Application.ThreadException += new System.Threading.ThreadExceptionEventHandler(Application_ThreadException);
	}
	*/


	public Form1(MappedByteBuffer map) {
		this.map = map;
		map.load();
		
		//System.out.println("limit = " + map.limit() );
		System.out.println("limit = " + map.limit()/1024 + "kb" );
		
		m_Image = new Phantom_FS_Image(map);
		
		superBlock = m_Image.getSuperBlock();
	}
	

	public final void Check()
	{
		OutputLine("superblock");
		OutputLine("-----------------");
		Output(""+superBlock);
		OutputLine("");
//if(true) return;
		OutputLine("Begin check " + new java.util.Date().toLocaleString() + "/");
		OutputLine("-----------------");

		BlockList freeList = null;
		BlockList lastShotContentList = null;
		BlockList prevShotContentList = null;

		ListDescriptor freeSpaceListDescriptor = null;
		ListDescriptor lastShotListDescriptor = null;
		ListDescriptor prevShotListDescriptor = null;

//C# TO JAVA CONVERTER TODO TASK: There is no preprocessor in Java:
		///#region Начальное заполнение списков
		{
			OutputLine("");
			OutputLine("");
			OutputLine("");
			OutputLine("Loading lists");
			OutputLine("-----------------");

			if (superBlock.getFree_list() != 0)
			{
				RefObject<ListDescriptor> tempRef_freeSpaceListDescriptor = new RefObject<ListDescriptor>(freeSpaceListDescriptor);
				freeList = m_Image.blockListContents(superBlock.getFree_list(), tempRef_freeSpaceListDescriptor);
				freeSpaceListDescriptor = tempRef_freeSpaceListDescriptor.argvalue;

				OutputLine("▫Free list loaded");
				Output(freeSpaceListDescriptor);
				Output(freeList);
			}

			if (superBlock.getLast_snap() != 0)
			{
				RefObject<ListDescriptor> tempRef_lastShotListDescriptor = new RefObject<ListDescriptor>(lastShotListDescriptor);
				lastShotContentList = m_Image.blockListContents(superBlock.getLast_snap(), tempRef_lastShotListDescriptor);
				lastShotListDescriptor = tempRef_lastShotListDescriptor.argvalue;

				OutputLine("▫Last snap loaded");
				Output(lastShotListDescriptor);
				Output(lastShotContentList);
			}

			if (superBlock.getPrev_snap() != 0)
			{
				RefObject<ListDescriptor> tempRef_prevShotListDescriptor = new RefObject<ListDescriptor>(prevShotListDescriptor);
				prevShotContentList = m_Image.blockListContents(superBlock.getPrev_snap(), tempRef_prevShotListDescriptor);
				prevShotListDescriptor = tempRef_prevShotListDescriptor.argvalue;

				OutputLine("▫Prev snap loaded");
				Output(prevShotListDescriptor);
				Output(prevShotContentList);
			}

			OutputLine("Done");
		}
//C# TO JAVA CONVERTER TODO TASK: There is no preprocessor in Java:
		///#endregion

		//TODO: проверить контрольную сумму суперблока

		//TODO: сравнить суперблоки

//C# TO JAVA CONVERTER TODO TASK: There is no preprocessor in Java:
		///#region Проверка типов блоков в описателях списков
		OutputLine("");
		OutputLine("Check magics");
		OutputLine("-----------------");
		if (freeSpaceListDescriptor != null)
		{
			blockTypeCheck(freeSpaceListDescriptor, Magics.DISK_STRUCT_MAGIC_FREEHEAD);
		}
		if (lastShotListDescriptor != null)
		{
			blockTypeCheck(lastShotListDescriptor, Magics.DISK_STRUCT_MAGIC_SNAP_LIST);
		}
		if (prevShotListDescriptor != null)
		{
			blockTypeCheck(prevShotListDescriptor, Magics.DISK_STRUCT_MAGIC_SNAP_LIST);
		}
		OutputLine("Done");
		///#endregion

		//TODO: статистика пересечения снимков
		///#region Статистика пересечения снимков
		///#endregion

//C# TO JAVA CONVERTER TODO TASK: There is no preprocessor in Java:
		///#region Чтоб списки не пересекались
		{
			java.util.HashMap<Integer, Boolean> t = new java.util.HashMap<Integer, Boolean>();

			OutputLine("");
			OutputLine("");
			OutputLine("Проверка на вхождение блоков не более чем в один список и не более чем единожды");
			OutputLine("-----------------");

			if (freeSpaceListDescriptor != null)
			{
				Output("▫Free list descr");

//ORIGINAL LINE: foreach (uint nBlock in freeSpaceListDescriptor)
				for (int nBlock : freeSpaceListDescriptor)
				{
					if (nBlock != 0)
					{
						t.put(nBlock, false);
					}
				}

				OutputLine("  Ok");
			}

			if (freeList != null)
			{
				Output("▫Free list");

//ORIGINAL LINE: foreach (uint nBlock in freeList)
				for (int nBlock : freeList)
				{
					if (nBlock != 0)
					{
						t.put(nBlock, false);
					}
				}

				OutputLine("  Ok");
			}

			if (lastShotListDescriptor != null)
			{
				Output("▫Содержимое списка описателей последнего снимка");

//C# TO JAVA CONVERTER WARNING: Unsigned integer types have no direct equivalent in Java:
//ORIGINAL LINE: foreach (uint nBlock in lastShotListDescriptor)
				for (int nBlock : lastShotListDescriptor)
				{
					if (nBlock != 0)
					{
						t.put(nBlock, false);
					}
				}

				OutputLine("  Ok");
			}

			if (lastShotContentList != null)
			{
				Output("▫Содержимое списка блоков последнего снимка");

//ORIGINAL LINE: foreach (uint nBlock in lastShotContentList)
				for (int nBlock : lastShotContentList)
				{
					if (nBlock != 0)
					{
						t.put(nBlock, false);
					}
				}

				OutputLine("  Ok");
			}

			if (prevShotListDescriptor != null)
			{
				Output("▫Prev snap descr");

//ORIGINAL LINE: foreach (uint nBlock in prevShotListDescriptor)
				for (int nBlock : prevShotListDescriptor)
				{
					if (nBlock != 0)
					{
						t.put(nBlock, false);
					}
				}

				OutputLine("  Ok");
			}

			if (prevShotContentList != null)
			{
				Output("▫Prev snap block list");

//ORIGINAL LINE: foreach (uint nBlock in prevShotContentList)
				for (int nBlock : prevShotContentList)
				{
					if (nBlock != 0)
					{
						t.put(nBlock, false);
					}
				}

				OutputLine("  Ok");
			}
			OutputLine("Done");

			{
				OutputLine("");
				OutputLine("Looking for unreferenced blocks between disk_start_page and free_start");
				OutputLine("-----------------");
				//List<uint> g = new List<uint>();
				int count = 0;

				for (int nBlock = superBlock.getDisk_start_page(); nBlock < superBlock.getFree_start(); nBlock++)
				{
					if (!t.containsKey(nBlock))
					{
						//g.Add(nBlock);
						count++;
					}
				}

				if (count > 0)
				{
					OutputLine("  не учтено блоков " + (new Integer(count)).toString() + " шт.");
				}

				OutputLine("Done");
			}

			{
				OutputLine("");
				OutputLine("Looking for references to blocks out of disk_start_page-disk_page_count");
				OutputLine("-----------------");
				//List<uint> g = new List<uint>();
				int count = 0;

				int leftBorder = superBlock.getDisk_start_page();

				int rightBorder = superBlock.getDisk_page_count();

				for (int nBlock : t.keySet())
				{
					if (nBlock < leftBorder || nBlock >= rightBorder)
					{
						count++;
					}
				}

				if (count > 0)
				{
					OutputLine("  out of bounds block references " + (new Integer(count)).toString() + " ");
				}
				else
				{
					OutputLine("None");
				}

				OutputLine("Done");
			}

		}

		OutputLine("");
		OutputLine("FS check finished. " + new java.util.Date().toLocaleString() + "/");
	}

	/** 
	 * Magics statistics - how many blocks of given magic type exist. Inexact. :(
	 **/
	private void magicsStats() {
		{
			OutputLine("");
			OutputLine("Block type statistics (есть вероятность ошибок в большую сторону, так как блоки данных могут быть приняты за блоки с меджиками)");
			OutputLine("-----------------");

			//ORIGINAL LINE: Dictionary<Magics, uint> magicsStatistics = new Dictionary<Magics, uint>();
			java.util.HashMap<Magics, Integer> magicsStatistics = new java.util.HashMap<Magics, Integer>();

			for( Magics enumVal : Magics.values() )
			{
				magicsStatistics.put(enumVal, 0);
			}

			//int last_block = superBlock.getDisk_start_page() + superBlock.getDisk_page_count() - 1;
			int last_block = superBlock.getDisk_page_count();

			for (int nBlock = superBlock.getDisk_start_page(); nBlock < last_block; nBlock++)
			{
				Block block = m_Image.ReadBlock(nBlock);

				if (magicsStatistics.containsKey(block.getMagic()))
				{
					//magicsStatistics.get(block.getMagic())++;
					Integer integer = magicsStatistics.get(block.getMagic());
					integer = integer+1;
				}
			}

			for (Magics magic : magicsStatistics.keySet())
			{
				OutputLine(magic.toString() + " " + magicsStatistics.get(magic).toString());
			}

			OutputLine("Done");
		}
	}

	public final void blockTypeCheck(ListDescriptor descriptor, Magics requestedType)
	{
//ORIGINAL LINE: foreach (uint nBlock in descriptor)
		for (int nBlock : descriptor)
		{
			Block block = m_Image.ReadBlock(nBlock);

			if (block.getMagic() != requestedType)
			{
				OutputLine("Ожидался блок типа " + requestedType.toString() + ", а обнаружен " + block.getMagic());
			}
		}
	}

	public final void Output(String message)
	{
		//textBox1.setText(textBox1.getText() + message);
		System.out.println(message);
	}

	public final void OutputLine(String message)
	{
		Output(message);
		//textBox1.setText(textBox1.getText() + message + "\r\n");
	}

	/*
	public final void Output(Phantom_Disk_Superblock superblock)
	{
		OutputLine("  версия 0x" + String.format("%08X", superblock.getVersion()));
		OutputLine("  контрольная сумма суперблока " + (new Integer(superblock.getChecksum())).toString() + (superblock.getIs_checksum_correct() ? "" : " (НЕ ВЕРНА)"));
		OutputLine("  disk_start_page " + (new Integer(superblock.getDisk_start_page())).toString());
		OutputLine("  disk_page_count " + (new Integer(superblock.getDisk_page_count())).toString());
		OutputLine("  начало предыдущего снимка " + (superblock.getPrev_snap() != 0 ? (new Integer(superblock.getPrev_snap())).toString() : "Отсутствует"));
		OutputLine("  начало текущего снимка " + (superblock.getLast_snap() != 0 ? (new Integer(superblock.getLast_snap())).toString() : "Отсутствует"));

		OutputLine("  начало описателя списка свободных блоков " + (new Integer(superblock.getFree_list())).toString());
		OutputLine("  free_start (начало монолитного свободного пространства) " + (new Integer(superblock.getFree_start())).toString());
	}
	*/
	
	public final void Output(BlockList block_list)
	{
		OutputLine("  всего блоков " + block_list.size());
		OutputLine("  «нулевых» блоков " + (new Integer(block_list.getNullCount())).toString());
	}

	public final void Output(ListDescriptor descriptor)
	{
		OutputLine("  количество блоков занимаемых описателем " + descriptor.size());
	}

/*	
	private void Application_ThreadException(Object sender, System.Threading.ThreadExceptionEventArgs e)
	{
		OutputLine("\r\nИсключение: " + e.Exception.Message);
	}

	private void listView1_RetrieveVirtualItem(Object sender, RetrieveVirtualItemEventArgs e)
	{
		Block b = m_Image.ReadBlock((int) e.ItemIndex);

		e.Item = new ListViewItem(e.ItemIndex.toString());
		e.Item.Tag = b;

	}

	private void listView1_SelectedIndexChanged(Object sender, EventArgs e)
	{
		if (listView1.SelectedIndices.size() != 0)
		{
			Block b = (Block)((listView1.Items[listView1.SelectedIndices[0]].Tag instanceof Block) ? listView1.Items[listView1.SelectedIndices[0]].Tag : null);

			if (b.getMagic() == Magics.DISK_STRUCT_MAGIC_SUPERBLOCK)
			{
				Phantom_Disk_Superblock p = new Phantom_Disk_Superblock(b.m_Buffer);
				propertyGrid1.SelectedObject = p;
				return;
			}

			propertyGrid1.SelectedObject = b;
		}
		else
		{
			propertyGrid1.SelectedObject = null;
		}
	}

	private void toolStripButton1_Click(Object sender, EventArgs e)
	{
		Check();
	}

	private void toolStripButton2_Click(Object sender, EventArgs e)
	{
		OpenFileDialog dlg = new OpenFileDialog();

		if (dlg.ShowDialog() == JOptionPane.OK_OPTION)
		{
			m_Image = new Phantom_FS_Image(dlg.FileName);

			listView1.VirtualListSize = m_Image.getBlockCount();

			toolStripButton1.setEnabled(true);
		}
	}


	/** 
	 Required designer variable.
	 
	* /
	private System.ComponentModel.IContainer components = null;

	/** 
	 Clean up any resources being used.
	 
	 @param disposing true if managed resources should be disposed; otherwise, false.
	* /
	@Override
	protected void dispose(boolean disposing)
	{
		if (disposing && (components != null))
		{
			components.dispose();
		}
		super.dispose(disposing);
	}

//C# TO JAVA CONVERTER TODO TASK: There is no preprocessor in Java:
	///#region Windows Form Designer generated code

	/** 
	 Required method for Designer support - do not modify
	 the contents of this method with the code editor.
	 
	* /
	private void InitializeComponent()
	{
		System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(Form1.class);
		this.listView1 = new System.Windows.Forms.ListView();
		this.columnHeader1 = new System.Windows.Forms.ColumnHeader();
		this.propertyGrid1 = new System.Windows.Forms.PropertyGrid();
		this.splitContainer1 = new System.Windows.Forms.SplitContainer();
		this.splitContainer2 = new System.Windows.Forms.SplitContainer();
		this.textBox1 = new System.Windows.Forms.TextBox();
		this.toolStripContainer1 = new System.Windows.Forms.ToolStripContainer();
		this.toolStrip1 = new System.Windows.Forms.ToolStrip();
		this.toolStripButton1 = new System.Windows.Forms.ToolStripButton();
		this.toolStripButton2 = new System.Windows.Forms.ToolStripButton();
		this.splitContainer1.Panel1.SuspendLayout();
		this.splitContainer1.Panel2.SuspendLayout();
		this.splitContainer1.SuspendLayout();
		this.splitContainer2.Panel1.SuspendLayout();
		this.splitContainer2.Panel2.SuspendLayout();
		this.splitContainer2.SuspendLayout();
		this.toolStripContainer1.ContentPanel.SuspendLayout();
		this.toolStripContainer1.TopToolStripPanel.SuspendLayout();
		this.toolStripContainer1.SuspendLayout();
		this.toolStrip1.SuspendLayout();
		this.SuspendLayout();
		// 
		// listView1
		// 
		this.listView1.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] { this.columnHeader1});
		this.listView1.Dock = System.Windows.Forms.DockStyle.Fill;
		this.listView1.FullRowSelect = true;
		this.listView1.setLocation(new System.Drawing.Point(0, 0));
		this.listView1.MultiSelect = false;
		this.listView1.setName("listView1");
		this.listView1.ShowItemToolTips = true;
		this.listView1.Size = new System.Drawing.Size(234, 361);
		this.listView1.TabIndex = 1;
		this.listView1.UseCompatibleStateImageBehavior = false;
		this.listView1.View = System.Windows.Forms.View.Details;
		this.listView1.VirtualMode = true;
//C# TO JAVA CONVERTER TODO TASK: Java has no equivalent to C#-style event wireups:
		this.listView1.SelectedIndexChanged += new System.EventHandler(this.listView1_SelectedIndexChanged);
//C# TO JAVA CONVERTER TODO TASK: Java has no equivalent to C#-style event wireups:
		this.listView1.RetrieveVirtualItem += new System.Windows.Forms.RetrieveVirtualItemEventHandler(this.listView1_RetrieveVirtualItem);
		// 
		// columnHeader1
		// 
		this.columnHeader1.setWidth(112);
		// 
		// propertyGrid1
		// 
		this.propertyGrid1.Dock = System.Windows.Forms.DockStyle.Fill;
		this.propertyGrid1.setLocation(new System.Drawing.Point(0, 0));
		this.propertyGrid1.setName("propertyGrid1");
		this.propertyGrid1.PropertySort = System.Windows.Forms.PropertySort.NoSort;
		this.propertyGrid1.Size = new System.Drawing.Size(390, 361);
		this.propertyGrid1.TabIndex = 2;
		// 
		// splitContainer1
		// 
		this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
		this.splitContainer1.setLocation(new System.Drawing.Point(0, 0));
		this.splitContainer1.setName("splitContainer1");
		// 
		// splitContainer1.Panel1
		// 
		this.splitContainer1.Panel1.Controls.Add(this.listView1);
		// 
		// splitContainer1.Panel2
		// 
		this.splitContainer1.Panel2.Controls.Add(this.propertyGrid1);
		this.splitContainer1.Size = new System.Drawing.Size(628, 361);
		this.splitContainer1.SplitterDistance = 234;
		this.splitContainer1.TabIndex = 3;
		// 
		// splitContainer2
		// 
		this.splitContainer2.Dock = System.Windows.Forms.DockStyle.Fill;
		this.splitContainer2.setLocation(new System.Drawing.Point(0, 0));
		this.splitContainer2.setName("splitContainer2");
		this.splitContainer2.Orientation = System.Windows.Forms.Orientation.Horizontal;
		// 
		// splitContainer2.Panel1
		// 
		this.splitContainer2.Panel1.Controls.Add(this.splitContainer1);
		// 
		// splitContainer2.Panel2
		// 
		this.splitContainer2.Panel2.Controls.Add(this.textBox1);
		this.splitContainer2.Size = new System.Drawing.Size(628, 443);
		this.splitContainer2.SplitterDistance = 361;
		this.splitContainer2.TabIndex = 4;
		// 
		// textBox1
		// 
		this.textBox1.Dock = System.Windows.Forms.DockStyle.Fill;
		this.textBox1.Font = new System.Drawing.Font("Courier New", 8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
		this.textBox1.setLocation(new System.Drawing.Point(0, 0));
		this.textBox1.Multiline = true;
		this.textBox1.setName("textBox1");
		this.textBox1.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
		this.textBox1.Size = new System.Drawing.Size(628, 78);
		this.textBox1.TabIndex = 5;
		// 
		// toolStripContainer1
		// 
		// 
		// toolStripContainer1.ContentPanel
		// 
		this.toolStripContainer1.ContentPanel.Controls.Add(this.splitContainer2);
		this.toolStripContainer1.ContentPanel.Size = new System.Drawing.Size(628, 443);
		this.toolStripContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
		this.toolStripContainer1.setLocation(new System.Drawing.Point(0, 0));
		this.toolStripContainer1.setName("toolStripContainer1");
		this.toolStripContainer1.Size = new System.Drawing.Size(628, 471);
		this.toolStripContainer1.TabIndex = 5;
		this.toolStripContainer1.setText("toolStripContainer1");
		// 
		// toolStripContainer1.TopToolStripPanel
		// 
		this.toolStripContainer1.TopToolStripPanel.Controls.Add(this.toolStrip1);
		// 
		// toolStrip1
		// 
		this.toolStrip1.Dock = System.Windows.Forms.DockStyle.None;
		this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] { this.toolStripButton2, this.toolStripButton1});
		this.toolStrip1.setLocation(new System.Drawing.Point(3, 0));
		this.toolStrip1.setName("toolStrip1");
		this.toolStrip1.Size = new System.Drawing.Size(256, 28);
		this.toolStrip1.TabIndex = 0;
		// 
		// toolStripButton1
		// 
		this.toolStripButton1.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.getText();
		this.toolStripButton1.setEnabled(false);
		this.toolStripButton1.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButton1.Image")));
		this.toolStripButton1.ImageTransparentColor = System.Drawing.Color.Magenta;
		this.toolStripButton1.setName("toolStripButton1");
		this.toolStripButton1.Size = new System.Drawing.Size(87, 25);
		this.toolStripButton1.setText("Проверка");
//C# TO JAVA CONVERTER TODO TASK: Java has no equivalent to C#-style event wireups:
		this.toolStripButton1.Click += new System.EventHandler(this.toolStripButton1_Click);
		// 
		// toolStripButton2
		// 
		this.toolStripButton2.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.getText();
		this.toolStripButton2.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButton2.Image")));
		this.toolStripButton2.ImageTransparentColor = System.Drawing.Color.Magenta;
		this.toolStripButton2.setName("toolStripButton2");
		this.toolStripButton2.Size = new System.Drawing.Size(126, 25);
		this.toolStripButton2.setText("Открыть файл");
//C# TO JAVA CONVERTER TODO TASK: Java has no equivalent to C#-style event wireups:
		this.toolStripButton2.Click += new System.EventHandler(this.toolStripButton2_Click);
		// 
		// Form1
		// 
		this.AutoScaleDimensions = new System.Drawing.SizeF(9F, 20F);
		this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
		this.ClientSize = new System.Drawing.Size(628, 471);
		this.Controls.Add(this.toolStripContainer1);
		this.setName("Form1");
		this.setText("Form1");
		this.splitContainer1.Panel1.ResumeLayout(false);
		this.splitContainer1.Panel2.ResumeLayout(false);
		this.splitContainer1.ResumeLayout(false);
		this.splitContainer2.Panel1.ResumeLayout(false);
		this.splitContainer2.Panel2.ResumeLayout(false);
		this.splitContainer2.Panel2.PerformLayout();
		this.splitContainer2.ResumeLayout(false);
		this.toolStripContainer1.ContentPanel.ResumeLayout(false);
		this.toolStripContainer1.TopToolStripPanel.ResumeLayout(false);
		this.toolStripContainer1.TopToolStripPanel.PerformLayout();
		this.toolStripContainer1.ResumeLayout(false);
		this.toolStripContainer1.PerformLayout();
		this.toolStrip1.ResumeLayout(false);
		this.toolStrip1.PerformLayout();
		this.ResumeLayout(false);

	}
*/
//C# TO JAVA CONVERTER TODO TASK: There is no preprocessor in Java:
	///#endregion
/*
	private System.Windows.Forms.ListView listView1;
	private System.Windows.Forms.ColumnHeader columnHeader1;
	private System.Windows.Forms.PropertyGrid propertyGrid1;
	private System.Windows.Forms.SplitContainer splitContainer1;
	private System.Windows.Forms.SplitContainer splitContainer2;
	private System.Windows.Forms.TextBox textBox1;
	private System.Windows.Forms.ToolStripContainer toolStripContainer1;
	private System.Windows.Forms.ToolStrip toolStrip1;
	private System.Windows.Forms.ToolStripButton toolStripButton1;
	private System.Windows.Forms.ToolStripButton toolStripButton2;
*/	
}