#include <os>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <expects>

#include <hal/machine.hpp>
#include <hw/vfs_device.hpp>
#include <fuse/fuse.hpp>

int main() {
  char buffer[600] = {};
  char read_again_buffer[600] = {};

  /* Testing read functionality */
  auto& vfs_device = os::machine().get<hw::VFS_device>(0);
  std::cout << "The device name is " << vfs_device.device_name() << "\n";

  uint64_t fh = vfs_device.open("banana.txt", O_RDONLY, 0);

  ssize_t read_size = vfs_device.read(fh, buffer, 0x1000);
  Expects(read_size == 584);

  vfs_device.close(fh);

  printf("Successfully read entire bananafile!\n");

  /* Testing write functionality */
  uint64_t fh2 = vfs_device.open("banana_copy.txt", O_RDWR, 0);

  ssize_t written_size = vfs_device.write(fh2, buffer, read_size);
  Expects(written_size == read_size);

  vfs_device.lseek(fh2, SEEK_SET, 0);

  ssize_t read_again_size = vfs_device.read(fh2, read_again_buffer, written_size);
  Expects(read_again_size == written_size);

  vfs_device.close(fh2);

  printf("Successfully wrote and read back banana!\n");

  /* Testing SEEK_SET and SEEK_CUR functionality */
  char buffer0[4] = {};
  char buffer1[5] = {};

  uint64_t fh3 = vfs_device.open("seek_file.txt", O_RDONLY, 0);
  
  vfs_device.lseek(fh3, 4, SEEK_SET);
  Expects(vfs_device.read(fh3, buffer0, 4) == 4);
  Expects(memcmp(buffer0, "yyyy", 4) == 0);

  vfs_device.lseek(fh3, 4, SEEK_CUR);
  Expects(vfs_device.read(fh3, buffer1, 4) == 4);
  Expects(memcmp(buffer1, "wwww", 4) == 0);

  vfs_device.close(fh3);

  printf("Seeking was a success!\n");

  /* Attempting to create a new file, write to it and read it back */
  char buffer2[8] = {};

  uint64_t fh4 = vfs_device.open("new_file.txt", O_CREAT | O_RDWR, 0644);
  Expects(vfs_device.write(fh4, (void*)"torgeir", 7) == 7);
  vfs_device.lseek(fh4, 0, SEEK_SET);  
  Expects(vfs_device.read(fh4, buffer2, 8) == 7);
  Expects(memcmp(buffer2, "torgeir", 7) == 0);
  vfs_device.close(fh4);

  printf("Successfully created a new file!\n");

  printf("%s\n", read_again_buffer);

  os::shutdown();
}