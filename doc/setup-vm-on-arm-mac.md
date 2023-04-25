# Setup Podman VM for x86 on ARM Mac

In case you want to use FUSE on ARM-based Macs, you will need to emulate x86_64, since e.g., MOTION requires some special x86 instructions.
As it may happen that some parts of FUSE are then unavailable on ARM machines, here is a short tutorial on how to setup a QEMU x86 VM with podman. 
With this VM, you can run x86 containers on top of that, so you can fully utilize the frameworks used in this project.
Note that this will be a bad choice for benchmarking of any kind, as x86 emulation on an ARM chip is a lot slower than running native code on it. 
Instead, it should be used to familiarize or try out some things even if you don't have an x86 machine available.

### Install Podman
```
brew update && brew upgrade
brew install coreutils podman
```

### Setup VM according to IBM guide
[IBM Developer](https://developer.ibm.com/tutorials/running-x86-64-containers-mac-silicon-m1/) 
* [Download Fedora CoreOS](https://getfedora.org/en/coreos/download?tab=metal_virtualized&stream=stable&arch=x86_64)
* `podman machine init —image-path path/to/fedora-coreos-36.20220703.3.1-qemu.x86_64.qcow2.xz fedora-intel`
* Go to `~/.config/containers/podman/machine/qemu` and open `fedora-intel.json`
	* Change `/opt/homebrew/bin/qemu-system-aarch64` -> `/opt/homebrew/bin/qemu-system-x86_64`
	* change `/opt/homebrew/share/qemu/edk2-aarch64-code.fd,if=pflash,format=raw,readonly=on` -> `file=/opt/homebrew/share/qemu/edk2-x86_64-code.fd,if=pflash,format=raw,readonly=on` 
	* Remove the complete part that says
```
		"-accel",
		"hvf",
		"-accel",
		"tgc",
		"-cpu",
		"host",
		"-M",
		"virt,highmem=on" 
		"-drive",
		"file=/Users//.local/share/containers/podman/machine/qemu/intel_ovmf_vars.fd,if=pflash,format=raw",
```


### Run VM and containers on top of VM!
```
podman system connection default fedora-intel
podman machine start fedora-intel 
```

Starting the VM will take a few minutes because of the emulation of x86_64, which is slower than running ARM natively.

```
podman build -t name:tag -f Dockerfile .
podman run -d -it --cap-add sys_ptrace -p127.0.0.1:2222:22 --name container_name image_name /bin/bash
```

In case you don’t know the name of the image, run

```
podman image list
```


