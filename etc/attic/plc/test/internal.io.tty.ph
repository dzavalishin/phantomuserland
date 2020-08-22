package .internal.io;

class tty 
{
	string	getwc() [16] {} 

	void    putws( var text : string ) [17] {}
	void    gotoxy( var x: int, var y: int ) [19] {}
	void	clear() [20] {}
	void	setcolor( var color : int ) [21] {}

	//void	fill() [22] {}
	//void	putblock() [23] {}

};
