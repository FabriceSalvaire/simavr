//!// Means a comment on comment 

//!// cf. http://www.stack.nl/~dimitri/doxygen/manual/docblocks.html

//!// http://www.stack.nl/~dimitri/doxygen/manual/commands.html
//!// All commands in the documentation start with a backslash (\) or an at-sign (@)


/**
 * ... text ... JavaDoc style
 */

/*!
 ... text ... Qt style
*/

///
/// ... text ...
///

//!
//!... text ...
//!

/*! \brief Brief description.
 *         Brief description continued.
 *
 *  Detailed description starts here.
 */

//!// If JAVADOC_AUTOBRIEF is set to YES

/** Brief description which ends at this dot. Details follow
 *  here.
 */

/// Brief description which ends at this dot. Details follow
/// here.



int var; /*!< Detailed description after the member */
int var; /**< Detailed description after the member */
int var; //!< Detailed description after the member
         //!< 
int var; ///< Detailed description after the member
         ///< 



/*! A test class */
class Test
{
public:
  /** An enum type. 
   *  The documentation block cannot be put after the enum! 
   */
  enum EnumType
    {
      int EVal1,     /**< enum value 1 */
      int EVal2      /**< enum value 2 */
    };
  void member();   //!< a member function.
  
protected:
  int value;       /*!< an integer value */
};



/**
 *  A test class. A more elaborate class description.
 */
class Test
{
public:
  /** 
   * An enum.
   * More detailed enum description.
   */
  enum TEnum { 
    TVal1, /**< enum value TVal1. */  
    TVal2, /**< enum value TVal2. */  
    TVal3  /**< enum value TVal3. */  
  } 
    *enumPtr, /**< enum pointer. Details. */
    enumVar;  /**< enum variable. Details. */
  
  /**
   * A constructor.
   * A more elaborate description of the constructor.
   */
  Test();
  /**
   * A destructor.
   * A more elaborate description of the destructor.
   */
  ~Test();
  
  /**
   * a normal member taking two arguments and returning an integer value.
   * @param a an integer argument.
   * @param s a constant character pointer.
   * @see Test()
   * @see ~Test()
   * @see testMeToo()
   * @see publicVar()
   * @return The test results
   */
  int testMe(int a,const char *s);
  
  /**
   * A pure virtual member.
   * @see testMe()
   * @param c1 the first argument.
   * @param c2 the second argument.
   */
  virtual void testMeToo(char c1,char c2) = 0;
  
  /** 
   * a public variable.
   * Details.
   */
  int publicVar;
  
  /**
   * a function variable.
   * Details.
   */
  int (*handler)(int a,int b);
};
