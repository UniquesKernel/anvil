from math_module import Vector3, cross_product
from hypothesis import strategies as st, given, assume
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

TOLERANCE = 0.01


class TestVector3:
    @given(a=vector3_strategy)
    def test_vector_identity(self, a):
        assert (a + Vector3(0, 0, 0)) == a

    @given(a=vector3_strategy, b=vector3_strategy)
    def test_commutative_add(self, a, b):
        assume(not (math.isclose(a.x, -b.x, rel_tol=TOLERANCE, abs_tol=TOLERANCE) and
                    math.isclose(a.y, -b.y, rel_tol=TOLERANCE, abs_tol=TOLERANCE) and
                    math.isclose(a.z, -b.z, rel_tol=TOLERANCE, abs_tol=TOLERANCE)))
        assert a + b == b + a

    @given(a=vector3_strategy)
    def test_additive_inverse(self, a):
        assert a + (-a) == Vector3(0, 0, 0)

    def test_vector3_not_associative(self):
        a = Vector3(1.0, 0.0, 0.0)
        b = Vector3(1e10, 0.0, 0.0)
        c = Vector3(-1e10, 0.0, 0.0)

        left = (a + b) + c
        right = a + (b + c)

        assert left != right

    @given(a=vector3_strategy)
    def test_subtraction_identity(self, a):
        assert a - a == Vector3(0, 0, 0)

    @given(a=vector3_strategy, b=vector3_strategy)
    def test_subtraction_as_addition(self, a, b):
        assume(not (math.isclose(a.x, b.x, rel_tol=TOLERANCE, abs_tol=TOLERANCE) and
                    math.isclose(a.y, b.y, rel_tol=TOLERANCE, abs_tol=TOLERANCE) and
                    math.isclose(a.z, b.z, rel_tol=TOLERANCE, abs_tol=TOLERANCE)))
        assert a - b == a + (-b)

    @given(vector3_strategy)
    def test_multiplicative_identity(self, a):
        assert 1.0 * a == a

    @given(a=vector3_strategy)
    def test_zero_scalar(self, a):
        assert 0.0 * a == Vector3(0, 0, 0)

    @given(a=vector3_strategy, scalar=float32_strategy)
    def test_commutative_scalar(self, a, scalar):
        assert scalar * a == a * scalar

    @given(vector3_strategy, float32_strategy, float32_strategy)
    def test_associative_scalar(self, a, b, c):
        assert b * (c * a) == (b * c) * a

    @given(a=vector3_strategy)
    def test_double_negation(self, a):
        assert -(-a) == a

    @given(vector3_strategy, vector3_strategy, float32_strategy)
    def test_distributive_addition(self, a, b, c):
        assume(not (
            abs(a.x + b.x) < TOLERANCE * max(abs(a.x), abs(b.x), 1.0) and
            abs(a.y + b.y) < TOLERANCE * max(abs(a.y), abs(b.y), 1.0) and
            abs(a.z + b.z) < TOLERANCE * max(abs(a.z), abs(b.z), 1.0)
        ))
        assert c * (a + b) == (c * a) + (c * b)

    @given(vector3_strategy, float32_strategy, float32_strategy)
    def test_distributive_scalar(self, a, b, c):
        assume(not math.isclose(b, -c, rel_tol=TOLERANCE, abs_tol=TOLERANCE))
        assert (b + c) * a == (b * a) + (c * a)

    @given(a=vector3_strategy, b=vector3_strategy, c=vector3_strategy)
    def test_dot_distributive(self, a, b, c):
        assume(not (math.isclose(b.x, -c.x, rel_tol=TOLERANCE, abs_tol=TOLERANCE) and
                    math.isclose(b.y, -c.y, rel_tol=TOLERANCE, abs_tol=TOLERANCE) and
                    math.isclose(b.z, -c.z, rel_tol=TOLERANCE, abs_tol=TOLERANCE)))
        dot_ac = a.x * c.x + a.y * c.y + a.z * c.z
        dot_ab = a.x * b.x + a.y * b.y + a.z * b.z
        assume(not math.isclose(dot_ab, -dot_ac, rel_tol=TOLERANCE, abs_tol=TOLERANCE))
        assert math.isclose(a * (b + c),
                            (a * b) + (a * c),
                            rel_tol=TOLERANCE,
                            abs_tol=TOLERANCE)

    @given(a=vector3_strategy, b=vector3_strategy, scalar=float32_strategy)
    def test_dot_scalar_associative(self, a, b, scalar):
        term1 = a.x * b.x
        term2 = a.y * b.y
        term3 = a.z * b.z
        assume(not math.isclose(term1 + term2, -term3, rel_tol=TOLERANCE, abs_tol=TOLERANCE))
        assume(not math.isclose(term1 + term3, -term2, rel_tol=TOLERANCE, abs_tol=TOLERANCE))
        assume(not math.isclose(term2 + term3, -term1, rel_tol=TOLERANCE, abs_tol=TOLERANCE))
        assert math.isclose((scalar * a) * b,
                            scalar * (a * b),
                            rel_tol=TOLERANCE,
                            abs_tol=TOLERANCE)

    @given(a=vector3_strategy, b=vector3_strategy)
    def test_dot_commutative(self, a, b):
        assert math.isclose(a * b, b * a, rel_tol=TOLERANCE, abs_tol=TOLERANCE)

    angle_strategy = st.floats(min_value=-math.pi, max_value=math.pi, allow_nan=False, allow_infinity=False)
    magnitude_strategy = st.floats(min_value=0.1, max_value=1e5, allow_nan=False, allow_infinity=False)

    @given(r1=magnitude_strategy, r2=magnitude_strategy,
       t1=angle_strategy, t2=angle_strategy,
       p1=angle_strategy, p2=angle_strategy)
    def test_dot_geometric_3d(self, r1, r2, t1, t2, p1, p2):
        a = Vector3(
            r1 * math.sin(p1) * math.cos(t1),
            r1 * math.sin(p1) * math.sin(t1),
            r1 * math.cos(p1)
        )
        b = Vector3(
            r2 * math.sin(p2) * math.cos(t2),
            r2 * math.sin(p2) * math.sin(t2),
            r2 * math.cos(p2)
        )
        cos_angle = (math.sin(p1) * math.sin(p2) * math.cos(t1 - t2) + math.cos(p1) * math.cos(p2))
        expected = r1 * r2 * cos_angle
        assert math.isclose(a * b, expected, rel_tol=TOLERANCE, abs_tol=TOLERANCE)

    @given(a=vector3_strategy, b=vector3_strategy)
    def test_vector3_cross_product_anticommutative(self, a, b):
        assert cross_product(a, b) == -cross_product(b, a)