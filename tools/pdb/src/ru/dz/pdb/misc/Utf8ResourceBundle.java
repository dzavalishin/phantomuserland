package ru.dz.pdb.misc;

import java.io.UnsupportedEncodingException;
import java.util.Enumeration;
import java.util.Locale;
import java.util.PropertyResourceBundle;
import java.util.ResourceBundle;

public abstract class Utf8ResourceBundle {

	public static final ResourceBundle getBundle(String baseName) {
		ResourceBundle bundle = ResourceBundle.getBundle(baseName);
		return createUtf8PropertyResourceBundle(bundle);
	}

	public static final ResourceBundle getBundle(String baseName, Locale locale) {
		ResourceBundle bundle = ResourceBundle.getBundle(baseName, locale);
		return createUtf8PropertyResourceBundle(bundle);
	}

	public static ResourceBundle getBundle(String baseName, Locale locale, ClassLoader loader) {
		ResourceBundle bundle = ResourceBundle.getBundle(baseName, locale);
		return createUtf8PropertyResourceBundle(bundle);
	}

	private static ResourceBundle createUtf8PropertyResourceBundle(ResourceBundle bundle) {
		if (!(bundle instanceof PropertyResourceBundle)) return bundle;

		return new Utf8PropertyResourceBundle((PropertyResourceBundle)bundle);
	}

	private static class Utf8PropertyResourceBundle extends ResourceBundle {
		PropertyResourceBundle bundle;

		private Utf8PropertyResourceBundle(PropertyResourceBundle bundle) {
			this.bundle = bundle;
		}
		/* (non-Javadoc)
		 * @see java.util.ResourceBundle#getKeys()
		 */
		public Enumeration getKeys() {
			return bundle.getKeys();
		}
		//$ANALYSIS-IGNORE,codereview.java.rules.casting.RuleCastingObjectDowncast
		protected Object handleGetObject(String key){
		  String value=(String)bundle.handleGetObject(key);
		  try {
		    return new String(value.getBytes("ISO-8859-1"),"UTF-8"); //$NON-NLS-1$ //$NON-NLS-2$
		  }
		 catch (  UnsupportedEncodingException e) {
		    return null;
		  }
		}
		

	}
}