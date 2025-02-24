# Problems

- "QseeLoadServiceImageSyscall Failed"
- WHen loading cmnlib_a (32-bit lib)

```
(qemu) HIT instrument @8879e3c54 cpu 0 1
qcom_qfprom_mmio1_read: off 20f0 sz 4 val 0
qcom_qfprom_mmio1_read: off 604c sz 4 val 0
qcom_qfprom_mmio1_read: off 6130 sz 4 val 0
qcom_qfprom_mmio1_read: off 6138 sz 4 val 0
qcom_qfprom_mmio1_read: off 6134 sz 4 val 0
qcom_tcsr_devconfig_mmio1_read: off 0 sz 4 val 600d0000
qcom_tcsr_devconfig_mmio1_read: off 0 sz 4 val 600d0000
qcom_qfprom_mmio1_read: off 6138 sz 4 val 0
qcom_qfprom_mmio1_read: off 6138 sz 4 val 0
qcom_qfprom_mmio1_read: off 6130 sz 4 val 0
qcom_qfprom_mmio1_read: off 603c sz 4 val 400
HIT instrument @887a2432c cpu 0 0
HIT instrument @887a240a4 cpu 0 0
HIT instrument @887a2432c cpu 0 0
HIT instrument @887a240a4 cpu 0 0
HIT instrument @887a2432c cpu 0 0
HIT instrument @887a240a4 cpu 0 0
HIT instrument @887a2432c cpu 0 0
HIT instrument @887a240a4 cpu 0 0
HIT instrument @887a2432c cpu 0 0
HIT instrument @887a240a4 cpu 0 0
HIT instrument @887a2432c cpu 0 0
HIT instrument @887a240a4 cpu 0 0
```

