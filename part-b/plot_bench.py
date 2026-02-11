import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

# 데이터 로드 (9개 컬럼)
cols = ['N', 'list_ins', 'hash_ins', 'rb_ins', 'xa_ins', 'list_lkp', 'hash_lkp', 'rb_lkp', 'xa_lkp']
data = pd.read_csv('bench_data.txt', sep='\s+', comment='#', names=cols)

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# (a) Insert Performance
ax1.plot(data['N'], data['list_ins'], 'o-', color='maroon', label='Linked List')
ax1.plot(data['N'], data['hash_ins'], 's-', color='orange', label='Hash Table')
ax1.plot(data['N'], data['rb_ins'], '^-', color='cadetblue', label='RB-Tree')
ax1.plot(data['N'], data['xa_ins'], 'd-', color='gray', label='XArray')

ax1.set_xscale('log')
ax1.set_yscale('log')
ax1.set_xlabel('Number of Entries (N)')
ax1.set_ylabel('Time per Operation (ns/op)')
ax1.set_title('(a) Insert Performance')
ax1.grid(True, which="both", ls="-", alpha=0.2)
ax1.legend()

# (b) Lookup Performance
ax2.plot(data['N'], data['list_lkp'], 'o-', color='maroon', label='Linked List')
ax2.plot(data['N'], data['hash_lkp'], 's-', color='orange', label='Hash Table')
ax2.plot(data['N'], data['rb_lkp'], '^-', color='cadetblue', label='RB-Tree')
ax2.plot(data['N'], data['xa_lkp'], 'd-', color='gray', label='XArray')

ax2.set_xscale('log')
ax2.set_yscale('log')
ax2.set_xlabel('Number of Entries (N)')
ax2.set_ylabel('Time per Operation (ns/op)')
ax2.set_title('(b) Lookup Performance')
ax2.grid(True, which="both", ls="-", alpha=0.2)
ax2.legend()

plt.tight_layout()
plt.savefig('bench_results.pdf')
print("Graph saved as bench_results.pdf")
