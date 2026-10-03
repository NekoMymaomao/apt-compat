/* apt-compat.c — 把 apt 命令行映射到本机包管理器并执行。
 *
 * 定位：纯映射层。能对应上的命令/参数映射成后端写法，对应不上的原样透传；
 * 不替用户提权（需要 root 时请自行 sudo apt ...）。
 *
 * 编译（见 Makefile）：
 *   gcc -O2 -static -s -o apt apt-compat.c apt-backends.c
 *   musl-gcc -O2 -static -s -o apt apt-compat.c apt-backends.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include "apt-backends.h"

#define APT_VERSION   "2.4.5"
#define LAYER_VERSION "2.0"

static Backend *g_b = NULL;          /* 选中后端 */
static int g_yes=0, g_sim=0, g_download=0;
static int g_purge=0, g_reinstall=0, g_only_upgrade=0;
static int g_list_installed=0, g_list_upgradable=0;

static void warn(const char *s) { fprintf(stderr, "W: %s\n", s); }
static void err (const char *s) { fprintf(stderr, "E: %s\n", s); }

/* ---------- PATH 查找（不 fork 任何命令） ---------- */
static int has_cmd(const char *name) {
    if (!name || !*name) return 0;
    const char *path = getenv("PATH");
    if (!path || !*path)
        path = "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin";
    size_t nlen = strlen(name);
    const char *p = path;
    for (;;) {
        const char *colon = strchr(p, ':');
        size_t plen = colon ? (size_t)(colon - p) : strlen(p);
        if (plen + 1 + nlen + 1 < 4096) {
            char buf[4096];
            if (plen == 0) snprintf(buf, sizeof buf, "./%s", name);
            else {
                memcpy(buf, p, plen);
                buf[plen] = '/';
                memcpy(buf + plen + 1, name, nlen + 1);
            }
            if (access(buf, X_OK) == 0) {
                struct stat st;
                if (stat(buf, &st) == 0 && S_ISREG(st.st_mode)) return 1;
            }
        }
        if (!colon) break;
        p = colon + 1;
    }
    return 0;
}

/* ---------- 动态参数数组 ---------- */
typedef struct { char **v; size_t n, cap; } Argv;
static void av_init(Argv *a) { a->v=NULL; a->n=0; a->cap=0; }
static void av_push(Argv *a, const char *s) {
    if (a->n + 2 > a->cap) {
        size_t nc = a->cap ? a->cap*2 : 16;
        char **nv = realloc(a->v, nc*sizeof(char*));
        if (!nv) { perror("realloc"); exit(1); }
        a->v = nv; a->cap = nc;
    }
    a->v[a->n++] = (char*)s;
    a->v[a->n]   = NULL;
}
static void av_free(Argv *a) { free(a->v); av_init(a); }

/* ---------- exec（不提权） ---------- */
static int run(char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 127; }
    if (pid == 0) {
        execvp(argv[0], argv);
        fprintf(stderr, "E: exec %s: %s\n", argv[0], strerror(errno));
        _exit(127);
    }
    int st;
    if (waitpid(pid, &st, 0) < 0) return 127;
    if (WIFEXITED(st))   return WEXITSTATUS(st);
    if (WIFSIGNALED(st)) return 128 + WTERMSIG(st);
    return 127;
}

/* ---------- 后端检测（用 install[0] 作为探测命令） ---------- */
static Backend *detect_backend(void) {
    for (int i=0; i<g_backend_count; i++)
        if (has_cmd(g_backends[i].install[0])) return &g_backends[i];
    return NULL;
}

/* ---------- 版本锁定 pkg=ver 翻译 ---------- */
static char *translate_pin(const char *spec) {
    const char *eq = strchr(spec, '=');
    if (!eq) return strdup(spec);
    size_t n = (size_t)(eq - spec);
    const char *v = eq + 1;
    char *out = NULL;
    int r;
    switch (g_b->pin) {
        case PIN_DASH:   r = asprintf(&out, "%.*s-%s", (int)n, spec, v); (void)r; break;
        case PIN_EQ:     r = asprintf(&out, "%.*s=%s", (int)n, spec, v); (void)r; break;
        case PIN_EMERGE: r = asprintf(&out, "=%.*s-%s", (int)n, spec, v); (void)r; break;
        default:
            fprintf(stderr, "W: %s has no version pin; installing latest of %.*s\n",
                    g_b->name, (int)n, spec);
            out = strndup(spec, n);
            break;
    }
    return out ? out : strdup(spec);
}
static char *strip_ver(const char *spec) {
    const char *eq = strchr(spec, '=');
    return eq ? strndup(spec, (size_t)(eq - spec)) : strdup(spec);
}

/* ---------- 参数组装 ---------- */
static Argv g_extra;                 /* `--` 之后 / 未知选项，原样透传给后端 */

static void push_tmpl(Argv *a, const char *const t[]) { for (int i=0; t[i]; i++) av_push(a, t[i]); }
static void push_pkgs(Argv *a, char **p, int n)        { for (int i=0; i<n; i++)   av_push(a, p[i]); }
static void apply_extra(Argv *a)                       { for (size_t i=0; i<g_extra.n; i++) av_push(a, g_extra.v[i]); }

/* 组装并执行：模板 [+ -y 映射的非交互标志] [+ -s 映射的模拟标志] [+ 包名] [+ 透传] */
static int do_simple(const char *const tmpl[], char **pkgs, int n) {
    if (!tmpl || !tmpl[0]) { err("operation not supported by this backend."); return 1; }
    Argv a; av_init(&a);
    push_tmpl(&a, tmpl);
    if (g_yes) push_tmpl(&a, g_b->noninteractive);
    if (g_sim) push_tmpl(&a, g_b->simulate);
    push_pkgs(&a, pkgs, n);
    apply_extra(&a);
    int rc = run(a.v);
    av_free(&a);
    return rc;
}

/* ---------- 特殊 list：扫描目录 ---------- */
static int list_dir(const char *path) {
    DIR *d = opendir(path);
    if (!d) { warn("cannot open directory."); return 1; }
    struct dirent *e;
    while ((e = readdir(d)))
        if (e->d_name[0] != '.') printf("%s\n", e->d_name);
    closedir(d);
    return 0;
}
static int list_emerge_installed(void) {   /* /var/db/pkg/<cat>/<pkg> */
    DIR *d = opendir("/var/db/pkg");
    if (!d) { warn("cannot open /var/db/pkg."); return 1; }
    struct dirent *cat;
    while ((cat = readdir(d))) {
        if (cat->d_name[0] == '.') continue;
        char path[4096];
        snprintf(path, sizeof path, "/var/db/pkg/%s", cat->d_name);
        DIR *cd = opendir(path);
        if (!cd) continue;
        struct dirent *pkg;
        while ((pkg = readdir(cd)))
            if (pkg->d_name[0] != '.') printf("%s/%s\n", cat->d_name, pkg->d_name);
        closedir(cd);
    }
    closedir(d);
    return 0;
}

/* ---------- 命令映射 ---------- */
static int cmd_update(void) {
    return do_simple(g_b->update, NULL, 0);
}

static int cmd_upgrade(int full) {
    return do_simple(full ? g_b->full_upgrade : g_b->upgrade, NULL, 0);
}

static int cmd_install(char **pkgs, int n, int reinstall_mode) {
    if (n == 0) { err("install: no package given"); return 100; }
    char **t = calloc(n, sizeof(char*));
    for (int i=0; i<n; i++) t[i] = translate_pin(pkgs[i]);

    const char *const *tmpl;
    if (g_download)          tmpl = g_b->download;
    else if (reinstall_mode) tmpl = g_b->reinstall[0] ? g_b->reinstall : g_b->install;
    else if (g_only_upgrade) tmpl = g_b->upgrade_pkg;
    else                     tmpl = g_b->install;

    if (!tmpl || !tmpl[0]) { err("install not supported by this backend."); free(t); return 1; }
    int rc = do_simple(tmpl, t, n);
    for (int i=0; i<n; i++) free(t[i]);
    free(t);
    return rc;
}

static int cmd_remove(char **pkgs, int n, int purge_mode) {
    if (n == 0) { err("remove: no package given"); return 100; }
    char **t = calloc(n, sizeof(char*));
    for (int i=0; i<n; i++) t[i] = strip_ver(pkgs[i]);

    const char *const *tmpl = purge_mode && g_b->purge[0] ? g_b->purge : g_b->remove;
    int rc = do_simple(tmpl, t, n);
    for (int i=0; i<n; i++) free(t[i]);
    free(t);
    return rc;
}

static int cmd_autoremove(void) {
    /* pacman 无等价单命令，还原为“列出孤儿包再删除” */
    if (!strcmp(g_b->name, "pacman")) {
        FILE *f = popen("pacman -Qdtq 2>/dev/null", "r");
        Argv a; av_init(&a);
        push_tmpl(&a, g_b->remove);          /* pacman -R */
        if (g_yes) push_tmpl(&a, g_b->noninteractive);
        if (g_sim) push_tmpl(&a, g_b->simulate);
        char buf[256]; int got = 0;
        if (f) {
            while (fgets(buf, sizeof buf, f)) {
                size_t L = strlen(buf);
                while (L && (buf[L-1]=='\n' || buf[L-1]=='\r')) buf[--L]=0;
                if (L) { av_push(&a, strdup(buf)); got = 1; }
            }
            pclose(f);
        }
        if (!got) { av_free(&a); return 0; }
        apply_extra(&a);
        int rc = run(a.v);
        av_free(&a);
        return rc;
    }
    if (!strcmp(g_b->name, "apk")) { warn("apk has no autoremove; nothing to do."); return 0; }
    if (!g_b->autoremove[0]) { warn("autoremove not supported by this backend."); return 0; }
    return do_simple(g_b->autoremove, NULL, 0);
}

static int cmd_search(char **pkgs, int n) {
    if (n == 0) { err("search: no keyword"); return 100; }
    return do_simple(g_b->search, pkgs, n);
}

static int cmd_show(char **pkgs, int n) {
    if (n == 0) { err("show: no package given"); return 100; }
    return do_simple(g_b->show, pkgs, n);
}

static int cmd_list(void) {
    if (g_list_installed) {
        if (!strcmp(g_b->name, "emerge"))   return list_emerge_installed();
        if (!strcmp(g_b->name, "slackpkg")) return list_dir("/var/log/packages");
        return do_simple(g_b->list_installed, NULL, 0);
    }
    if (g_list_upgradable) {
        if (!g_b->list_upgradable[0]) { warn("this backend has no upgradable listing."); return 0; }
        return do_simple(g_b->list_upgradable, NULL, 0);
    }
    if (!g_b->list_all[0]) {
        if (!strcmp(g_b->name, "emerge")) warn("emerge has no 'list all available'; try search.");
        else warn("this backend has no 'list all available'; try search.");
        return 0;
    }
    return do_simple(g_b->list_all, NULL, 0);
}

static int cmd_clean(int autocl) {
    const char *const *tmpl = autocl && g_b->autoclean[0] ? g_b->autoclean : g_b->clean;
    if (!tmpl || !tmpl[0]) {
        if (!strcmp(g_b->name, "emerge")) warn("emerge has no cache clean.");
        else warn("clean not supported by this backend.");
        return 0;
    }
    return do_simple(tmpl, NULL, 0);
}

static void usage(void) {
    fprintf(stderr,
"apt %s (compat-layer %s, backend: %s)\n"
"Usage: apt [options] command\n"
"\n"
"Basic commands:\n"
"  list     - list packages based on package names\n"
"  search   - search in package descriptions\n"
"  show     - show package details\n"
"\n"
"Install, upgrade and remove commands:\n"
"  install      - install packages\n"
"  reinstall    - reinstall packages\n"
"  remove       - remove packages\n"
"  purge        - remove packages and config files\n"
"  autoremove   - automatically remove all unused packages\n"
"  update       - update list of available packages\n"
"  upgrade      - upgrade the system by installing/upgrading packages\n"
"  full-upgrade - upgrade the system, may remove/install packages\n"
"\n"
"Misc commands:\n"
"  clean        - erase downloaded archive files\n"
"  autoclean    - erase old downloaded archive files\n"
"\n"
"Options: -y -q -s -d --purge --reinstall --only-upgrade --download-only\n"
"         --installed --upgradable\n"
"Args after -- are passed through to the underlying package manager.\n"
"This tool only maps commands; it never escalates privileges.\n",
        APT_VERSION, LAYER_VERSION, g_b ? g_b->name : "none");
}

int main(int argc, char **argv) {
    g_b = detect_backend();

    /* --version / --help 在后端探测失败时也能用 */
    if (argc >= 2) {
        const char *c0 = argv[1];
        if (!strcmp(c0,"--version") || !strcmp(c0,"-V") || !strcmp(c0,"-v") || !strcmp(c0,"version")) {
            printf("apt %s (compat-layer %s, backend: %s)\n",
                   APT_VERSION, LAYER_VERSION, g_b ? g_b->name : "none");
            return 0;
        }
        if (!strcmp(c0,"--help") || !strcmp(c0,"-h") || !strcmp(c0,"help")) {
            usage();
            return 0;
        }
    }

    if (!g_b) {
        err("No supported package manager found.");
        return 100;
    }

    av_init(&g_extra);
    Argv pos; av_init(&pos);
    int endopts = 0;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!endopts && !strcmp(a,"--")) { endopts = 1; continue; }
        if (endopts) { av_push(&g_extra, a); continue; }
        if (a[0]=='-' && a[1]) {
            if      (!strcmp(a,"-y")||!strcmp(a,"--yes")||!strcmp(a,"--assume-yes")) g_yes=1;
            else if (!strcmp(a,"-s")||!strcmp(a,"--simulate")||!strcmp(a,"--dry-run")||!strcmp(a,"--just-print")) g_sim=1;
            else if (!strcmp(a,"-d")||!strcmp(a,"--download-only")) g_download=1;
            else if (!strcmp(a,"--purge")) g_purge=1;
            else if (!strcmp(a,"--reinstall")) g_reinstall=1;
            else if (!strcmp(a,"--only-upgrade")) g_only_upgrade=1;
            else if (!strcmp(a,"--installed")) g_list_installed=1;
            else if (!strcmp(a,"--upgradable")) g_list_upgradable=1;
            else if (!strcmp(a,"-o")) { if (i+1<argc) i++; }
            else if (!strcmp(a,"-q")||!strcmp(a,"--quiet")||!strcmp(a,"--no-install-recommends")) { /* apt 专属，忽略 */ }
            else av_push(&g_extra, a);   /* 未知选项：原样透传给后端 */
            continue;
        }
        av_push(&pos, a);
    }

    if (pos.n == 0) { usage(); av_free(&pos); return 0; }

    const char *cmd = pos.v[0];
    char **pkgs = pos.v + 1;
    int npkgs = (int)(pos.n - 1);
    int rc = 0;

    if      (!strcmp(cmd,"update"))       rc = cmd_update();
    else if (!strcmp(cmd,"upgrade"))      rc = cmd_upgrade(0);
    else if (!strcmp(cmd,"full-upgrade")||!strcmp(cmd,"dist-upgrade")) rc = cmd_upgrade(1);
    else if (!strcmp(cmd,"install"))      rc = cmd_install(pkgs, npkgs, g_reinstall);
    else if (!strcmp(cmd,"reinstall"))    rc = cmd_install(pkgs, npkgs, 1);
    else if (!strcmp(cmd,"remove"))       rc = cmd_remove(pkgs, npkgs, g_purge);
    else if (!strcmp(cmd,"purge"))        rc = cmd_remove(pkgs, npkgs, 1);
    else if (!strcmp(cmd,"autoremove"))   rc = cmd_autoremove();
    else if (!strcmp(cmd,"search"))       rc = cmd_search(pkgs, npkgs);
    else if (!strcmp(cmd,"show")||!strcmp(cmd,"info")||!strcmp(cmd,"policy")) rc = cmd_show(pkgs, npkgs);
    else if (!strcmp(cmd,"list"))         rc = cmd_list();
    else if (!strcmp(cmd,"clean"))        rc = cmd_clean(0);
    else if (!strcmp(cmd,"autoclean"))    rc = cmd_clean(1);
    else {
        err("Invalid operation.");
        usage();
        rc = 100;
    }
    av_free(&pos);
    return rc;
}
