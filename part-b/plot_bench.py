import matplotlib.pyplot as plt
import pandas as pd

data = pd.read_csv('bench_data.txt', sep='\s+', comment='#', 
                   names=['N', 'l_i', 'h_i', 'r_i', 'x_i', 'l_l', 'h_l', 'r_l', 'x_l'])

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

# Insert Plot
ax1.plot(data['N'], data['l_i'], label='List', marker='o')
ax1.plot(data['N'], data['h_i'], label='Hash', marker='s')
ax1.plot(data['N'], data['r_i'], label='RB-Tree', marker='^')
ax1.plot(data['N'], data['x_i'], label='XArray', marker='d')
ax1.set_xscale('log'); ax1.set_yscale('log')
ax1.set_title('Insert Performance'); ax1.legend()

# Lookup Plot
ax2.plot(data['N'], data['l_l'], label='List', marker='o')
ax2.plot(data['N'], data['h_l'], label='Hash', marker='s')
ax2.plot(data['N'], data['r_l'], label='RB-Tree', marker='^')
ax2.plot(data['N'], data['x_l'], label='XArray', marker='d')
ax2.set_xscale('log'); ax2.set_yscale('log')
ax2.set_title('Lookup Performance'); ax2.legend()

plt.tight_layout()
plt.savefig('bench_results.pdf')
