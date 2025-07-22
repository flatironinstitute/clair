.. highlight:: bash

.. _install_package:

Packages
========

OS X (brew)
-----------

Experimental packages of ``c2py`` and ``clair`` are::

  brew install parcollet/ccq/c2py  parcollet/ccq/clair

They are built from source from the GitHub parcollet/ccq repository.


.. note::

  TO BE UPDATED: the packages with tools are not yet available on the `parcollet/ccq` tap.

Uninstalling
............

In order to uninstall all formulas from the `parcollet/ccq` tap, you can use::

        brew uninstall `brew list --full-name -1|grep ccq`


