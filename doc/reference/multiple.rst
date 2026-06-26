.. _multiple_modules:

Across multiple modules
***********************

When developing larger projects, you may need to create multiple C++ Python extension modules that communicate with each other. For example, module B might need to use a class defined and wrapped in module A.

``clair-c2py`` generates up to two files for each wrapped module:

- A ``.wrap.cxx`` file (the implementation of the bindings), always generated.
- A ``.wrap.hxx`` file (a small header declaring which classes are wrapped in A),
  generated only if module A wraps at least one class.

The ``.wrap.hxx`` file allows other modules to know the type wrapped in module A.
Technically, it just specializes of the
``c2py::is_wrapped<T>`` variable for each type ``T`` wrapped in module A.
Modules that wrap only free functions and/or enums do not produce a ``.wrap.hxx``,
since there is no per-class information to share.

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

``clair-c2py`` generates ``module_a.wrap.cxx``
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

In your ``CMakeLists.txt``, build both modules with ``c2py_add_module`` and tell module B that
module A's bindings must be generated first via ``DEPENDS_ON_BINDINGS``:

.. code-block:: cmake

   c2py_add_module(module_a)

   # module_b includes module_a's .wrap.hxx, so module_a's bindings are generated first
   c2py_add_module(module_b DEPENDS_ON_BINDINGS module_a)

The ordering only matters when ``Update_Python_Bindings`` is ``ON``; with checked-in ``.wrap``
files there is no generation step to order.

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




