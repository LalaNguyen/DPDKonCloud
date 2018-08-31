# DPDKonCloud

1. Suspend the RCU to avoid halt
2. Remember to update the driver to avoid packet loss

----- Workable Driver on both sender and receiver  -----
driver: ixgbe
version: 5.3.7
firmware-version: 0x800004f8
expansion-rom-version:
bus-info: 0000:04:00.0
supports-statistics: yes
supports-test: yes
supports-eeprom-access: yes
supports-register-dump: yes
supports-priv-flags: yes

----- Workable Boot Menu -------
## ## End Default Options ##

title           Ubuntu 12.04.5 LTS, kernel 3.13.0-32-generic
uuid            11f5dcbb-0a51-4ed5-be61-58fae01050bc
kernel          /vmlinuz-3.13.0-32-generic root=UUID=959ec956-ee86-4227-9f91-1372e3d26e8b ro console=tty0 console=ttyS0,115200n8 quiet splash
initrd          /initrd.img-3.13.0-32-generic

title           Ubuntu 12.04.5 LTS, kernel 3.2.0.67-generic
uuid            11f5dcbb-0a51-4ed5-be61-58fae01050bc
kernel          /vmlinuz-3.2.0-67-generic root=UUID=959ec956-ee86-4227-9f91-1372e3d26e8b ro isolcpus=1,2,3,4 maxcpus=1  console=tty0 console=ttyS0,115200n8 quite splash
initrd          /initrd.img-3.2.0-67-generic

title           Ubuntu 12.04.5 LTS, kernel 3.13.0-32-generic (recovery mode)
uuid            11f5dcbb-0a51-4ed5-be61-58fae01050bc
kernel          /vmlinuz-3.13.0-32-generic root=UUID=959ec956-ee86-4227-9f91-1372e3d26e8b ro  single
initrd          /initrd.img-3.13.0-32-generic

title           Ubuntu 12.04.5 LTS, memtest86+
uuid            11f5dcbb-0a51-4ed5-be61-58fae01050bc
kernel          /memtest86+.bin

### END DEBIAN AUTOMAGIC KERNELS LIST
title XMHF
rootnoverify (hd0,1)
kernel /init-x86.bin serial=115200,8n1,0x3f8
module /hypervisor-x86.bin.gz
modulenounzip (hd0)+1
module /Xeon-E7-8800-4800-2800-SINIT-v1.1.bin

