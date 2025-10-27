# Guide on how to use VirtioFS DAX in IncludeOS  

## Intro on VirtioFS DAX (now called VFS_DAX)  

*VFS_DAX* is an experimental feature in the now deprecated *C-VirtioFSD*.  
*VFS_DAX* allowed the guest to access host page cache pages (aka mmap of files).  
The only difference between DAX and regular host mmap is the fact that  
the mmap appears in physical guest memory of the VFS device bar.  
The unikernel can now perform CPU read and store instructions directly  
on host page cache pages without copying!  

## Guide on building QEMU and VirtioFSD

1. Clone the [virtiofs-qemu](https://gitlab.com/virtio-fs/qemu) and checkout commit hash `32006c66f2578af4121d7effaccae4aa4fa12e46`.  
2. Download dependencies: `libseccomp-dev`, `libcapstone-dev`, `libfdt-dev` etc (debian). Resolve as build or configuration fails (repro not my problem).  
3. Goto repository root, create a build directory and type the following:  
  `./configure --python=<path-to-your-python> --disable-werror --target-list="x86_64-softmmu"`.  
4. Stub the setrlimit-thingy and seccomp-thingy for VirtioFSD. Build using the `build-qemu.sh`.  
   script from [virtio-fs-ci](https://gitlab.com/virtio-fs/virtio-fs-ci).
5. Add sysadmin cap by doing `sudo setcap cap_sys_admin+ep virtiofsd`.  
6. Now you have a custom qemu and VirtioFSD that can drive DAX!  

## Guide on using VirtioFS DAX within IncludeOS  

1. You need my (torgeiru) `vmrunner` from branch `virtiofs_dax`.  
2. The following branch you are on right now.  
3. This is a VFS DAX JSON template (binary paths are required):  
```json
{
  "virtiofs_dax" : {
    "shared" : "<path-to-the-folder-you-want-to-share>",
    "cache_size" : 2,
    "qemu_path" : "<path-to-qemu>",
    "virtiofsd_path" : "<path-to-the-virtiofsd>"
  }
}
```
4. Boot your IncludeOS unikernel with the json configuration  
  and the virtiofs driver included in the cmake.  
5. Bon appetite!  

[Other useful resource on VirtioFS DAX](https://virtio-fs.gitlab.io/howto-qemu.html/?utm_source=chatgpt.com)
