package ru.dz.pdb;

public class ChecksumException extends Exception {

	public ChecksumException() {
	}

	public ChecksumException(String message) {
		super(message);
	}

	public ChecksumException(Throwable cause) {
		super(cause);
	}

	public ChecksumException(String message, Throwable cause) {
		super(message, cause);
	}
	
}
