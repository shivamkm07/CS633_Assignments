import matplotlib.pyplot as plt
import numpy as np

count_executions = 5
P_list = [16, 36, 49, 64]
N_list = [16, 32, 64, 128, 256, 512, 1024]

fig, ax = plt.subplots(figsize=(30, 10))

count = len(P_list)*len(N_list)*3

Times = []
with open("testing.txt", "r") as file1:
    for line in file1:
        Times.append(float(line.strip('\n')))

plot_data = {}
for p in P_list:
    plot_data[p] = []

for i in range(count_executions):

    start_index = i*count
    end_index = (i+1)*count

    for p in range(len(P_list)):

        start_P = start_index + p*len(N_list)*3

        plot_data[P_list[p]].append([Times[i:i + 3] for i in range(start_P, start_P + 3*len(N_list), 3)])

colours = [['blue', 'lightblue'],['darkgreen','lightgreen'], ['red', 'yellow']]
positions = [[1, 1.5, 2, 2.5, 3, 3.5, 4],[1.1, 1.6, 2.1, 2.6, 3.1, 3.6, 4.1],[1.2, 1.7, 2.2, 2.7, 3.2, 3.7, 4.2]]

def box_plot(data, labels, edge_colours, fill_colours, positions, method):
    bp = ax.boxplot(data, positions = positions ,labels = labels, widths = 0.075, patch_artist=True)    
    ax.set_yscale('log')
    ax.set_ylabel('Execution Time (seconds)')
    ax.set_xlabel('Array Size per Process (n*n)')
    for element in ['boxes', 'whiskers', 'fliers', 'means', 'medians', 'caps']:
        plt.setp(bp[element], color = edge_colours)

    for patch in bp['boxes']:
        patch.set(facecolor = fill_colours) 

    return bp

for p in P_list:
    plot_data[p] = np.array(plot_data[p])

for p in P_list:
    boxes = {}
    for method in range(3):
        P_n_method_data = []
        label = []
        for n in range(len(N_list)):
            P_n_method_data.append(plot_data[p][:, n, method])
            if (method == 1):
                label.append(str(N_list[n])+"*"+str(N_list[n]))
            else:
                label.append('')
        boxes[method] = box_plot(P_n_method_data, label, colours[method][0], colours[method][1], positions[method], method+1)

    ax.legend([boxes[0]["boxes"][0], boxes[1]["boxes"][0], boxes[2]["boxes"][0]], ['Method 1', 'Method 2', 'Method 3'], loc = 'upper right')

    plt.tight_layout()
    ax.set_xlim(0.75, 4.45)
    fig.savefig('plot'+str(p)+'.png', dpi=fig.dpi)
    fig, ax = plt.subplots(figsize=(30, 10))