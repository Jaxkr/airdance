import matplotlib.pyplot as plt
plt.style.use('seaborn-whitegrid')
import numpy as np

UP = 5
DOWN = 3
LEFT = 4
RIGHT = 2

left_gravX_points = []
right_gravX_points = []

def createPoints(data, destination, starting_index):
    i = starting_index
    counter = 0
    while i < 180:
        destination.append([counter, float(data[i])])
        i += 6
        counter += 1

def processLine(line):
    left_foot_data = line[:180]
    right_foot_data = line[180:]
    # Change the magic number here to select a different data index to analyze.
    # see the C++ code for documentation on this format.
    createPoints(left_foot_data, left_gravX_points, 4)
    

f = open('../data/sample1.txt', 'r')
line = f.readline()
while line:
    s = line.split(' ')
    direction = int(s.pop(0))
    newline = s.pop()
    if (direction == DOWN):
        processLine(s)

    line = f.readline()


# average the plot
bins = {}
sample_count = 0
for e in left_gravX_points:
    sample_count += 1
    if e[0] in bins:
        bins[e[0]] += e[1]
    else:
        bins[e[0]] = 0

print(bins)
print(sample_count)
for key in bins:
    bins[key] = bins[key] / sample_count
points = []
for key in bins:
    points.append([key, bins[key]])
print(points)

data = np.array(points)
x, y = data.T
plt.scatter(x,y)
plt.show()
