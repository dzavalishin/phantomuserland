package .ru.dz.phantom.tetris;

class playingfield {
	var _width : int;
	var _height : int;
	/*
	** 'n' - No box (Gray) 0
	** '1' - Red 1
	** '2' - Orange 2
	** '3' - Green 3
	** '4' - Blue 4
	*/
	var _matrix : void[ ];
	
	void init( var width : int, var height : int ) {
		if ( ( width < 0 ) || ( height < 0 ) || ( width > 30 ) ) {
			throw "ArgumentException : width and height cannot be below than zero. Width must be <= 50.";
		}
		//
		_width = width;
		_height = height;
		_matrix = new void[ ]( );
		//
		var y : int;
		y = 0;
		while ( y < height ) {
			var row : string;
			row = "000000000000000000000000000000";
			//
			_matrix[ y ] = row;
			y = y + 1;
		}
	}
	
	void clean() {
		var y : int;
		y = 0;
		while ( y < _height ) {
			var row : string;
			row = "000000000000000000000000000000";
			//
			_matrix[ y ] = row;
			y = y + 1;
		}
	}
	
	int getWidth() {
		return (_width);
	}
	
	int getHeight() {
		return (_height);
	}
	
	/* private */
	void validateCoords( var x : int, var y : int ) {
		if ((x < 0) || (x >= _width)) {
			var exceptionString : string;
			exceptionString = "ArgumentException in validateCoords X = ";
			throw exceptionString.concat(x.toString());
		}
		if ((y < 0) || (y >= _height)) {
			var exceptionString : string;
			exceptionString = "ArgumentException in validateCoords Y = ";
			throw exceptionString.concat(y.toString());
		}
	}
	
	int getBoxAt( var x : int, var y : int ) {
		validateCoords( x, y );
		//
		var row : string;
		row = _matrix[ y ];
		var character : int;
		character = row.charAt( x );
		//
		switch ( character ) {
			case 48 : {
				return 0;
			}
			case 49 : {
				return 1;
			}
			case 50 : {
				return 2;
			}
			case 51 : {
				return 3;
			}
			case 52 : {
				return 4;
			}
			default : {
				throw "InvalidOperationException";
			}
		}
	}
	
	
	void setBoxAt( var x : int, var y : int, var box : int ) {
		validateCoords( x, y );
		if (( box < 0 ) || ( box > 4 )) {
			throw "ArgumentException : box";
		}
		//
		var row : string;
		row = _matrix[ y ];
		var rowToReplace : string;
		// ERROR
		//rowToReplace = row.substring(0, x).concat("1").concat(row.substring(x + 1, row.length() - x));
		
		var strFirstPart : string;
		strFirstPart = row.substring( 0, x );
		
		var boxString : string;
		switch ( box ) {
			case 0 : {
				boxString = "0";
				break;
			}
			case 1 : {
				boxString = "1";
				break;
			}
			case 2 : {
				boxString = "2";
				break;
			}
			case 3 : {
				boxString = "3";
				break;
			}
			case 4 : {
				boxString = "4";
				break;
			}
			default : {
				throw "InvalidOperation : box value is out of range.";
			}
		}
		rowToReplace = strFirstPart.concat( boxString );
		
		var strLastPart : string;
		if ( x + 1 < row.length() ) {
			strLastPart = row.substring( x + 1, row.length( ) - 1 - x );
			rowToReplace = rowToReplace.concat( strLastPart );
		}
		
		_matrix[ y ] = rowToReplace;
	}
};