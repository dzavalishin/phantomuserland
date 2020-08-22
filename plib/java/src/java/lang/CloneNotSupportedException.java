
package java.lang;

public class CloneNotSupportedException extends Exception
{
  /**
   * Compatible with JDK 1.0+.
   */
  private static final long serialVersionUID = 6700697376100628473L;

  /**
   * Create an exception without a message.
   */
  public CloneNotSupportedException()
  {
  }

  /**
   * Create an exception with a message.
   *
   *
   * @param s the message
   */
  public CloneNotSupportedException(String s)
  {
    super(s);
  }
}
