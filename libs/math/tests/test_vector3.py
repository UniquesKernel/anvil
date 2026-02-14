from math_module import Vector3
from hypothesis import strategies as st, given
import math
import numpy as np

"""
C++ uses 32 bit floating point values, and graphics. For graphics rendering
a value range [-1e10, 1e10], seems sufficient for any useful graphics
rendering.
"""
float32_strategy = st.floats(
    min_value=-1e10,
    max_value=1e10,
    allow_nan=False,
    allow_infinity=False,
    width=32
)

"""
A simple strategy for building vectors, the limited scope floating
point strategy
"""

vector3_strategy = st.builds(
    Vector3,
    float32_strategy,
    float32_strategy,
    float32_strategy
)

TOLERANCE = 1e-4


class TestVector3:
    # ============================================================================
    # Vector Addition Properties
    # ============================================================================
    @given(a=vector3_strategy)
    def test_vector_identity(self, a):
        """Vector3 has an additive identity (0,0,0)"""
        assert (a + Vector3(0, 0, 0)) == a

    @given(a=vector3_strategy, b=vector3_strategy)
    def test_commutative_add(self, a, b):
        """Vector addition is commutative: a + b == b + a"""
        assert a + b == b + a

    @given(a=vector3_strategy)
    def test_additive_inverse(self, a):
        """Every vector has an additive inverse: a + (-a) = 0"""
        assert a + (-a) == Vector3(0, 0, 0)

    def test_vector3_not_associative(self):
        """
        Vector addition is not associative due to
        floating-point precision
        """
        a = Vector3(1.0, 0.0, 0.0)
        b = Vector3(1e10, 0.0, 0.0)
        c = Vector3(-1e10, 0.0, 0.0)

        left = (a + b) + c
        right = a + (b + c)

        assert left != right

    # ============================================================================
    # Vector Subtraction Properties
    # ============================================================================

    @given(a=vector3_strategy)
    def test_subtraction_identity(self, a):
        """Subtracting a vector from itself gives zero vector"""
        assert a - a == Vector3(0, 0, 0)

    @given(a=vector3_strategy, b=vector3_strategy)
    def test_subtraction_as_addition(self, a, b):
        """Subtraction equals addition of negation"""
        assert a - b == a + (-b)

    # ============================================================================
    # Scalar Multiplication Properties
    # ============================================================================

    @given(vector3_strategy)
    def test_multiplicative_identity(self, a):
        """Scalar multiplication has identity element 1"""
        assert 1.0 * a == a

    @given(a=vector3_strategy)
    def test_zero_scalar(self, a):
        """Multiplication by zero scalar gives zero vector"""
        assert 0.0 * a == Vector3(0, 0, 0)

    @given(a=vector3_strategy, scalar=float32_strategy)
    def test_commutative_scalar(self, a, scalar):
        """Scalar multiplication is commutative: c * a == a * c"""
        assert scalar * a == a * scalar

    @given(vector3_strategy, float32_strategy, float32_strategy)
    def test_associative_scalar(self, a, b, c):
        """Scalar multiplication is associative: b * (c * a) == (b * c) * a"""
        assert b * (c * a) == (b * c) * a

    @given(a=vector3_strategy)
    def test_double_negation(self, a):
        """Double negation returns original vector"""
        assert -(-a) == a

    # ============================================================================
    # Distributive Properties
    # ============================================================================

    @given(vector3_strategy, vector3_strategy, float32_strategy)
    def test_distributive_addition(self, a, b, c):
        """Scalar distributes over vector addition: c * (a + b) == c*a + c*b"""
        assert c * (a + b) == (c * a) + (c * b)

    @given(vector3_strategy, float32_strategy, float32_strategy)
    def test_distributive_scalar(self, a, b, c):
        """Scalar addition distributes over vector: (b + c) * a == b*a + c*a"""
        assert (b + c) * a == (b * a) + (c * a)

    # ============================================================================
    # Dot Product Properties
    # ============================================================================

    @given(a=vector3_strategy, b=vector3_strategy, c=vector3_strategy)
    def test_dot_distributive(self, a, b, c):
        """Dot product is distributive over addition"""
        assert math.isclose(a * (b + c),
                            (a * b) + (a * c),
                            rel_tol=TOLERANCE,
                            abs_tol=TOLERANCE)

    @given(a=vector3_strategy, b=vector3_strategy, scalar=float32_strategy)
    def test_dot_scalar_associative(self, a, b, scalar):
        """Dot product with scalar: (s * a) * b = s * (a * b)"""
        assert math.isclose((scalar * a) * b,
                            scalar * (a * b),
                            rel_tol=TOLERANCE,
                            abs_tol=TOLERANCE)

    @given(a=vector3_strategy, b=vector3_strategy)
    def test_dot_commutative(self, a, b):
        """Dot product is commutative: a * b = b * a"""
        assert a * b == b * a

    @given(a=vector3_strategy)
    def test_vector3_magnitude(self, a):
        """Vector magnitude squared equals dot product with itself"""
        assert a * a == math.sqrt(a.x**2 + a.y**2 + a.z**2)**2 * np.float32(math.cos(0))
