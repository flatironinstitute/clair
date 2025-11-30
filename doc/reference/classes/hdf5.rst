.. _class_hdf5:

HDF5 Serialization
******************

Classes with HDF5 serialization support can be saved and loaded from HDF5 files.

.. literalinclude:: ../../examples/classes/hdf5.cpp
   :language: cpp

The Python usage:

.. code-block:: python

   from mymodule import Matrix
   import h5py
   
   m = Matrix(3, 3)
   # Save to HDF5
   with h5py.File('data.h5', 'w') as f:
       m.save_to_hdf5(f, 'matrix')
   
   # Load from HDF5
   with h5py.File('data.h5', 'r') as f:
       m2 = Matrix.load_from_hdf5(f, 'matrix')
