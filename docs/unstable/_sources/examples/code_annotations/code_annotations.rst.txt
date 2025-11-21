.. _code_annotations:

Code annotations
****************

The following examples demonstrate the use of various code annotations supported by **clair/c2py** to customize the generated Python bindings.

All examples have been compiled with CMake using the instructions from :ref:`compiling_code_annotations`.


C2PY_IGNORE
===========

The ``C2PY_IGNORE`` annotation can be used to exclude specific functions or classes from being exposed in the Python bindings.

.. literalinclude:: ./c2py_ignore.cpp
   :language: cpp

After generating the extension module, we can use it in Python:

.. code-block:: console

    >>> from c2py_ignore import *
    >>> g(2)
    6
    >>> f(2)
    Traceback (most recent call last):
      File "<python-input-2>", line 1, in <module>
        f(2)
        ^
    NameError: name 'f' is not defined

As expected, only the function ``g`` is available in Python, while ``f`` has been ignored.


C2PY_RENAME
===========

The ``C2PY_RENAME`` annotation can be used to give a wrapped function or class a different name in the Python bindings.

.. literalinclude:: ./c2py_rename.cpp
   :language: cpp

After generating the extension module, we can use it in Python:

.. code-block:: console

    >>> from c2py_rename import *
    >>> g(5)
    10
    >>> f(5)
    Traceback (most recent call last):
    File "<python-input-2>", line 1, in <module>
        f(5)
        ^
    NameError: name 'f' is not defined

The function ``f`` has been rename to ``g`` as instructed.


C2PY_MODULE_INIT
================

The ``C2PY_MODULE_INIT`` annotation lets us define a function that will be called when the module is initialized/imported.

.. literalinclude:: ./c2py_module_init.cpp
   :language: cpp

After generating the extension module, we can use it in Python:

.. code-block:: console

    >>> import c2py_module_init
    c2py/clair rocks!

Importing the module triggers the execution of the ``init`` function, which prints a message to the console.


C2PY_WRAP_AS_METHOD
===================

The ``C2PY_WRAP_AS_METHOD`` annotation lets us add a function as a method to the first argument's class object.

.. literalinclude:: ./c2py_wrap_as_method.cpp
   :language: cpp

After generating the extension module, we can use it in Python:

.. code-block:: console

    >>> from c2py_wrap_as_method import *
    >>> m = Myclass(x = 100)
    >>> m.f(5)
    500
    >>> f(5)
    Traceback (most recent call last):
    File "<python-input-5>", line 1, in <module>
        f(5)
        ^
    NameError: name 'f' is not defined

The free C++ function ``f`` has been added as a method to the ``Myclass`` class in Python.


.. _compiling_code_annotations:

Compiling the examples
======================

We use the following CMake file:

.. literalinclude:: ./CMakeLists.txt
   :language: cmake

To compile all modules at once, run:

.. code-block:: bash

   $ mkdir build
   $ cd build
   $ cmake .. -DUpdate_Python_Bindings=ON
   $ make -j 8
