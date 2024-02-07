.. highlight:: bash

.. _install_src:

Compiling from source
=====================

.. note:: 

   It is not always necessary to install c2py, 
   as it can be fetched by an application using it as a dependency during its own compilation process, 
   cf CMake section.


clair
-----

#. Download the source code of the latest stable version by cloning the ``c2py`` repository from GitHub::

     $ git clone https://github.com/TRIQS/clair clair.src

#. If you want a particular version, check it out, e.g. (use `git tag` go to see the available version)::

     $ git checkout 1.1.0

#. Call cmake, including any additional custom CMake options, see below::

     $ cmake -B clair.build -S clair.src -DCMAKE_INSTALL_PREFIX=path_to_install_dir 

#. Compile the code, run the tests and install the application (here with 8 cores, change accordingly)::

     $ make -j 8
     $ ctest -j 8
     $ make -j 8 install

c2py
----

To install ``c2py``, follow the same procedure as for ``clair``, replacing ``clair`` by ``c2py``.

.. warning:: 

   ``clair`` and ``c2py`` **must** use the same version.


#. Download the source code of the latest stable version by cloning the ``c2py`` repository from GitHub::

     $ git clone https://github.com/TRIQS/c2py c2py.src

#. If you want a particular version, check it out, e.g. (use `git tag` go to see the available version)::

     $ git checkout 1.1.0

#. Call cmake, including any additional custom CMake options, see below::

     $ cmake -B c2py.build -S c2py.src -DCMAKE_INSTALL_PREFIX=path_to_install_dir 

#. Compile the code, run the tests and install the application (here with 8 cores, change accordingly)::

     $ make -j 8
     $ ctest -j 8
     $ make -j 8 install


Set environment variables
-------------------------

To the proper paths into your current shell environment with::

     $ source path_to_install_dir/share/clair/clairvars.sh
     $ source path_to_install_dir/share/c2py/c2pyvars.sh


Custom CMake options
--------------------

The compilation of ``clair`` can be configured using CMake-options::

    cmake ...  -DOPTION1=value1 -DOPTION2=value2

+-----------------------------------------------------------------+-----------------------------------------------+
| Options                                                         | Syntax                                        |
+=================================================================+===============================================+
| Specify an installation path                                    | -DCMAKE_INSTALL_PREFIX=path_to_clair          |
+-----------------------------------------------------------------+-----------------------------------------------+
| Build the documentation                                         | -DBuild_Documentation=ON                      |
+-----------------------------------------------------------------+-----------------------------------------------+
