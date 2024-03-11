# f1 = open(r'./data/data.bin', 'rb').read()
# f2 = open(r'./data/data2.bin', 'rb').read()
# f3 = open(r'./data/data3.bin', 'rb').read()

# errors = 0
# for i in range(10000):
#     if(f1[i] == f2[i] and f1[i] != f3[i]):
#         continue

#     print(f"Mismatch at {i}: {f1[i]},{f2[i]},{f3[i]}")
#     errors += 1

# print(f"Total errors {errors}")

with open(r'./data/data.bin', 'rb') as f:
    print(f.read()[-2::])