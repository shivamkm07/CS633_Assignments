import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


Times = []
with open("time.txt", "r") as file1:
    for line in file1:
        Times.append(float(line.strip('\n')))
print(Times)

x_axis = []
for P in [1,2]:
    for ppn in [1,2,4]:
        x_axis.append("("+str(P)+","+str(ppn)+")")
print(x_axis)

plt.plot(x_axis,Times,color='red',marker='o')
plt.xlabel('(Num_Nodes,PPN)',fontsize=14)
plt.ylabel('Execution Time',fontsize=14)
plt.savefig('plot.jpg')

