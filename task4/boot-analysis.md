## 5.1 Boot Timeline

**GRUB modification:**

- File edited: /etc/default/grub

- Line changed: 

```
GRUB_CMDLINE_LINUX="console=ttyS0,115200 initcall_debug"
```

- Command: sudo update-grub

**5 slowest initcalls:**

| Rank | Initcall Function | Time (usecs) |
|------|------------------------|--------------|
| 1 | pci_subsys_init | 110,000 |
| 2 | acpi_init | 81,000 |
| 3 | inet_init | 31,729 |
| 4 | init_encrypted | 28,536 |
| 5 | serial8250_init | 28,250 |


**Total boot time:**

- First timestamp: [ 0.000000]

- "Freeing unused kernel memory" timestamp: [   18.826950]

- Total: 18..926950 seconds

**Slowest subsystem analysis:**

PCIe subsystem took longest because PCI bus initialization involves probing the physical bus to discover and configure connected hardware devices such as Graphic card.

## 5.2 Boot Parameter Deep Dive

**Q1: loglevel=8 effect**

- Default dmesg lines: 2690

- With loglevel=8: 2709

- Difference: +19 lines

**Q2: loglevel parsing location**

- File: init/main.c

- Line: 265

- Command used: 
`grep -rn 'early_param("loglevel"'`

**Q3: Early parameter parsing function**

- Function: __init parse_early_param

- Location: init/main.c:783

**Q4: early_param() vs __setup()**

| Aspect | early_param() | __setup() |
|--------------|-------------------------|------------------------|
| When parsed | Very Early (Before memory/device init) | Later (During device/driver init) |
| Use case | System-critical options (Console, Memory) | Device drivers, non-critical features |
| Example | loglevel, mem, console | netdev, root, ro |
