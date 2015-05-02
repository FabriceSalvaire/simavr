// cf. https://www.gnu.org/prep/standards/standards.html#Formatting

/* 
 *  Formatting Your Source Code
 */

// Please keep the length of source lines to - 79 - characters or less, for maximum readability in
// the widest range of environments.

// It is important to put the open-brace that starts the body of a C function in column one, so that
// they will start a defun.  Several tools look for open-braces in column one to find the beginnings
// of C functions.  These tools will not work on code not formatted that way.

// Avoid putting open-brace, open-parenthesis or open-bracket in column one when they are inside a
// function, so that they won’t start a defun.  The open-brace that starts a struct body can go in
// column one if you find it useful to treat that definition as a defun.

// It is also important for function definitions to start the name of the function in column one.
// This helps people to search for function definitions, and may also help certain tools recognize
// them.  Thus, using Standard C syntax, the format is this:

static char *
concat (char *s1, char *s2)   // place function name at column 1
{   // a { at column 1 indicates a function definition
  // ...
}

// In Standard C, if the arguments don’t fit nicely on one line, split it like this:
int
lots_of_args (int an_integer, long a_long, short a_short,
              double a_double, float a_float)
{}

// For struct and enum types, likewise put the braces in column one, unless the whole contents fits
// on one line:
struct foo
{
  int a, b;
};
// or
struct foo { int a, b; };

// For the body of the function, our recommended style looks like this:
void
foo ()
{
  if (x < foo (y, z))
    haha = bar[4] + 5;
  else
    {
      while (z)
        {
          haha += foo (z, z);
          z--;
        }
      return ++x + bar ();
    }
  // We find it easier to read a program when it has spaces before the open-parentheses and after
  // the commas.  Especially after the commas.
  
  // When you split an expression into multiple lines, split it before an operator, not after one.
  // Here is the right way:
  if (foo_this_is_long && bar > win (x, y, z)
      && remaining_condition)
    {}

  // Try to avoid having two operators of different precedence at the same level of indentation.
  // For example, don’t write this:
  mode = (inmode[j] == VOIDmode
          || GET_MODE_SIZE (outmode[j]) > GET_MODE_SIZE (inmode[j])
          ? outmode[j] : inmode[j]);
  // Instead, use extra parentheses so that the indentation shows the nesting:
  mode = ((inmode[j] == VOIDmode
           || (GET_MODE_SIZE (outmode[j]) > GET_MODE_SIZE (inmode[j])))
          ? outmode[j] : inmode[j]);
  
  // Insert extra parentheses so that Emacs will indent the code properly.
  v = (rup->ru_utime.tv_sec*1000 + rup->ru_utime.tv_usec/1000
       + rup->ru_stime.tv_sec*1000 + rup->ru_stime.tv_usec/1000);

  // Format do-while statements like this:
  do
    {
      a = foo (a);
    }
  while (a > 0);
}

/*
 *  Commenting Your Work
 */

// Please put two spaces after the end of a sentence in your comments, so that the Emacs sentence
// commands will work.

#ifndef foo
// ..
#else /* foo */
// ...
#endif /* foo */

#ifndef foo
// ...
#endif /* not foo */

/*
 *  Clean Use of C Constructs
 */

// write either this:
int foo, bar;
// or this:
int foo;
int bar;

// When you have an if-else statement nested in another if statement, always put braces around the
// if-else.
void
foo ()
{
  if (foo)
    {
      if (bar)
        win ();
      else
        lose ();
    }
  
  if (foo)
    win ();
  else if (bar)
    lose ();
  
  if (foo)
    win ();
  else
    {
      if (bar)
        lose ();
    }
}
