unitSize = 1024 * 16
ext = ".txt"
for i in range(5):
    with open(str(i) + ext, 'w') as file:
        file.write("********************************\n")
        file.write(str(i) * (2 ** i) * unitSize)
