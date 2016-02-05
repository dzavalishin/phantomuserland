package ru.dz.plc.compiler;

/**
 * <p>Title: ru.dz.plc.compiler</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2004</p>
 * <p>Company: </p>
 * @author dz
 * @version 1.0
 */

public class PhantomVariable
{
	private String   name;
	private PhantomType  type;
	private boolean _public = false;

	public PhantomVariable( PhantomVariable v )
	{
		this.name = v.name;
		this.type = v.type;
	}

	public PhantomVariable( String name, PhantomType type)
	{
		this.name = name;
		this.type = type;
	}

	public String getName() { return name; }
	public PhantomType  getType() { return type; }

	public void setType(PhantomType type) {
		this.type = type;
	}

	/** generate getter and setter. Java's class-visible is public in Phantom! */
	public boolean isPublic() {
		return _public;
	}

	public void setPublic(boolean _public) {
		this._public = _public;
	}
};
