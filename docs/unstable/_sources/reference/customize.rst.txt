.. _customize:

Customization
*************

Many customization options of the bindings generation are available, 
e.g. wrap only some functions and classes, choose template instantiation, and so on.
This can be done

* with code annotations or
* with (compile time) declarations in the reserved namespace ``c2py_module``. 

The choice between the two approaches depends on the project and needs. The latter is more general and non intrusive, but slighly more complex.


Code annotations
----------------

``clair``'s behaviour can be modified by simple annotations in the code.

* ``C2PY_IGNORE``: Placed before a function or a class, ``clair`` ignores it.
* ``C2PY_WRAP``: Placed before a function or a class, ``clair`` keeps it.  This can be used in combination with a ``reject_names`` filter ``.*``.

These annotations overrule any filter options in ``c2py_module``.

Filters
-------

By default, ``clair`` will generate bindings for all non-template classes and functions,
except those defined in `system` files, i.e. the standard library and more generally 
anything the compiler includes via `-isystem` options.
In order to refine this behaviour, several filters can be used.

* ``match_names``, ``reject_names``. These **regular expressions** control which functions/classes/enums are exposed to Python.

   The algorithm to determine if an element X of qualified name Xqname (i.e. the full name with the namespaces) should be exposed is:
     
     #. if ``match_names`` is defined and does not match Xqname then skip X
     #. if ``reject_names`` is defined and matches Xqname then skip X
     #. else expose X to Python. 

Here is an example:

.. literalinclude:: ../examples/filter.cpp
   :language: cpp
   
In this example, the function ``f`` and the struct ``a_class`` are exposed to Python, 
while the function ``g``, and the struct ``N::hidden`` are not.

.. warning::

   Be careful that these strings are Regex. For example, ``N::*`` does not match ``N::f``.
   Also note that the qualified name starts with ``::``. So to match the start of a name, use ``^::N::f``. 

Template instantiation. Explicitly wrapping a function
------------------------------------------------------

In addition to the functions automatically detected by clair, 
one can explicitly declare some functions to be wrapped in the ``c2py_module::add`` namespace
using the ``c2py::dispatch`` type.
This is specially useful for **template instantiation**.

.. literalinclude:: ../examples/config_dispatch.cpp
   :language: cpp

Since the function ``f`` is a template, no automatic wrapping will happen by default, 
as the compiler can not know which instantiations to use.
The declaration ``auto f =...`` adds a Python function `f` (the name of the declaration)
which consists in dispatching `f<int>` and `f<double>`.

* ``c2py::dispatch<list of function pointers>``

   Declares a list of C++ functions to be dispatched together

.. note::

     *Any* function pointer can be used in ``c2py::dispatch``, so the same technique can be used
     to regroup different functions into one dispatch, or rename a function.


Class template instantiation
----------------------------

Similar to functions, instantiations of class templates should be declared 
with a `using` command in the ``c2py_module::add`` namespace and a Python name should be provided:

.. literalinclude:: ../examples/config_using.cpp
   :language: cpp

In this example, the Python classes ``Ai`` and ``Ad`` wrap the C++ classes ``A<int>`` and ``A<double>``, respectively.

Modifying classes
-----------------

To be documented.

Misc
----

Additional information can be provided:

.. literalinclude:: ../examples/config_misc.cpp
   :language: cpp
   :linenos:
 
 
.. toctree::
   :maxdepth: 2

   api


