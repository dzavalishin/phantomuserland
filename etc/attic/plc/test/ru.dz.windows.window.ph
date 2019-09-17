import .internal.io.tty;

package .ru.dz.windows;

import .ru.dz.windows.root;

class window
{
	var console : .ru.dz.windows.root;
	var xsize : int;
	var ysize : int;
	var xpos : int;
	var ypos : int;

	var border_color : int;

	void init( var r : root )
	{
	console = r;
/*
	xsize = 10; ysize = 3;
	xpos = 5; ypos = 5;

	border_color = 14 | (3*32);

console.putws("window init");
*/
	}

	void gotoxy( var x : int, var y : int)
	{
	console.gotoxy(x+xpos,y+ypos);
	}

	void putws( var s : string )
	{
	console.putws(s);
	}

	void display()
	{
	//gotoxy(0,0);
	//putws("*----");
	//console.putws("*----");
	}

};




