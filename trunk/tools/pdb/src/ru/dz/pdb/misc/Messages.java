package ru.dz.pdb.misc;

import java.util.MissingResourceException;
import java.util.ResourceBundle;

public class Messages {
	private static final String BUNDLE_NAME = "ru.dz.pdb.misc.messages"; //$NON-NLS-1$

	private static final ResourceBundle RESOURCE_BUNDLE = Utf8ResourceBundle
			.getBundle(BUNDLE_NAME);

	private Messages() {
	}

	public static String getString(String key) {
		try {
			return RESOURCE_BUNDLE.getString(key);
		} catch (MissingResourceException e) {
			return '!' + key + '!';
		}
	}
}
