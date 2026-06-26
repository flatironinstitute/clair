.. _clsref:

Classes
*******


Default Behavior & Customization
--------------------------------

#. Every class defined in the C++ source files is wrapped, subject to the customization options (see :ref:`options <customize>`).

#. Only public elements (methods, fields) are exposed to Python.

#. The name of the class in Python is transformed into **CamelCase**, e.g.
   ``my_class`` in C++ becomes ``MyClass`` in Python.
   Use the ``C2PY_RENAME`` option to customize the name (see :ref:`customize`).

Let us now review in some details the default behaviour for various elements of a class.

.. toctree::
   :maxdepth: 1

   constructors
   methods
   operators
   iterable
   string_representation
