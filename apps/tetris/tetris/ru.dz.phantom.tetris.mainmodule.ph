package .ru.dz.phantom.tetris;

import .internal.io.tty;
import .internal.bitmap;

import .ru.dz.phantom.tetris.imagesmanager;
import .ru.dz.phantom.tetris.utilites;
import .ru.dz.phantom.tetris.playingfield;
import .ru.dz.phantom.tetris.figure;
import .ru.dz.phantom.tetris.io;

class mainmodule {
	/*
	** Readonly fields (initialized in constructor and cannot be changed afterwards).
	*/
	var _bootObject;
	var _console : .internal.io.tty;
	var _playingFieldWidth : int;
	var _playingFieldHeight : int;
	var _utilites : utilites;
	var _io : io;
	var _field : playingfield;
	var _figure : figure;
	var _figurePartiallyShown : int;

	var _imageRed : .internal.bitmap;
	var _imageBlue : .internal.bitmap;
	var _imageGray : .internal.bitmap;
	var _imageOrange : .internal.bitmap;
	var _imageGreen : .internal.bitmap;

	void print( var input : string ) {
        _bootObject.16(input);
    }

	void init( var bootObject, var console : .internal.io.tty, var playingFieldWidth : int, var playingFieldHeight : int ) {
		_bootObject = bootObject;
		_console = console;
		_playingFieldWidth = playingFieldWidth;
		_playingFieldHeight = playingFieldHeight;
		//
		_utilites = new utilites( );
		_utilites.init( _bootObject );
		//
		_io = new io( );
		_io.init( _utilites );
		//
		_field = new playingfield( );
		_field.init( _playingFieldWidth, _playingFieldHeight );
		//
		var imagesManager : imagesmanager;
		imagesManager = new imagesmanager();
		
		_imageRed = new .internal.bitmap();
		_imageGreen = new .internal.bitmap();
		_imageGray = new .internal.bitmap();
		_imageBlue = new .internal.bitmap();
		_imageOrange = new .internal.bitmap();
		
		_imageRed.loadFromString(imagesManager.getImageRed());
		_imageGreen.loadFromString(imagesManager.getImageGreen());
		_imageGray.loadFromString(imagesManager.getImageGray());
		_imageBlue.loadFromString(imagesManager.getImageBlue());
		_imageOrange.loadFromString(imagesManager.getImageOrange());
	}

	void runGame(  ) {
		//intro( );
		
		// Initialize a playing field
		_figure = new figure();
		_figure.init(_field, 1);
		_figurePartiallyShown = 1;
		
		while ( ( !( isExitCommandReceived( ) ) ) && ( !( isGameOver( ) ) ) ) {
			if ( _io.isAnyKeyPressed( ) ) {
				var c : int;
				c = _io.getLastPressedKey();
				if (c == 1) {
					dispatchCommand(c);
				}
			}
			
			dispatchCommand(2);
			
			if (_figure.canMoveDown(_field) == 0) {
				if (_figure.isFullyVisibleAtPlayground(_field)) {
					print("Figure is at the Earth.\n");
					_figure.finishMovement(_field);
					//
					_figure = new figure();
					_figure.init(_field, 3);
					_figurePartiallyShown = 1;
				} else {
					//_figure.finishMovement(_field);
					//repaintPlayground();
					print("Game over.");
					throw "GameOver";
				}
			} else {
				//if (_figurePartiallyShown) {
				//	if (_figure.isFullyVisibleAtPlayground(_field)) {
				//		_figurePartiallyShown = 0;
				//	}
				//}
				_figure.moveDown(_field);
			}
			
			deleteFilledRows();
			
			repaintPlayground();
			_utilites.sleep( 1 );
		}
	}
	
	void repaintPlayground() {
		repaintField();
		repaintFigure();
	}
	
	void deleteFilledRows() {
		while (isLowerRowFilled()) {
			deleteLowerRow();
		}
	}
	
	int isLowerRowFilled() {
		var x : int;
		var rowFilled : int;
		rowFilled = 1;
		//
		x = 0;
		while ((x < _field.getWidth()) && rowFilled) {
			if (_field.getBoxAt(x, 0) == 0) {
				rowFilled = 0;
			}
			x = x + 1;
		}
		//
		return (rowFilled);
	}
	
	void deleteLowerRow() {
		var x : int;
		var y : int;
		//
		y = 0;
		while (y < _field.getHeight() - 1) {
			x = 0;
			while (x < _field.getWidth()) {
				_field.setBoxAt(x, y, _field.getBoxAt(x, y + 1));
				x = x + 1;
			}
			y = y + 1;
		}
	}
	
	/* private */
	void repaintField() {
		var x : int;
		var y : int;
		y = 0;
		while (y < _field.getHeight()) {
			x = 0;
			while (x < _field.getWidth()) {
				var box : int;
				box = _field.getBoxAt(x, y);
				switch (box) {
					case 0 : {
						_imageGray.paintTo(_console, x * 20, y * 20);
						break;
					}
					case 1 : {
						_imageRed.paintTo(_console, x * 20, y * 20);
						break;
					}
					case 2 : {
						_imageOrange.paintTo(_console, x * 20, y * 20);
						break;
					}
					case 3 : {
						_imageGreen.paintTo(_console, x * 20, y * 20);
						break;
					}
					case 4 : {
						_imageBlue.paintTo(_console, x * 20, y * 20);
						break;
					}
					default : {
						throw "InvalidOperation : unknown box code retrieved during repainting.";
					}
				}
				x = x + 1;
			}
			y = y + 1;
		}
	}
	
	/* private */
	void repaintFigure() {
		var x : int;
		var y : int;
		y = 0;
		while (y < _figure.getMaskSize()) {
			x = 0;
			while (x < _figure.getMaskSize()) {
				var box : int;
				box = _figure.getBoxAt(x, y);
				var realX : int;
				var realY : int;
				// Dumping the {x; y} => {realX; realY} convertion
				/*
				var dumpString : string;
				dumpString = "x = ";
				dumpString = dumpString.concat(x.toString());
				dumpString = dumpString.concat("; y = ");
				dumpString = dumpString.concat(y.toString());
				dumpString = dumpString.concat("; realX = ");
				*/
				realX = _figure.getRealX(x);
				/*dumpString = dumpString.concat(realX.toString());
				dumpString = dumpString.concat("; realY = ");
				*/
				realY = _figure.getRealY(y);
				/*
				dumpString = dumpString.concat(y.toString());
				print(dumpString.concat("\n"));
				*/
				//
				if ((realX >= 0) && (realX < _field.getWidth()) && (realY >= 0) && (realY < _field.getHeight())) {
					if (box == 1) {
						var color : int;
						color = _figure.getColor();
						switch (color) {
							case 0 : {
								_imageGray.paintTo(_console, realX * 20, realY * 20);
								break;
							}
							case 1 : {
								_imageRed.paintTo(_console, realX * 20, realY * 20);
								break;
							}
							case 2 : {
								_imageOrange.paintTo(_console, realX * 20, realY * 20);
								break;
							}
							case 3 : {
								_imageGreen.paintTo(_console, realX * 20, realY * 20);
								break;
							}
							case 4 : {
								_imageBlue.paintTo(_console, realX * 20, realY * 20);
								break;
							}
							default : {
								throw "InvalidOperation : unknown box code retrieved during repainting.";
							}
						}
					}
				}
				x = x + 1;
			}
			y = y + 1;
		}
	}
	
	void intro( ) {
		var j : int;
		j = 0;
        while ( j < 10 ) {
			_console.putws( "Loading.." );
			var rand : int;
			rand = _utilites.generateRandom(0, 1);
			// *** This string causes an error during compiling :
			// _console.putws( _utilites.generateRandom( 0, 1 ).toString( ) );
			_console.putws( rand.toString( ) );
			_console.putws( "\n" );
			j = j + 1;
			_utilites.sleep( 1 );
        }
	}
	
	void dispatchCommand( var commandCode : int ) {
		/*
		** 0 - none
		** 1 - left
		** 2 - down
		** 3 - right
		** 4 - exit
		*/
		switch ( commandCode ) {
			case 0 :
				//_console.putws( "Command received : NONE" );
				break;
			case 1 :
				print("Trying to move left.\n");
				if (_figure.canMoveLeft(_field)) {
					_figure.moveLeft(_field);
				}
				print("Moving left OK.\n");
				break;
			case 2 : {
				print("Trying to rotate left.\n");
				if (_figure.canRotateLeft(_field)) {
					_figure.rotateLeft(_field);
				}
				print("Rotating left OK.\n");
				break;
			}
			case 3 : {
				if (_figure.canMoveRight(_field)) {
					_figure.moveRight(_field);
				}
				break;
			}
			case 4 : {
				throw "NotImplemented";
			}
			default : {
					print( "Command received : UNKNOWN ( " );
					print( commandCode.toString( ) );
					print( ")" );
				}
		}
		//
		print( "\n" );
	}
	
	int isExitCommandReceived( ) {
		return ( 0 );
	}
	
	int isGameOver( ) {
		return ( 0 );
	}
};