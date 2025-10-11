import os
from hypothesis import settings

# Optional: avoid errors if var missing
max_examples = int(os.getenv("HYPOTHESIS_MAX_EXAMPLES", "10000"))
settings.register_profile("ci", max_examples=max_examples)
settings.load_profile("ci")