cmake_minimum_required(VERSION 3.7)
project(fs)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        cache.c
        system/open_close.c
        dev.c
        filp.c
        fs.h
        fs_main.c
        inode.c
        makefs.c
        path.c
        system/read_write.c
        fsutil/srec_import.c
        fsutil/sys_stdio.c
        fsutil/cmake_util.c make_disk.c fsutil/fsutil.h fsutil/cmake_bitmap.c system/pipe.c fsutil/tty.c system/stat.c system/lseek.c system/chdir_mkdir.c util.c system/chown_chmod.c system/dup.c system/link_unlink.c system/umask_access.c fsutil/dir_util.c system/getdent.c fsutil/srec_import.h unit_test.c system/sync.c system/mknod.c rootfs.c)

include_directories(../include/fs_include)
include_directories(../include)
add_compile_definitions(FSUTIL)
add_compile_definitions(_DEBUG)

add_executable(fs ${SOURCE_FILES})