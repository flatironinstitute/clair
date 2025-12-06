.. _fntref:

Functions
*********


Default behavior & customization
--------------------------------    

* All functions defined in the C++ source files are wrapped, subject to the customization :ref:`options <customize>`.
  
* Only functions whose arguments and return type are of **convertible types** 
  (:ref:`see the list of convertible types <converters>`) can be wrapped. 
  Other functions will trigger a compilation error if they are not excluded.


Template functions
------------------

For generic (template) functions,
clair-c2py only generates bindings for their **explicit instantiations**, 
subject to the same customization rules as the regular functions.

Example
.......

In this example, we define a function template ``add`` that adds two values of the same type.
and explicitly instantiate it for 3 simple types. The dynamical dispatch in Python, 
is done exactly like for regular functions.


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

* Coroutines are supported and can be wrapped, in particular generators.
  Cf FIXME EXAMPLE.

