import math
import os

from matplotlib import pyplot as plt



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

    def skin_boxplot(bp):
        plt.setp(bp["boxes"], color="black")
        plt.setp(bp["fliers"], color="black")
        plt.setp(bp["whiskers"], color="black")
        plt.setp(bp["medians"], color="black", linewidth=3, solid_capstyle="butt")

    size = 10**6
    distributions = ("Shuffled", "Shuffled (16 values)", "All equal", "Ascending", "Descending", "Pipe organ", "Push front", "Push middle")

    algos = ("heapsort", "introsort", "pdqsort")
    if "timsort" in data[size]["Shuffled"]: algos += ("timsort",)

    groupnames = distributions
    groupsize = len(algos)
    groups = [[data[size][distribution][algo] for algo in algos] for distribution in distributions]

    for i, group in enumerate(groups):
        skin_boxplot(plt.boxplot(group, positions=[.75 + (0.5 + groupsize)*i + n for n in range(groupsize)], widths=0.6))

    # Set axes limits and labels.
    plt.margins(0.1)
    plt.xticks([0.25 + (groupsize)/2 + (0.5 + groupsize) * n for n in range(len(groupnames))], groupnames)
    plt.xlim(0, (0.5 + groupsize) * len(groups))
    plt.ylim(0, plt.ylim()[1] * 1.05)
    plt.xlabel("Distribution")
    plt.ylabel("Cycles per element")

    # Turn off ticks for x-axis.
    plt.tick_params(
        axis="x",
        which="both",
        bottom="off",
        top="off",
        labelbottom="on"
    )

    ax = plt.gca()
    for i in range(len(groups)):
        for n in range(groupsize):
            ax.text(0.75 + (0.5 + groupsize) * i + n, plt.ylim()[1] * 0.92, str(n + 1), horizontalalignment="center", size="x-small")

    for i in range(len(groups) - 1):
        x = (1 + i) * (0.5 + groupsize)
        plt.plot((x, x), (0, plt.ylim()[1] * 0.95), "-", color="lightgrey")
    plt.plot((0, plt.xlim()[1]), (plt.ylim()[1] * 0.95, plt.ylim()[1] * 0.95), "-", color="lightgrey")


    plt.title("Sorting $10^{}$elements".format(round(math.log(size, 10))))
    plt.suptitle("     ".join("({}) {}".format(n + 1, algo) for n, algo in enumerate(algos)), y=.885)

    figure = plt.gcf() # get current figure
    figure.set_size_inches(19.2, 10.8)
    plt.savefig(os.path.join("plots", os.path.splitext(filename)[0] + ".png"), dpi = 100)

    plt.clf()
