import math
import os
import sys

import numpy
from matplotlib import pyplot as plt

cpu_info = sys.argv[1]



distribution_names = {
    "shuffled_16_values_int": "Shuffled (16 values)",
    "shuffled_int": "Shuffled",
    "all_equal_int": "All equal",
    "ascending_int": "Ascending",
    "descending_int": "Descending",
    "pipe_organ_int": "Pipe organ",
    "push_front_int": "Push front",
    "push_middle_int": "Push middle"
}

sort_order = ["pdqsort", "std::sort", "std::stable_sort", "timsort", "std::sort_heap"]

for filename in os.listdir("profiles"):
    data = {}
    for line in open(os.path.join("profiles", filename)):
        size, distribution, algo, *results = line.split()
        size = int(size)
        distribution = distribution_names[distribution]
        results = [int(result) for result in results]
        if not size in data: data[size] = {}
        if not distribution in data[size]: data[size][distribution] = {}
        data[size][distribution][algo] = results

    for size in data:
        distributions = ("Shuffled", "Shuffled (16 values)", "All equal", "Ascending", "Descending",
                        "Pipe organ", "Push front", "Push middle")

        algos = tuple(data[size]["Shuffled"].keys())
        algos = tuple(sorted(algos, key=lambda a: sort_order.index(a) if a in sort_order else 1000))

        groupnames = distributions
        groupsize = len(algos)
        groups = [[data[size][distribution][algo] for algo in algos] for distribution in distributions]
        barwidth = 0.6
        spacing = 1
        groupwidth = groupsize * barwidth + spacing

        colors = ["#1f77b4", "#aec7e8", "#ff7f0e", "#ffbb78", "#800080"]
        for i, algo in enumerate(algos):
            heights = [numpy.median(data[size][distribution][algo]) for distribution in distributions]
            errors = [numpy.std(data[size][distribution][algo]) for distribution in distributions]
            plt.barh([barwidth*i + groupwidth*n for n in range(len(distributions))],
                    heights, 0.6, color = colors[i], label = algo)

        # Set axes limits and labels.
        plt.yticks([barwidth * groupsize/2 + groupwidth*n for n in range(len(groupnames))], groupnames)
        plt.xlabel("Cycles per element ({})".format(cpu_info))

        # Turn off ticks for y-axis.
        plt.tick_params(
            axis="y",
            which="both",
            left="off",
            right="off",
            labelleft="on"
        )

        ax = plt.gca()
        ax.invert_yaxis()
        ax.relim()
        ax.autoscale_view()
        plt.ylim(plt.ylim()[0]+1, plt.ylim()[1]-1)
        plt.legend(loc="right", fontsize=10)

        plt.title("Sorting $10^{}$ elements".format(round(math.log(size, 10))))

        figure = plt.gcf()
        figure.set_size_inches(8*.75, 6*.75)
        plt.savefig(os.path.join("plots", "{}_{}.png".format(os.path.splitext(filename)[0], size)),
                    dpi = 100, bbox_inches="tight")

        plt.clf()
