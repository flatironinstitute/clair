.. _overview:

Overview of Clair
===================

**clair-c2py** is a C++ to Python binding generator. 
Given a C++ source file, it automatically generates the necessary C++/Python bindings 
to expose the C++ code as a Python module. 


It is composed of the following main components:

* **c2py**: A library and command-line tool to generate Python bindings from C++ code.
* **clair-c2py**: A command-line tool that integrates ``c2py`` into CMake build systems, automating the generation of bindings during the build process.

The following sections provide an overview of how to use Clair in your projects, including a simple example of generating bindings manually and integrating Clair into a CMake-based build system.

.. toctree::
   :maxdepth: 2

   gettingstarted
   cmake