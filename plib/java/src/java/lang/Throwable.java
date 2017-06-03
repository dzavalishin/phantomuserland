package java.lang;

// TODO import?

// Placeholder

public class Throwable {


  /**
   * Create an exception without a message. The cause remains uninitialized.
   *
   * @see #initCause(Throwable)
   */
  public Throwable()
  {
  }

  /**
   * Create an exception with a message. The cause remains uninitialized.
   *
   * @param s the message
   * @see #initCause(Throwable)
   */
  public Throwable(String s)
  {
    //super(s);
  }

  /**
   * Create an exception with a message and a cause.
   *
   * @param s the message string
   * @param cause the cause of this error
   * @since 1.4
   */
  public Throwable(String s, Throwable cause)
  {
    //super(s, cause);
  }

  /**
   * Create an exception with a given cause, and a message of
   * <code>cause == null ? null : cause.toString()</code>.
   *
   * @param cause the cause of this exception
   * @since 1.4
   */
  public Throwable(Throwable cause)
  {
    //super(cause);
  }


};
