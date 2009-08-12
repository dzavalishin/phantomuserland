package ru.dz.phantom.snapdump;

public class InvalidPhantomMemoryException extends Exception {

	public InvalidPhantomMemoryException() {
	}

	public InvalidPhantomMemoryException(String message) {
		super(message);
	}

	public InvalidPhantomMemoryException(Throwable cause) {
		super(cause);
	}

	public InvalidPhantomMemoryException(String message, Throwable cause) {
		super(message, cause);
	}

}
