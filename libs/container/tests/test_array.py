from hypothesis.stateful import RuleBasedStateMachine, rule
from hypothesis import strategies as st


class ArrayModel(RuleBasedStateMachine):
    def __init__(self):
        super().__init__()

    @rule(num=st.integers())
    def test(self, num):
        assert num == num


TestArrayModel = ArrayModel.TestCase
