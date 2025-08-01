#include <fs/filesystem.hpp>
#include <hw/vfs_device.hpp>

/**
 * Virtual filesystem device filesystem (lack of names smh)
 **/
namespace fs {
  class VFSD_filesystem : public File_system {
  public:
    //VFSD_filesystem(hw::VFS_device& d) : _vfsdev(d) {}
    /* Filesystem information */
    int device_id() const noexcept override { return -1; }
    std::string name() const override { return "VFSD_filesystem"; }

    /* Sync filesystem operations */
    List ls(const std::string& path) const override { return List{{error_t::E_INVAL, "Not implemented"}, nullptr }; }
    List ls(const Dirent&) const override { return List{{error_t::E_INVAL, "Not implemented"}, nullptr }; }
    Buffer read(const Dirent&, uint64_t pos, uint64_t n) const override { return Buffer {{error_t::E_INVAL, "Not implemented"}, nullptr }; }
    Dirent stat(Path, const Dirent* const = nullptr) const { return Dirent { nullptr }; }

    /* Async callback filesystem operations. Unimplemented for first VFS iteration */
    void ls(const std::string& path, on_ls_func) const override {}
    void ls(const Dirent& entry,     on_ls_func) const override {}
    void read(const Dirent&, uint64_t pos, uint64_t n, on_read_func) const override {}
    void stat(Path_ptr, on_stat_func fn, const Dirent* const = nullptr) const override {}
    void cstat(const std::string& pathstr, on_stat_func) override {}

    /* Unused functions not applicable to a virtual filesystem */
    uint64_t block_size() const override { return -1; }
    void init(uint64_t lba, uint64_t size, on_init_func on_init) override {}
  private:
    VFS_device& _vfsdev;
  };
}