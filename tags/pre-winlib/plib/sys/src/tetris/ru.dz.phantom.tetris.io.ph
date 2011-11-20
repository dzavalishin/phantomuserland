package .ru.dz.phantom.tetris;

import .ru.dz.phantom.tetris.utilites;

class io {
	var _utilites : utilites;
	/* Key code pressed / or emulated ? :-) /
	** 0 - none
	** 1 - left
	** 2 - down
	** 3 - right
	** 4 - exit
	*/
	var lastKeyPressed : int;
	
	/*
	** Constants
	*/
	var minLastKeyPressedCode : int;
	var maxLastKeyPressedCode : int;
	
	void init( var utilites : utilites ) {
		_utilites = utilites;
		lastKeyPressed = 0;
		//
		minLastKeyPressedCode = 0;
		maxLastKeyPressedCode = 4;
	}

	int isAnyKeyPressed( ) {
		if ( _utilites.generateRandom( 0, 5 ) == 0 ) {
			lastKeyPressed = _utilites.generateRandom( minLastKeyPressedCode, maxLastKeyPressedCode );
			if ( lastKeyPressed > maxLastKeyPressedCode || lastKeyPressed < minLastKeyPressedCode ) {
				throw "InvalidOperation : lastKeyPressed has invalid value.";
			}
			return ( 1 );
		}
		//
		return ( 0 );
	}
	
	int getLastPressedKey( ) {
		return ( lastKeyPressed );
	}
};
