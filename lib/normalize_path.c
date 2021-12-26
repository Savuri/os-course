#include <inc/lib.h>

static void 
NormalizePathOneJump(char cur_dir[MAXPATHLEN], const char *left) {
    if (!strcmp(left, ".")) {
        cur_dir[MAXPATHLEN-1] = '\0';
        return;
    }

    if (!strcmp(left, "..")) {
        if (!strcmp(cur_dir, "/")) {
            cur_dir[1] = '\0';
            return;
        }

        char* new_ptr = strrchr(cur_dir, '/');

        if (new_ptr == cur_dir) {
            // "/one_dir"
            cur_dir[1] = '\0';
            return;
        }

        *new_ptr = '\0';
        return;
    }

    char *ptr = cur_dir + strlen(cur_dir);
    if (strcmp(cur_dir, "/") && ptr < cur_dir + MAXPATHLEN - 1) {
        // it is not root
        *ptr = '/';
        ++ptr;
    }

    strncpy(ptr, left, MAXPATHLEN - strlen(ptr) - 1);
    cur_dir[MAXPATHLEN-1] = '\0';

}

void 
NormalizePath(char cur_dir[MAXPATHLEN], char path[MAXPATHLEN]) {
    if (path[0] == '\0') {
        return;
    }

    cur_dir[MAXPATHLEN-1] = '\0';
    path[MAXPATHLEN-1] = '\0';

    if (path[0] == '/') {
        cur_dir[1] = '\0';
        unsigned len = strlen(path);
        for (int i = 0; i < len; ++i) {
            path[i] = path[i+1];
        }
    }

    if (path[strlen(path)-1] == '/') {
        path[strlen(path)-1] = '\0';
    } else {
        path[MAXPATHLEN-1] = '\0';
    }

    // [left, right)
    char *left = path;
    char *right = strchr(left+1, '/');
    
    while (right != NULL) {
        *right = '\0';
        NormalizePathOneJump(cur_dir, left);
        left = right + 1;
        right = strchr(left+1, '/');
    } 

    NormalizePathOneJump(cur_dir, left);
    cur_dir[MAXPATHLEN-1] = '\0';
}
