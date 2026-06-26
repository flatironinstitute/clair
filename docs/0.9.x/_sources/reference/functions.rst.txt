.. _fntref:

Functions
*********


Default Behavior & Customization
--------------------------------

1. Every function defined in the C++ source files is wrapped, subject to the customization options (see :ref:`options <customize>`).

2. Only functions whose arguments and return types are **convertible types** (see :ref:`the list of convertible types <converters>`) can be wrapped.

3. If a function cannot be wrapped because its argument or return type is not convertible, 
   ``clair-c2py`` will generate a Clang **compile-time error** with a diagnostic. 
   For example, the following C++ code:

   .. literalinclude:: ../examples/fail.cpp
      :language: cpp
      :caption: fail.cpp

   will trigger the following compilation error:

   .. raw:: html

      <style type="text/css">
      .ansi2html-content { background: #1c1c1c; color: #cccccc; font-family: monospace; padding: 1em; border-radius: 4px; font-size: 70%; }
      .ansi2html-red { color: #e06c75; }
      .ansi2html-green { color: #529b1eff; }
      .ansi2html-bold { font-weight: bold; }
      </style>
      <pre class="ansi2html-content">
      fail.cpp:3:1: <span class="ansi2html-red">error</span>: c2py: Cannot convert a raw C++ pointer to Python
      3 | double *make_raw_pointer(long size) { return new double[size]; }
         | <span class="ansi2html-green ansi2html-bold">^</span>
      fail.cpp:5:8: <span class="ansi2html-red">error</span>: c2py: Cannot convert this argument from Python to C++
      5 | void f(FILE *x) {}
         |        <span class="ansi2html-green ansi2html-bold">^</span>
      </pre>

To fix such errors, you can exclude the problematic function using the customization options (see :ref:`customize`).



.. _function_templates:

Template Functions
------------------

For generic (template) functions, ``clair-c2py`` only generates bindings for their **explicit instantiations**, subject to the same customization rules as regular functions.

In the following example, we define a function template ``add`` that adds two values of the same type and explicitly instantiate it for three simple types. Dynamic dispatch in Python works exactly as it does for regular functions.


.. literalinclude:: ../examples/function_template.cpp
   :language: cpp
   :caption: function_template.cpp
   :end-before: #include "function_template.wrap.cxx"

.. testsetup::

   import sys
   import os
   # Add the examples build directory to Python path
   # This assumes doctest is run from the build/doc directory
   sys.path.insert(0, os.path.abspath('../examples'))

.. doctest::

    >>> from function_template import add
    >>> add(1, 2)
    3
    >>> add(2.0, 5.0)
    7.0
    >>> add("Hello, ", "world!")
    'Hello, world!'
    >>> add(1, 2.0)
    3.0


Coroutines, generators
----------------------


* Coroutines are supported and can be wrapped, including generators.

