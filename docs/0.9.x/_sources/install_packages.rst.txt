.. highlight:: bash

.. _install_package:

Packages
========

Anaconda
--------

We provide Linux and OSX packages through the `conda-forge <https://conda-forge.org/>`_
repositories. After `installing conda <https://docs.conda.io/en/latest/miniconda.html>`_
you can install clair with::

  conda install -c conda-forge clair-c2py

See also `github.com/conda-forge/clair-c2py-feedstock <https://github.com/conda-forge/clair-c2py-feedstock/>`_.

OS X (brew)
-----------

Experimental packages of ``c2py`` and ``clair`` are::

  brew install parcollet/ccq/c2py  parcollet/ccq/clair

They are built from source from the GitHub parcollet/ccq repository.


.. note::

  The packages with tools are not yet available on the `parcollet/ccq` tap.

Uninstalling
............

In order to uninstall all formulas from the `parcollet/ccq` tap, you can use::

        brew uninstall `brew list --full-name -1|grep ccq`


