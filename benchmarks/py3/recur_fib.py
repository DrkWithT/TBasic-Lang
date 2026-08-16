"""
    recur_fib.py
    Microbench for call and dispatch.
    By: DrkWithT
"""

def fib(n):
    if n < 2:
        return n

    return fib(n - 1) + fib(n - 2)

assert fib(30) == 832040
