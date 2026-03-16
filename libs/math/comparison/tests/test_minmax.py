from anvil_math.math_comparison import max_int, min_int
from hypothesis import given, strategies as st

MAX_UINT64 = 2**64 - 1


@st.composite
def two_integers(draw):
    a = draw(st.integers(min_value=0, max_value=MAX_UINT64))
    b = draw(st.integers(min_value=0, max_value=MAX_UINT64))
    return (a, b)


@given(pair=two_integers())
def test_min_int_returns_lowest(pair):
    a, b = pair
    result = min_int(a, b)
    assert result <= a
    assert result <= b
    assert result == a or result == b


@given(pair=two_integers())
def test_max_int_returns_highest(pair):
    a, b = pair
    result = max_int(a, b)
    assert result >= a
    assert result >= b
    assert result == a or result == b
