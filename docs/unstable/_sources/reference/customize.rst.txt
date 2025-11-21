.. _customize:

Customization
*************

Many customization options of the bindings generation are available, 
e.g. wrap only some functions and classes, choose template instantiation, and so on.
This can be done

* with code annotations or
* with a TOML input file.

The choice between the two approaches depends on the project and needs.


Code annotations
----------------

``clair``'s behaviour can be modified by simple annotations in the code.

* ``C2PY_IGNORE``: Placed before a function or a class, ``clair`` ignores it.
* ``C2PY_WRAP_AS_METHOD``: Placed before a function, ``clair`` adds the function as a method to the first argument's class object.
* ``C2PY_MODULE_INIT``: Placed before a function, ``clair`` uses this function as the module initialization function.
* ``C2PY_RENAME(new_name)``: Placed before a function or a class, ``clair`` uses ``new_name`` as the name in Python.

These annotations overrule any filter options in the TOML file.

See :ref:`code_annotations` for examples.

TOML input file
---------------

The TOML file is required to have the same name as the C++ source file, except for the ``.cpp`` extension.
For example, if the C++ source file given to ``clair-c2py`` is ``my_module.cpp``, the TOML file should be called ``my_module.toml``.

Here is an example TOML file showing and explaining the available options:

.. literalinclude:: ../examples/toml/initial.toml
   :language: toml

See :ref:`toml` for how these options affect the generated bindings.

