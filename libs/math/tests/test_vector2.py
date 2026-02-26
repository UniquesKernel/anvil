from math_module import Vector2
from hypothesis import strategies as st, given, assume
import math
import numpy as np

"""
C++ uses 32 bit floating point values, and graphics. For graphics rendering
a value range [-1e10, 1e10], seems sufficient for any usuful graphics
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
vector2_strategy = st.builds(
    Vector2,
    float32_strategy,
    float32_strategy
)

TOLERANCE = 0.01


class TestVector2:
    # ============================================================================
    # Equality Tolerance Properties
    # ============================================================================

    unit_float_strategy = st.floats(
        min_value=-1.0,
        max_value=1.0,
        allow_nan=False,
        allow_infinity=False,
        width=32,
    )

    unit_vector2_strategy = st.builds(
        Vector2,
        unit_float_strategy,
        unit_float_strategy,
    )

    @given(a=unit_vector2_strategy,
           axis=st.sampled_from(["x", "y"]),
           sign=st.sampled_from([-1.0, 1.0]))
    def test_equality_inside_tolerance(self, a, axis, sign):
        """Vectors explicitly inside tolerance compare equal."""
        delta = sign * 1e-6
        if axis == "x":
            b = Vector2(a.x + delta, a.y)
        else:
            b = Vector2(a.x, a.y + delta)
        assert a == b

    @given(a=unit_vector2_strategy,
           axis=st.sampled_from(["x", "y"]),
           sign=st.sampled_from([-1.0, 1.0]))
    def test_equality_outside_tolerance(self, a, axis, sign):
        """Vectors explicitly outside tolerance compare not equal."""
        delta = sign * 1e-3
        if axis == "x":
            b = Vector2(a.x + delta, a.y)
        else:
            b = Vector2(a.x, a.y + delta)
        assert a != b

    # ============================================================================
    # Vector Addition Properties
    # ============================================================================
    @given(a=vector2_strategy)
    def test_vector_identity(self, a):
        """Vector2 has an additive identity (0,0)"""
        assert (a + Vector2(0, 0)) == a

    @given(a=vector2_strategy, b=vector2_strategy)
    def test_commutative_add(self, a, b):
        """Vector addition is commutative: a + b == b + a"""
        # Avoid catastrophic cancellation when a and b have opposite signs
        assume(not (math.isclose(a.x, -b.x, rel_tol=TOLERANCE, abs_tol=TOLERANCE) and 
                    math.isclose(a.y, -b.y, rel_tol=TOLERANCE, abs_tol=TOLERANCE)))
        assert a + b == b + a

    @given(a=vector2_strategy)
    def test_additive_inverse(self, a):
        """Every vector has an additive inverse: a + (-a) = 0"""
        assert a + (-a) == Vector2(0, 0)

    def test_vector2_not_associative(self):
        """
        Vector addition is not associative due to
        floating-point precision
        """
        a = Vector2(1.0, 0.0)
        b = Vector2(1e10, 0.0)
        c = Vector2(-1e10, 0.0)

        left = (a + b) + c
        right = a + (b + c)

        assert left != right

    # ============================================================================
    # Vector Subtraction Properties
    # ============================================================================

    @given(a=vector2_strategy)
    def test_subtraction_identity(self, a):
        """Subtracting a vector from itself gives zero vector"""
        assert a - a == Vector2(0, 0)

    @given(a=vector2_strategy, b=vector2_strategy)
    def test_subtraction_as_addition(self, a, b):
        """Subtraction equals addition of negation"""
        # Avoid catastrophic cancellation when a ≈ b
        assume(not (math.isclose(a.x, b.x, rel_tol=TOLERANCE, abs_tol=TOLERANCE) and 
                    math.isclose(a.y, b.y, rel_tol=TOLERANCE, abs_tol=TOLERANCE)))
        assert a - b == a + (-b)

    # ============================================================================
    # Scalar Multiplication Properties
    # ============================================================================

    @given(vector2_strategy)
    def test_multiplicative_identity(self, a):
        """Scalar multiplication has identity element 1"""
        assert 1.0 * a == a

    @given(a=vector2_strategy)
    def test_zero_scalar(self, a):
        """Multiplication by zero scalar gives zero vector"""
        assert 0.0 * a == Vector2(0, 0)

    @given(a=vector2_strategy, scalar=float32_strategy)
    def test_commutative_scalar(self, a, scalar):
        """Scalar multiplication is commutative: c * a == a * c"""
        assert scalar * a == a * scalar

    @given(vector2_strategy, float32_strategy, float32_strategy)
    def test_associative_scalar(self, a, b, c):
        """Scalar multiplication is associative: b * (c * a) == (b * c) * a"""
        assert b * (c * a) == (b * c) * a

    @given(a=vector2_strategy)
    def test_double_negation(self, a):
        """Double negation returns original vector"""
        assert -(-a) == a

    # ============================================================================
    # Distributive Properties
    # ============================================================================

    @given(vector2_strategy, vector2_strategy, float32_strategy)
    def test_distributive_addition(self, a, b, c):
        """Scalar distributes over vector addition: c * (a + b) == c*a + c*b"""
        # Avoid catastrophic cancellation in the sum
        assume(not (math.isclose(a.x, -b.x, rel_tol=TOLERANCE, abs_tol=TOLERANCE) and 
                    math.isclose(a.y, -b.y, rel_tol=TOLERANCE, abs_tol=TOLERANCE)))
        assert c * (a + b) == (c * a) + (c * b)

    @given(vector2_strategy, float32_strategy, float32_strategy)
    def test_distributive_scalar(self, a, b, c):
        """Scalar addition distributes over vector: (b + c) * a == b*a + c*a"""
        # Avoid catastrophic cancellation when b + c ≈ 0
        assume(not math.isclose(b, -c, rel_tol=TOLERANCE, abs_tol=TOLERANCE))
        assert (b + c) * a == (b * a) + (c * a)

    # ============================================================================
    # Dot Product Properties
    # ============================================================================

    @given(a=vector2_strategy, b=vector2_strategy, c=vector2_strategy)
    def test_dot_distributive(self, a, b, c):
        """Dot product is distributive ver addition"""
        # Avoid catastrophic cancellation in vector addition
        assume(not (
            abs(a.x + b.x) < TOLERANCE * max(abs(a.x), abs(b.x), 1.0) and
            abs(a.y + b.y) < TOLERANCE * max(abs(a.y), abs(b.y), 1.0) 
        ))
        dot_ab = a.x * b.x + a.y * b.y
        dot_ac = a.x * c.x + a.y * c.y
        # Avoid catastrophic cancellation in dot product terms
        assume(not math.isclose(dot_ab, -dot_ac, rel_tol=TOLERANCE, abs_tol=TOLERANCE))
        assert math.isclose(a * (b + c),
                            (a * b) + (a * c),
                            rel_tol=TOLERANCE,
                            abs_tol=TOLERANCE)

    @given(a=vector2_strategy, b=vector2_strategy, scalar=float32_strategy)
    def test_dot_scalar_associative(self, a, b, scalar):
        """Dot product with scalar: (s * a) * b = s* (a * b)"""
        # Avoid catastrophic cancellation in dot product components
        term1 = a.x * b.x
        term2 = a.y * b.y
        assume(not math.isclose(term1, -term2, rel_tol=TOLERANCE, abs_tol=TOLERANCE))
        assert math.isclose((scalar * a) * b,
                            scalar * (a * b),
                            rel_tol=TOLERANCE,
                            abs_tol=TOLERANCE)

    @given(a=vector2_strategy, b=vector2_strategy)
    def test_dot_commutative(self, a, b):
        """Dot product is commutative: a * b = b * a"""
        assert math.isclose(a * b, b * a, rel_tol=TOLERANCE, abs_tol=TOLERANCE)

    angle_strategy = st.floats(min_value=-math.pi, max_value=math.pi, allow_nan=False, allow_infinity=False)
    magnitude_strategy = st.floats(min_value=0.1, max_value=1e5, allow_nan=False, allow_infinity=False)

    @given(r1=magnitude_strategy, r2=magnitude_strategy, t1=angle_strategy, t2=angle_strategy)
    def test_dot_geometric(self, r1, r2, t1, t2):
        """Dot product equals |a||b|cos(θ) where θ is the angle between them"""
        a = Vector2(r1 * math.cos(t1), r1 * math.sin(t1))
        b = Vector2(r2 * math.cos(t2), r2 * math.sin(t2))
        expected = r1 * r2 * math.cos(t1 - t2)
        assert math.isclose(a * b, expected, rel_tol=TOLERANCE, abs_tol=TOLERANCE)
