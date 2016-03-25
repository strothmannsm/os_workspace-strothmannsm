#include <iostream>

extern "C" {
#include "../src/S16FS.c"
}

int main() {
	// Actual tests coming later (sorry)
    fs_unmount(fs_format("test.s16fs"));

    fs_unmount(fs_mount("test.s16fs"));
}
