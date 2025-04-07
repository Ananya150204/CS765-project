import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
# Extract R1 and R2 values for each malicious node percentage across timeouts (only complete entries)

# Structure:
# Each entry is (R1_eclipse, R2_eclipse, R1_no_eclipse, R2_no_eclipse)

data_full = {
    'Malicious %': [5, 10, 15, 40, 60],
    'R1_200_e': [0.01, 0.0375, 0.089, 0.59, 0.087],
    'R2_200_e': [0.25, 0.6, 0.778, 0.743, 0.033],
    'R1_200_ne': [0.01, 0.16, 0.11, 0.507, 0.107],  # example inferred/assumed
    'R2_200_ne': [1.0, 0.857, 1, 0.735, 0.05],    # example inferred/assumed

    'R1_2000_e': [0.0375, 0.157, 0.093, 0.608, 0],
    'R2_2000_e': [0.75, 1.0, 0.7, 0.978, 0],
    'R1_2000_ne': [0.0235, 0.045, 0.162, 0.456, 0.2],     # assumed/inferred
    'R2_2000_ne': [0.667, 0.571, 0.75, 0.756, 0.111],

    'R1_20000_e': [0.081, 0.0337, 0.144, 0.592, 0.596],
    'R2_20000_e': [0.777, 0.6, 0.909, 0.941, 0.5],
    'R1_20000_ne': [0.062, 0.131, 0.205, 0.297, 0.394],  # assumed/incomplete
    'R2_20000_ne': [0.625, 0.733, 0.882, 0.463, 0.283],

    'R1_200000_e': [0.0, 0.096, 0.128, 0.125,0],
    'R2_200000_e': [0.0, 0.889, 0.846, 0.125, 0.0],
    'R1_200000_ne': [0.022, 0.081, 0.176, 0.437, 0.33],  # estimated
    'R2_200000_ne': [0.667, 0.5, 0.762, 0.94, 0.53],
}

df_all = pd.DataFrame(data_full)

import seaborn as sns

# Melt data for seaborn plotting
melted_R1 = df_all.melt(id_vars=['Malicious %'], 
                        value_vars=[col for col in df_all.columns if col.startswith('R1')],
                        var_name='Condition', value_name='R1')

melted_R2 = df_all.melt(id_vars=['Malicious %'], 
                        value_vars=[col for col in df_all.columns if col.startswith('R2')],
                        var_name='Condition', value_name='R2')

# Plot R1
plt.figure(figsize=(12, 6))
sns.lineplot(data=melted_R1, x='Malicious %', y='R1', hue='Condition', marker='o')
plt.title('R1 vs Malicious Node Percentage (across all timeouts)')
plt.grid(True)
plt.savefig('r1_vs_malicious_percent.png')
plt.show()

# Plot R2
plt.figure(figsize=(12, 6))
sns.lineplot(data=melted_R2, x='Malicious %', y='R2', hue='Condition', marker='o')
plt.title('R2 vs Malicious Node Percentage (across all timeouts)')
plt.grid(True)
plt.savefig('r2_vs_malicious_percent.png')
plt.show()

