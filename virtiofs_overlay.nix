{
  withCcache, # Enable ccache. Requires correct permissions, see below.
  smp,      # Enable multicore support (SMP)
  vpkgs ? import ./virtiofs_pinned.nix {}
} :
final: prev: {
  virtiofsd = vpkgs.virtiofsd;
  qemu = vpkgs.qemu;
}
