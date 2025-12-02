.. _multiple_modules:

Multiple Modules
****************

When developing larger projects, you may need to create multiple C++ Python extension modules that communicate with each other. For example, module B might need to use a class defined and wrapped in module A.

**clair-c2py** generates two files for each wrapped module:

- A ``.wrap.cxx`` file (the implementation of the bindings)
- A ``.wrap.hxx`` file (a small header declaring which classes are wrapped in A)

The ``.wrap.hxx`` file allows other modules to know the type wrapped in module A.
Technically, it just specializes of the
``c2py::is_wrapped<T>`` variable for each type ``T`` wrapped in module A.

Example: Two communicating modules
===================================

Module A: Defining a class
---------------------------

Let's create the first module that wraps a class ``N::A``:

**my_class.hpp**:

.. code-block:: cpp

   namespace N {
     struct A {
       int k = 5;
       int f(int i) { return i + k; }
     };
   }

**module_a.cpp**:

.. code-block:: cpp

   #include <c2py/c2py.hpp>
   #include "./my_class.hpp"

   #include "module_a.wrap.cxx"

clair-c2py generates ``module_a.wrap.cxx``
and  ``module_a.wrap.hxx``.

Module B
---------

Now create a second module that uses
the wrapped class ``A`` from module A:

**module_b.cpp**:

.. code-block:: cpp
   :emphasize-lines: 3

   #include <c2py/c2py.hpp>
   #include "./my_class.hpp"
   #include "./module_a.wrap.hxx"  // declares that N::A is wrapped

   struct B {
     int g(int i, N::A const &a) { return i + a.k; }
   };

   #include "module_b.wrap.cxx"


CMake configuration
-------------------

In your ``CMakeLists.txt``, ensure module A is compiled before module B:

.. code-block:: cmake

   # Add both modules
   # FIXME 
   add_clair_module(module_a)
   add_clair_module(module_b)

   # Module B depends on module A's .wrap.hxx file
   add_dependencies(module_b module_a)

Usage in Python
---------------

Once both modules are compiled, you can use them together:

.. code-block:: python

   import module_a
   import module_b

   # Create objects from both modules
   a = module_a.A()
   b = module_b.B()

   # Use them together - B.g() takes an A object
   result = b.g(2, a)  # Returns 7 (2 + a.k where a.k = 5)




