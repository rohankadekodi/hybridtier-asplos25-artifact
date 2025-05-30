import pandas as pd
import matplotlib.pyplot as plt
import io

file_path = 'result.csv'

with open(file_path, 'r') as file:
    data_string = file.read()

# Split the data into individual tables
tables_raw = data_string.strip().split('\n\n')

# Iterate through each table and generate a plot
for table_raw in tables_raw:
    # Read each table into a DataFrame
    table_lines = table_raw.split('\n')
    # The actual CSV data starts from the second line (index 1)
    csv_data = '\n'.join(table_lines[1:])
    table_io = io.StringIO(csv_data)
    df = pd.read_csv(table_io, sep=',')

    # Clean up column names by stripping whitespace
    df.columns = df.columns.str.strip()

    # Extract title and y-axis label
    title_line = table_lines[0]
    title_parts = title_line.split(' - ')
    title = title_parts[0]
    ylabel = title_parts[1]

    # Select relevant columns for plotting
    df_plot = df[['memory config', 'hybridtier', 'memtis']]

    # Melt the DataFrame to long format for easier plotting with seaborn
    df_melted = df_plot.melt(id_vars='memory config', var_name='Metric', value_name='Value')

    # Create the bar plot
    fig, ax = plt.subplots(figsize=(8, 6))

    # Plotting using grouped bar plots
    bar_width = 0.35
    index = df_melted['memory config'].unique()
    r1 = range(len(index))
    r2 = [x + bar_width for x in r1]

    bars1 = ax.bar(r1, df_melted[df_melted['Metric'] == 'hybridtier']['Value'], width=bar_width, label='hybridtier')
    bars2 = ax.bar(r2, df_melted[df_melted['Metric'] == 'memtis']['Value'], width=bar_width, label='memtis')

    ax.set_xlabel('Memory Config')
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.set_xticks([r + bar_width/2 for r in range(len(index))])
    ax.set_xticklabels(index)
    ax.legend()
    plt.tight_layout()

    filename = title.replace(' ', '_') + '.png'
    plt.savefig("figs/"+filename)
    plt.close(fig)
    #plt.show()
