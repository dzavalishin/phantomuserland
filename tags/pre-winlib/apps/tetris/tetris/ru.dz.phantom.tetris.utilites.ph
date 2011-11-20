package .ru.dz.phantom.tetris;

class utilites {
	var _internal_counter : int;
	var _bootObject;
	
	void init( var bootObject ) {
		// NOTE : This should be removed to real random number retrieving
		_internal_counter = 0;
		_bootObject = bootObject;
	}

	int generateRandom( var minValue : int, var maxValue : int ) {
		if ( maxValue <= minValue ) {
			throw "ArgumentException";
		}
		//
		_internal_counter = _internal_counter + 11745 - ( _internal_counter / 5 );
		if ( _internal_counter > 60000 ) {
			_internal_counter = _internal_counter - 50000;
		}
		//
		var divided : int;
		divided = _internal_counter / ( maxValue - minValue + 1 );
		return ( minValue + _internal_counter - divided * ( maxValue - minValue + 1 ) );
	}
	
	void sleepMilliseconds( var milliseconds : int ) {
		// it will wait not less than for a second in win env anyway yet
		_bootObject.21( milliseconds );
	}
	
	void sleep( var seconds : int ) {
		sleepMilliseconds( seconds * 1000 );
	}
};