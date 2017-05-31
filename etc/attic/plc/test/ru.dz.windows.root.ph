import .internal.io.tty;
import .ru.dz.windows.window;

package .ru.dz.windows;

class root
{
	var console : .internal.io.tty;
//	var ch : string;

	void init( 	var _console : .internal.io.tty )
	{
	console = _console;
	console.putws(" in root...");
	}

	void gotoxy( var x : int, var y : int)
	{
	console.gotoxy(x,y);
	}

	void putws( var s : string )
	{
	console.putws(s);
	}


	window window()
	{
	var ret : .ru.dz.windows.window;
	ret = new .ru.dz.windows.window();
	ret.init(this);

ret.display();

	return ret;
	}

	void test()
	{
//var ch : string;
string ch;

	console.gotoxy(0,3);
	console.putws("Hello on new console (root win)!");
	console.gotoxy(10,10);
	console.setcolor(14);
	console.putws("Pos 10,10");


	console.gotoxy(0,0);
	console.putws("\n");


//	var ch : string;
//	while(1)
	{
		ch = console.getwc();
//		if( c == "\b" ) break;
//		console.putws(c);
	}


	var w : .ru.dz.windows.window;

//	w = window();
console.putws(" ...after window init");
//	w.display();
//console.putws(" ...after window.display");
	}


};




