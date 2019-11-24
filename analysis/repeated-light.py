# %% [markdown]
# # Repeated Light
# This notebook allows to connect to the Arduino to query the current brightness readings.
# They are then plotted for comparison

# %% Imports
import serial
import matplotlib.pyplot as plt
import seaborn as sns
import json
import pandas as pd
import numpy as np
import time
# necessary to not pick up the dark theme
matplotlib.rcParams.update(_VSCode_defaultMatplotlib_Params)

# %% Setup
ser = serial.Serial("COM4", baudrate=115200, timeout=1)
# %% Update method

def update():
    ser.write(b'go\r\n')

    msg = ""
    while True:
        next_part = ser.readline().decode("utf-8")
        if next_part == "":
            break
        else:
            msg += next_part

    json_msg = ""
    brackets = 0
    for c in msg:
        if c == '[':
            brackets += 1
            json_msg += c
        elif c == ']':
            brackets -= 1
            json_msg += c
            if brackets == 0:
                break
        elif brackets != 0:
            json_msg += c

    data = json.loads(json_msg)
    return data

#%% manual
data = update()
data_transposed = np.transpose(data[100:250])
df = pd.DataFrame({
    'D0': data_transposed[0],
    'D1': data_transposed[1],
    'D2': data_transposed[2],
    'D3': data_transposed[3],
    'time': range(0, len(data_transposed[0]))
})
df = pd.melt(df, id_vars=['time'], var_name="diode", value_name="brightness")

plt.figure(figsize=(15, 8))
ax = sns.lineplot(x="time", y="brightness", hue="diode", data=df)
ax.xaxis.set_major_formatter(plt.FuncFormatter(
    lambda value, tick_num: "%0.2f ms" % (value/4)))

#%% auto repeat
while True:
    data = update()
    data_transposed = np.transpose(data[100:250])
    df = pd.DataFrame({
        'D0': data_transposed[0],
        'D1': data_transposed[1],
        'D2': data_transposed[2],
        'D3': data_transposed[3],
        'time': range(0, len(data_transposed[0]))
    })
    df = pd.melt(df, id_vars=['time'], var_name="diode", value_name="brightness")

    plt.figure(figsize=(15, 8))
    ax = sns.lineplot(x="time", y="brightness", hue="diode", data=df)
    ax.xaxis.set_major_formatter(plt.FuncFormatter(
        lambda value, tick_num: "%0.2f ms" % (value/4)))
    plt.show()
    time.sleep(2)

# %% big plot
data_transposed = np.transpose(data[100:])
df = pd.DataFrame({
    'D0': data_transposed[0],
    'D1': data_transposed[1],
    'D2': data_transposed[2],
    'D3': data_transposed[3],
    'time': range(0, len(data_transposed[0]))
})
df = pd.melt(df, id_vars=['time'], var_name="diode", value_name="brightness")

plt.figure(figsize=(15, 8))
ax = sns.lineplot(x="time", y="brightness", hue="diode", data=df)
ax.xaxis.set_major_formatter(plt.FuncFormatter(
    lambda value, tick_num: "%0.1f ms" % (value/4)))
# ax.set(xlabel="time in 0.1ms")

# %% zoom in separate
data_transposed = np.transpose(data[100:170])
df = pd.DataFrame({
    'D0': data_transposed[0],
    'D1': data_transposed[1],
    'D2': data_transposed[2],
    'D3': data_transposed[3],
    'time': range(0, len(data_transposed[0]))
})
df = pd.melt(df, id_vars=['time'], var_name="diode", value_name="brightness")

plt.figure(figsize=(15, 8))
ax = sns.lineplot(x="time", y="brightness", hue="diode", data=df)
ax.xaxis.set_major_formatter(plt.FuncFormatter(
    lambda value, tick_num: "%0.2f ms" % (value/4)))

# %% image for paper
with open("data/2019-11-21-repeated-light.json") as f:
    data = json.load(f)
data_transposed = np.transpose(data[130:290])
df = pd.DataFrame({
    'D0': data_transposed[0],
    'D1': data_transposed[1],
    'D2': data_transposed[2],
    'D3': data_transposed[3],
    'time': range(0, len(data_transposed[0]))
})
df = pd.melt(df, id_vars=['time'],
             var_name="photodiode", value_name="brightness")

fig = plt.figure(figsize=(10, 8))
ax = sns.lineplot(x="time", y="brightness", hue="photodiode", data=df)
ax.xaxis.set_major_formatter(plt.FuncFormatter(
    lambda value, tick_num: "%0.2f ms" % (value/4)))
ax.yaxis.set_major_formatter(plt.FuncFormatter(
    lambda value, tick_num: "%0.1f" % (value/10000)))
ax.spines['right'].set_visible(False)
ax.spines['top'].set_visible(False)

# increase font size
# from https://stackoverflow.com/questions/3899980/how-to-change-the-font-size-on-a-matplotlib-plot
for item in ([ax.title, ax.xaxis.label, ax.yaxis.label] + ax.get_xticklabels() + ax.get_yticklabels()):
    item.set_fontsize(16)
ax.legend(fontsize = 16)

# x tick labels are overlapping: only show every second
# from https://stackoverflow.com/questions/50033189/matplotlib-x-axis-overlap/50034359
for label in ax.get_xaxis().get_ticklabels()[::2]:
    label.set_visible(False)

fig.savefig("out/repeated-light.pdf")

#%% count how many highs there are to lows
data_zero = pd.DataFrame({"brightness": np.transpose(data[50:])[0], "t": np.arange(50, len(data))})
data_zero_min = np.min(data_zero.loc[:, "brightness"])
data_zero.loc[:, "cat"] = 0
data_zero.loc[data_zero.loc[:, "brightness"] > data_zero_min * 1.45, "cat"] = 1
data_zero.loc[data_zero.loc[:, "brightness"] > 7000, "cat"] = 2
print("Min: %d"% data_zero_min)
sns.scatterplot(x="t", y ="brightness", hue="cat", data = data_zero)
sns.lineplot(x="t", y="brightness", color="red", data=data_zero, zorder=-1)

last_cat = 0
counters = [0, 0, 0]
peak_count = []
for i in data_zero.index:
    current_cat = data_zero.loc[i, "cat"]
    if last_cat > 0 and current_cat == 0:
        peak_count.append(counters)
        counters = [0, 0, 0]
    counters[current_cat] += 1
    last_cat = current_cat

print(peak_count)
# %% Cleanup
ser.close()
