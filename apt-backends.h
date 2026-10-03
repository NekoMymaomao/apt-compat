#ifndef APT_BACKENDS_H
#define APT_BACKENDS_H

/* 版本锁定写法 */
enum {
    PIN_DASH,   /* name-version   (dnf/yum/xbps) */
    PIN_EQ,     /* name=version   (zypper/apk)   */
    PIN_EMERGE, /* =name-version  (emerge)       */
    PIN_NONE    /* 不支持：去版本并告警 (pacman 等) */
};

/* 每个操作是一个以 NULL 结尾的 argv 前缀模板。
 * 带包名的操作（install/remove/.../show）会在此模板后追加包名。
 * 取值为 {0}（首元素为空）表示该后端不支持此操作。 */
typedef struct {
    const char *name;               /* 探测/执行用的可执行名 */
    int pin;                        /* PIN_* */
    const char *update[8];
    const char *upgrade[8];         /* 升级所有已装包 */
    const char *upgrade_pkg[8];     /* 仅升级指定包（--only-upgrade） */
    const char *full_upgrade[8];    /* 可能卸载/安装的整机升级 */
    const char *autoremove[8];
    const char *install[8];
    const char *reinstall[8];       /* {0} → 退回 install */
    const char *remove[8];
    const char *purge[8];           /* {0} → 退回 remove */
    const char *search[8];
    const char *show[8];
    const char *download[8];
    const char *list_all[8];
    const char *list_installed[8];  /* {0} → 走特殊实现 */
    const char *list_upgradable[8];
    const char *clean[8];
    const char *autoclean[8];
    const char *noninteractive[8];  /* 非交互标志，追加在模板后 */
    const char *simulate[8];        /* 模拟标志，追加在模板后 */
} Backend;

extern Backend g_backends[];
extern int g_backend_count;

#endif
