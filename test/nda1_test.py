import unittest
import numpy as np
import pickle

from nda1 import Dummy, ff,modif1

class TestStorable(unittest.TestCase):

    def test_ff(self):
        a = np.array([1.2, 2.3, 4.4])
        self.assertAlmostEqual(ff(a), 1.2, 1.e-13)
 
    def test_1(self):
        def ff(a): 
           print(a)
           return 8.3
        d = Dummy(f=ff)
        a = np.array([1.2, 2.3, 4.4])
        #d.f(a)
        self.assertAlmostEqual(d.f(a), 8.3, 1.e-13)

    def test_modif1(self):
        a = np.array([1.2, 2.3, 4.4])
        b = 2*a
        modif1(a)
        self.assertTrue(np.allclose(a, b, 1.e-10))

    # def test_modif2(self):
    #     a = np.array([1.2, 2.3, 4.4])
    #     b = 2*a
    #     modif2(a)
    #     self.assertAlmostEqual(ff(a), b, 1.e-13)
 
if __name__ == '__main__':
    unittest.main()
