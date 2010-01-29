package .ru.dz.phantom.tetris;

import .ru.dz.phantom.tetris.playingfield;

class figure {
	/*
	** Matrix represents the concrete figure design.
	*/
	var _mask : playingfield;
	/*
	** Temporary matrix for storing while rotating.
	*/
	var _tempMask : playingfield;
	// private static readonly
	var _maskSize : int;
	//
	var _relativeX : int;
	var _relativeY : int;
	var _color : int;

	void init(var field : playingfield, var color : int) {
		_maskSize = 5;
		//
		_relativeX = field.getWidth() / 2;
		_relativeY = field.getHeight() - 1 + 3;
		_color = color;
		//
		_mask = new playingfield();
		_mask.init(_maskSize, _maskSize);
		_tempMask = new playingfield();
		_tempMask.init(_maskSize, _maskSize);
		// Refactor this : will be overriden in each derived class
		_mask.setBoxAt(1, 2, 1);
		_mask.setBoxAt(2, 2, 1);
		_mask.setBoxAt(3, 2, 1);
		_mask.setBoxAt(2, 1, 1);
	}
	
	int getColor() {
		return (_color);
	}
	
	int getMaskSize() {
		return (_maskSize);
	}
	
	int getBoxAt(var x : int, var y : int) {
		return (_mask.getBoxAt(x, y));
	}
	
	// private
	int getRealX(var x : int) {
		return (x - (_maskSize / 2) + _relativeX);
	}
	
	// private
	int getRealY(var y : int) {
		return (y - (_maskSize / 2) + _relativeY);
	}
	
	// private
	int isMovingToPointAvailable(var field : playingfield, var movedX : int, var movedY : int) {
		if (movedY >= field.getHeight()) {
			if ((movedX < 0) || (movedX >= field.getWidth())) {
				return (0);
			}
		} else {
			if ((movedX < 0) || (movedX >= field.getWidth()) || (movedY < 0)) {
				return (0);
			} else {
				if (field.getBoxAt(movedX, movedY) != 0) {
					return (0);
				}
			}
		}
	}
		
	int isVisibleAtPlayground(var field : playingfield) {
		var x : int;
		var y : int;
		var visibleDetected : int;
		visibleDetected = 0;
		y = 0;
		while ((y < _maskSize) && (visibleDetected == 0)) {
			x = 0;
			while ((x < _maskSize) && (visibleDetected == 0)) {
				if (_mask.getBoxAt(x, y) != 0) {
					var realX : int;
					var realY : int;
					realX = getRealX(x);
					realY = getRealY(y);
					//
					if ( (realX >= 0) && (realX < field.getWidth()) && (realY >= 0) && (realY < field.getHeight()) ) {
						visibleDetected = 1;
					}
				}
				x = x + 1;
			}
			y = y + 1;
		}
		//
		return (visibleDetected);
	}
	
	int isFullyVisibleAtPlayground(var field : playingfield) {
		var x : int;
		var y : int;
		var fullyVisible : int;
		fullyVisible = 1;
		y = 0;
		while ((y < _maskSize) && (fullyVisible == 1)) {
			x = 0;
			while ((x < _maskSize) && (fullyVisible == 1)) {
				if (_mask.getBoxAt(x, y) != 0) {
					var realX : int;
					var realY : int;
					realX = getRealX(x);
					realY = getRealY(y);
					//
					if (! ( (realX >= 0) && (realX < field.getWidth()) && (realY >= 0) && (realY < field.getHeight()) ) ) {
						fullyVisible = 0;
					}
				}
				x = x + 1;
			}
			y = y + 1;
		}
		//
		return (fullyVisible);
	}
	
	int canMoveDown(var field : playingfield) {
		var moveIsAllowed : int;
		moveIsAllowed = 1;
		var x : int;
		var y : int;
		y = 0;
		while ((y < _maskSize) && (moveIsAllowed == 1)) {
			x = 0;
			while ((x < _maskSize) && (moveIsAllowed == 1)) {
				if (_mask.getBoxAt(x, y) != 0) {
					var movedX : int;
					var movedY : int;
					////// Calculations modified coordinates //////
					movedX = x;
					movedY = y - 1;
					////////////
					movedX = getRealX(movedX);
					movedY = getRealY(movedY);
					// Any moving is constrained by left and right corners and by bottom but not by top
					if (!isMovingToPointAvailable(field, movedX, movedY)) {
						moveIsAllowed = 0;
					}
				}
				x = 1 + x;
			}
			y = y + 1;
		}
		return (moveIsAllowed);
	}
	
	void moveDown(var field : playingfield) {
		if (!canMoveDown(field)) {
			throw "InvalidOperation : cannot move down a figure.";
		}
		//
		_relativeY = _relativeY - 1;
	}
	
	int canMoveLeft(var field : playingfield) {
		var moveIsAllowed : int;
		moveIsAllowed = 1;
		var x : int;
		var y : int;
		y = 0;
		while ((y < _maskSize) && (moveIsAllowed == 1)) {
			x = 0;
			while ((x < _maskSize) && (moveIsAllowed == 1)) {
				if (_mask.getBoxAt(x, y) != 0) {
					var movedX : int;
					var movedY : int;
					////// Calculations modified coordinates //////
					movedX = x - 1;
					movedY = y;
					////////////
					movedX = getRealX(movedX);
					movedY = getRealY(movedY);
					// Any moving is constrained by left and right corners and by bottom but not by top
					if (!isMovingToPointAvailable(field, movedX, movedY)) {
						moveIsAllowed = 0;
					}
				}
				x = 1 + x;
			}
			y = y + 1;
		}
		return (moveIsAllowed);
	}
	
	void moveLeft(var field : playingfield) {
		if (!canMoveLeft(field)) {
			throw "InvalidOperation : cannot move left a figure.";
		}
		//
		_relativeX = _relativeX - 1;
	}
	
	int canMoveRight(var field : playingfield) {
		var moveIsAllowed : int;
		moveIsAllowed = 1;
		var x : int;
		var y : int;
		y = 0;
		while ((y < _maskSize) && (moveIsAllowed == 1)) {
			x = 0;
			while ((x < _maskSize) && (moveIsAllowed == 1)) {
				if (_mask.getBoxAt(x, y) != 0) {
					var movedX : int;
					var movedY : int;
					////// Calculations modified coordinates //////
					movedX = x + 1;
					movedY = y;
					////////////
					movedX = getRealX(movedX);
					movedY = getRealY(movedY);
					// Any moving is constrained by left and right corners and by bottom but not by top
					if (!isMovingToPointAvailable(field, movedX, movedY)) {
						moveIsAllowed = 0;
					}
				}
				x = 1 + x;
			}
			y = y + 1;
		}
		return (moveIsAllowed);
	}
	
	void moveRight(var field : playingfield) {
		if (!canMoveRight(field)) {
			throw "InvalidOperation : cannot move right a figure.";
		}
		//
		_relativeX = _relativeX + 1;
	}
	
	// #region# Rotating
	
	// private
	int convertToVirtualX(var x : int) {
		return (x - _maskSize / 2);
	}
	
	// private
	int convertFromVirtualX(var virtualX : int) {
		return (virtualX + _maskSize / 2);
	}
	
	// private
	int convertToVirtualY(var y : int) {
		return (y - _maskSize / 2);
	}
	
	// private
	int convertFromVirtualY(var virtualY : int) {
		return (virtualY + _maskSize / 2);
	}
	
	// private
	int getRotatedX(var x : int, var y : int, var isLeftRotating : int) {
		if (isLeftRotating) {
			return (convertFromVirtualX (convertToVirtualY(y)) );
		} else {
			return (convertFromVirtualX (0 - convertToVirtualY(y)) );
		}
	}
	
	// private
	int getRotatedY(var x : int, var y : int, var isLeftRotating : int) {
		if (isLeftRotating) {
			return (convertFromVirtualY (0 - convertToVirtualX(x)) );
		} else {
			return (convertFromVirtualY (convertToVirtualX(x)) );
		}
	}
	
	// private
	int canRotate(var field : playingfield, var isLeftRotating : int) {
		_tempMask.clean();
		//
		var moveIsAllowed : int;
		moveIsAllowed = 1;
		var x : int;
		var y : int;
		y = 0;
		while ((y < _maskSize) && (moveIsAllowed == 1)) {
			x = 0;
			while ((x < _maskSize) && (moveIsAllowed == 1)) {
				if (_mask.getBoxAt(x, y) != 0) {
					var movedX : int;
					var movedY : int;
					////// Calculations modified coordinates //////
					movedX = getRotatedX(x, y, isLeftRotating);
					movedY = getRotatedY(x, y, isLeftRotating);
					_tempMask.setBoxAt(movedX, movedY, _mask.getBoxAt(x, y));
					////////////
					movedX = getRealX(movedX);
					movedY = getRealY(movedY);
					// Any moving is constrained by left and right corners and by bottom but not by top
					if (!isMovingToPointAvailable(field, movedX, movedY)) {
						moveIsAllowed = 0;
					}
				}
				x = 1 + x;
			}
			y = y + 1;
		}
		return (moveIsAllowed);
	}
	
	int canRotateLeft(var field : playingfield) {
		return (canRotate(field, 1));
	}
	
	int canRotateRight(var field : playingfield) {
		return (canRotate(field, 0));
	}
	
	// private
	void rotate(var field : int, var isLeftRotating : int) {
		if (!canRotate(field, isLeftRotating)) {
			throw "InvalidOperation : cannot rotate a figure.";
		}
		// Copy rotated _tempMask to _mask
		var x : int;
		var y : int;
		y = 0;
		while (y < _maskSize) {
			x = 0;
			while (x < _maskSize) {
				_mask.setBoxAt(x, y, _tempMask.getBoxAt(x, y));
				x = x + 1;
			}
			y = y + 1;
		}
	}
	
	void rotateLeft(var field : playingfield) {
		rotate(field, 1);
	}
	
	void rotateRight(var field : playingfield) {
		rotate(field, 0);
	}
	
	// #endregion#
	
	/// <summary>
	/// It modifies the playing field.
	/// </summary>
	void finishMovement(var field : playingfield) {
		// Mode of non-classic finish of movement, by default set in FALSE
		var droppingBoxesDownMode : int;
		droppingBoxesDownMode = 0;
		var x : int;
		var y : int;
		x = 0;
		while (x < _maskSize) {
			var realX : int;
			realX = getRealX(x);
			//
			y = 0;
			while (y < _maskSize) {
				if (_mask.getBoxAt(x, y) != 0) {
					var realY : int;
					realY = getRealY(y);
					if ((realX >= 0) && (realX < field.getWidth()) && (realY >= 0) && (realY < field.getHeight())) {
						if (field.getBoxAt(realX, realY) != 0) {
							throw "InvalidOperation : no white space for figure part.";
						}
						// Handles falling boxes
						if (droppingBoxesDownMode) {
							while ((realY > 0) && (field.getBoxAt(realX, realY) == 0)) {
								realY = realY - 1;
							}
							// Step back if we stopped because region is preempted
							if (field.getBoxAt(realX, realY) != 0) {
								realY = realY + 1;
							}
						}
						field.setBoxAt(realX, realY, _color);
					} else {
						throw "InvalidOperation : some part of figure is invisible in the persistence moment.";
					}
				}
				y = y + 1;
			}
			x = x + 1;
		}
	}
};