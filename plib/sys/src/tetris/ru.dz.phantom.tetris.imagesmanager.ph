package .ru.dz.phantom.tetris;

class imagesmanager {
	.internal.string getImageRed() { 
		return import "src/tetris/images/box_red.pbm";
	}
	
	.internal.string getImageGreen() {
		return import "src/tetris/images/box_green.pbm";
	}
	
	.internal.string getImageBlue() {
		return import "src/tetris/images/box_blue.pbm";
	}
	
	.internal.string getImageOrange() {
		return import "src/tetris/images/box_orange.pbm";
	}
	
	.internal.string getImageGray() {
		return import "src/tetris/images/box_gray.pbm";
	}
};
