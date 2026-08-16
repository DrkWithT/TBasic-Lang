"""
    bb_sort.py
    General microbench for loops, logical comparisons, indexable object accesses.
    DrkWithT
"""

def makeNums(n):
    temp = []
    i = n

    while i > 0:
        temp.append(i)
        i = i - 1

    return temp

def bbSort(items, count):
    i = 0
    j = 0
    end = count - 1
    a = None
    b = None

    if end < 1:
        return items

    while i < end:
        j = 0

        while j < end:
            if items[j] > items[j + 1]:
                a = items[j]
                b = items[j + 1]

                items[j] = b
                items[j + 1] = a

            j = j + 1

        i = i + 1
    
    return items

count = 40
sample = makeNums(count)

print(bbSort(sample, count))
