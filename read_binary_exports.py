import struct
import sys

# Ask the user to enter the path to the pickle file
if not sys.argv[1:]:
    filepath = input("Enter path to binary file: ")
else:
    filepath = sys.argv[1]


def read_from_binary_file(path):
    with open(path, "rb") as f:
        # repeat until the end of the file
        i = 0
        while i < 100:
            n_bytes = f.read(4)
            if not n_bytes:
                break
            n = struct.unpack("i", n_bytes)[0]
            m_bytes = f.read(4)
            m = struct.unpack("i", m_bytes)[0]

            left_bidomain_bytes = f.read(n * 4)
            vertex_scores_bytes = f.read(m * 4)
            left_bidomain = struct.unpack(f"{n}i", left_bidomain_bytes)
            vertex_scores = struct.unpack(f"{m}i", vertex_scores_bytes)
            print(n, m)
            i += 1


read_from_binary_file(filepath)
