.. _compiling_examples:

Compiling the examples
**********************

All the examples in the documentation can be compiled using CMake.

General instructions
====================

Each example directory contains a ``CMakeLists.txt`` file. To compile the examples:

.. code-block:: bash

   $ cd <example_directory>
   $ mkdir build
   $ cd build
   $ cmake .. -DUpdate_Python_Bindings=ON
   $ make -j 8

The ``-DUpdate_Python_Bindings=ON`` flag tells CMake to regenerate the Python bindings using ``clair-c2py``.


Code annotations examples
=========================

The code annotations examples are located in ``doc/examples/code_annotations/``.

CMake configuration:

.. literalinclude:: ../examples/code_annotations/CMakeLists.txt
   :language: cmake


TOML configuration examples
============================

The TOML configuration examples are located in ``doc/examples/toml/``.

CMake configuration:

.. literalinclude:: ../examples/toml/CMakeLists.txt
   :language: cmake


Function examples
=================

The function examples are located in ``doc/examples/functions/``.

CMake configuration:

.. literalinclude:: ../examples/functions/CMakeLists.txt
   :language: cmake


Class examples
==============

The class examples are located in ``doc/examples/classes/``.

CMake configuration:

.. literalinclude:: ../examples/classes/CMakeLists.txt
   :language: cmake


Function template examples
===========================

The function template examples are located in ``doc/examples/function_templates/``.

CMake configuration:

.. literalinclude:: ../examples/function_templates/CMakeLists.txt
   :language: cmake


Class template examples
========================

The class template examples are located in ``doc/examples/class_templates/``.

CMake configuration:

.. literalinclude:: ../examples/class_templates/CMakeLists.txt
   :language: cmake
