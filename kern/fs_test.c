#include "fs.h"
#include "console.h"
#include "file.h"
#include "string.h"
#include <elf.h>
#define TEST_FUNC(name) \
  do { \
    if (name() == 0) { \
      cprintf(#name" pass!\n"); \
    } else { \
      cprintf(#name" fail!\n"); \
    } \
  } while (0)

#define INIT_FILE_NUM 5
static const char init_files[INIT_FILE_NUM][7] = { "/init", "/ls", "/mkfs", "/sh", "cat" };

#define TEST_WRITE_NUM 2
static const char write_files[TEST_WRITE_NUM][20] = { "/hello.cpp", "/readme.md" };
static const char write_text[TEST_WRITE_NUM][100] = {
    "#include <cstdio> \n int main() { \n\tprintf(\"Hello world!\\n\");\n\treturn 0;\n}\n",
    "This is a readme file\n"
};
int test_initial_scan()
{
    for (int i = 0; i < INIT_FILE_NUM; i++) {
        if (namei(init_files[i]) == 0) {
            return -1;
        } 
    }
    for (int i = 0; i < TEST_WRITE_NUM; i++) {
        if (namei(write_files[i]) != 0) {
            return -1;
        } 
    }
    return 0;
}

int test_initial_read() // read the elf magic number
{
    // read elf
    Elf64_Ehdr elf_header;
    for (int i = 0; i < INIT_FILE_NUM; i++) {
        struct file* f = filealloc();
        f->readable = 1;
        f->type = FD_INODE;
        f->ip = namei(init_files[i]);
        if (f->ip == 0) {
            return -1;
        } 
        f->off = 0;
        f->ref = 1;
        if (fileread(f, (char*)&elf_header, sizeof(elf_header)) != sizeof(elf_header)) {
            return -1;
        }
        fileclose(f);
        if (strncmp((const char*)elf_header.e_ident, ELFMAG, 4)) {
            return -1;
        } 
    }
    return 0;
}

int test_file_write()
{
    char buf[100];
    for (int i = 0; i < TEST_WRITE_NUM; i++) {
        struct file* f = filealloc();
        f->readable = 1;
        f->type = FD_INODE;
        f->writable = 1;
        f->ip = create(write_files[i], T_FILE, 0, 0);
        f->ref = 1;
        if (f->ip == 0) {
            return -1;
        }
        iunlock(f->ip);
        f->off = 0;
        if (filewrite(f, write_text[i], strlen(write_text[i])) != strlen(write_text[i])) {
            return -1;
        }
        memset(buf, 0, sizeof(buf));
        fileread(f, buf, strlen(write_text[i]));
        if (strcmp(buf, write_text[i])) {
            return -1;
        }

        // delete the file
        // f->ip->nlink--;
        fileclose(f);
        // dirunlink(namei("/"), write_files[i] + 1, f->ip->inum);
    }
    return 0;
}

void
test_file_system()
{
    TEST_FUNC(test_initial_scan);
    TEST_FUNC(test_initial_read);
    TEST_FUNC(test_file_write);
    // TEST_FUNC(test_initial_scan);
    do {} while (0);
}