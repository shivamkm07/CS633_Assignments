import pandas as pd
import seaborn as sns
import numpy as np
import matplotlib.pyplot as plt


Times = []
with open("collected_data", "r") as file1:
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


#
#counter = 0
#
#bcast_data = pd.DataFrame.from_dict({
#    "D": [],
#    "P": [],
#    "ppn": [],
#    "mode": [],  # 1 --> optimized, 0 --> standard
#    "time": [],
#})
#
#reduce_data = pd.DataFrame.from_dict({
#    "D": [],
#    "P": [],
#    "ppn": [],
#    "mode": [],  # 1 --> optimized, 0 --> standard
#    "time": [],
#})
#
#gather_data = pd.DataFrame.from_dict({
#    "D": [],
#    "P": [],
#    "ppn": [],
#    "mode": [],  # 1 --> optimized, 0 --> standard
#    "time": [],
#})
#
#alltoallv_data = pd.DataFrame.from_dict({
#    "D": [],
#    "P": [],
#    "ppn": [],
#    "mode": [],  # 1 --> optimized, 0 --> standard
#    "time": [],
#})
#
#
#for execution in range(10):
#    for P in [4, 16]:
#        for ppn in [1, 8]:
#            for D in [16, 256, 2048]:
#
#                bcast_data = bcast_data.append({
#                    "D": D, "P": P, "ppn": ppn, "mode": "Default", "time": Times[counter]
#                }, ignore_index=True)
#                counter += 1
#
#                bcast_data = bcast_data.append({
#                    "D": D, "P": P, "ppn": ppn, "mode": "Optimised", "time": Times[counter]
#                }, ignore_index=True)
#                counter += 1
#
#
#                reduce_data = reduce_data.append({
#                    "D": D, "P": P, "ppn": ppn, "mode": "Default", "time": Times[counter]
#                }, ignore_index=True)
#                counter += 1
#
#                reduce_data = reduce_data.append({
#                    "D": D, "P": P, "ppn": ppn, "mode": "Optimised", "time": Times[counter]
#                }, ignore_index=True)
#                counter += 1
#
#
#                gather_data = gather_data.append({
#                    "D": D, "P": P, "ppn": ppn, "mode": "Default", "time": Times[counter]
#                }, ignore_index=True)
#                counter += 1
#
#                gather_data = gather_data.append({
#                    "D": D, "P": P, "ppn": ppn, "mode": "Optimised", "time": Times[counter]
#                }, ignore_index=True)
#                counter += 1
#
#                alltoallv_data = alltoallv_data.append({
#                    "D": D, "P": P, "ppn": ppn, "mode": "Default", "time": Times[counter]
#                }, ignore_index=True)
#                counter += 1
#                
#                alltoallv_data = alltoallv_data.append({
#                    "D": D, "P": P, "ppn": ppn, "mode": "Optimised", "time": Times[counter]
#                }, ignore_index=True)
#                counter += 1
#
#bcast_data["(P, ppn)"] = list(map(lambda x, y: ("(" + x + ", " + y + ")"), map(str, bcast_data["P"]), map(str, bcast_data["ppn"])))
#reduce_data["(P, ppn)"] = list(map(lambda x, y: ("(" + x + ", " + y + ")"), map(str, reduce_data["P"]), map(str, reduce_data["ppn"])))
#gather_data["(P, ppn)"] = list(map(lambda x, y: ("(" + x + ", " + y + ")"), map(str, gather_data["P"]), map(str, gather_data["ppn"])))
#alltoallv_data["(P, ppn)"] = list(map(lambda x, y: ("(" + x + ", " + y + ")"), map(str, alltoallv_data["P"]), map(str, alltoallv_data["ppn"])))
#
#
#ax1 = sns.catplot(x="(P, ppn)", y="time", data=bcast_data, kind="box", col="D", hue="mode", palette=sns.color_palette(["lightblue", "orange"]))
#ax1.fig.subplots_adjust(top=0.875)
#ax1.set_xlabels("#Processes x #ProcessesPerNode (P, ppn)")
#ax1.set_ylabels("Execution Time (seconds)")
#ax1.fig.suptitle('MPICH_Bcast Performance') 
#ax1.savefig('plot_Bcast.jpg')
#
#
#ax2 = sns.catplot(x="(P, ppn)", y="time", data=reduce_data, kind="box", col="D", hue="mode", palette=sns.color_palette(["lightblue", "orange"]))
#ax2.fig.subplots_adjust(top=0.875)
#ax2.set_xlabels("#Processes x #ProcessesPerNode (P, ppn)")
#ax2.set_ylabels("Execution Time (seconds)")
#ax2.fig.suptitle('MPICH_Reduce Performance') 
#ax2.savefig('plot_Reduce.jpg')
#
#
#ax3 = sns.catplot(x="(P, ppn)", y="time", data=gather_data, kind="box", col="D", hue="mode", palette=sns.color_palette(["lightblue", "orange"]))
#ax3.fig.subplots_adjust(top=0.875)
#ax3.set_xlabels("#Processes x #ProcessesPerNode (P, ppn)")
#ax3.set_ylabels("Execution Time (seconds)")
#ax3.fig.suptitle('MPICH_Gather Performance') 
#ax3.savefig('plot_Gather.jpg')
#
#
#ax4 = sns.catplot(x="(P, ppn)", y="time", data=alltoallv_data, kind="box", col="D", hue="mode", palette=sns.color_palette(["lightblue", "orange"]))
#ax4.fig.subplots_adjust(top=0.875)
#ax4.set_xlabels("#Processes x #ProcessesPerNode (P, ppn)")
#ax4.set_ylabels("Execution Time (seconds)")
#ax4.fig.suptitle('MPICH_Alltoallv Performance') 
#ax4.savefig('plot_Alltoallv.jpg')
#
#
#
