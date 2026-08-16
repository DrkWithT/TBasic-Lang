"""
    dict_bench.py
    Microbench for object accesses.
    DrkWithT
"""

data = {
    "i": 5000,
    "a": 0
}

while data["i"] > 0:
    data["a"] = data["a"] + data["i"]
    data["i"] = data["i"] - 1

assert data["a"] == 12502500
