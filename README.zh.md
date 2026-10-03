[English](README.md) | **中文**

# apt-compat

一个把 **apt 命令行**映射到本机包管理器的静态二进制。

它让你在非 Debian 系发行版上照常敲 `apt install / apt search / apt update ...`。

- **纯映射层**：能对应上的命令与参数映射成后端写法，对应不上的**原样透传**。
- **不代提权**：绝不自动加 `sudo`；需要 root 时由你自己 `sudo apt ...`。
- **单文件静态二进制**：`ldd` 显示 `not a dynamic executable`，无运行时依赖。

---

## 支持的系统与包管理器

按表中顺序探测；命中第一个存在的即选中。

| 系统 | 包管理器 | 探测命令 |
|---|---|---|
| Fedora, RHEL 8+, CentOS Stream, Rocky, AlmaLinux, Amazon Linux, Oracle Linux | dnf | `dnf` |
| 旧版 RHEL/CentOS 7 | yum | `yum` |
| Arch, Manjaro, EndeavourOS, Garuda, MSYS2 | pacman | `pacman` |
| openSUSE Leap / Tumbleweed, SLES | zypper | `zypper` |
| Gentoo, Funtoo | emerge (Portage) | `emerge` |
| Alpine, postmarketOS | apk | `apk` |
| Void Linux | xbps | `xbps-install` |
| OpenWrt | opkg | `opkg` |
| Solus | eopkg | `eopkg` |
| Mageia, OpenMandriva | urpm (`urpmi`/`urpme`/`urpmq`) | `urpmi` |
| Slackware | slackpkg | `slackpkg` |
| NixOS | nix-env | `nix-env` |
| CRUX | prt-get | `prt-get` |
| Guix System | guix | `guix` |
| FreeBSD, GhostBSD | pkg | `pkg` |
| Haiku | pkgman | `pkgman` |
| NuTyX | cards | `cards` |

> 注意：一个系统若同时装了多个受支持的后端（例如装了 dnf 和 yum），按上表顺序优先取靠前者。

---

## 命令映射

### 安装 / 卸载 / 查询

| 后端 | install | remove | purge | search | show |
|---|---|---|---|---|---|
| dnf | `dnf install` | `dnf remove` | `dnf remove` | `dnf search` | `dnf info` |
| yum | `yum install` | `yum remove` | `yum remove` | `yum search` | `yum info` |
| pacman | `pacman -S` | `pacman -R` | `pacman -Rn` | `pacman -Ss` | `pacman -Si` |
| zypper | `zypper install` | `zypper remove` | `zypper remove` | `zypper search` | `zypper info` |
| emerge | `emerge` | `emerge --unmerge` | `emerge --unmerge` | `emerge --search` | `emerge -pv` |
| apk | `apk add` | `apk del` | `apk del` | `apk search` | `apk info` |
| xbps | `xbps-install` | `xbps-remove` | `xbps-remove` | `xbps-query -Rs` | `xbps-query -RS` |
| opkg | `opkg install` | `opkg remove` | `opkg remove` | `opkg find` | `opkg info` |
| eopkg | `eopkg install` | `eopkg remove` | `eopkg remove` | `eopkg search` | `eopkg info` |
| urpm | `urpmi` | `urpme` | `urpme` | `urpmq -y` | `urpmq -i` |
| slackpkg | `slackpkg install` | `slackpkg remove` | `slackpkg remove` | `slackpkg search` | `slackpkg info` |
| nix-env | `nix-env -i` | `nix-env -e` | `nix-env -e` | `nix-env -qaP` | `nix-env -qa --description` |
| prt-get | `prt-get install` | `prt-get remove` | `prt-get remove` | `prt-get search` | `prt-get info` |
| guix | `guix install` | `guix remove` | `guix remove` | `guix search` | `guix show` |
| pkg | `pkg install` | `pkg remove` | `pkg remove` | `pkg search` | `pkg info` |
| pkgman | `pkgman install` | `pkgman uninstall` | `pkgman uninstall` | `pkgman search` | — |
| cards | `cards install` | `cards remove` | `cards remove` | `cards search` | `cards info` |

### 更新索引 / 升级 / 列表

| 后端 | update | upgrade | full-upgrade | list --installed |
|---|---|---|---|---|
| dnf | `dnf -q makecache` | `dnf upgrade` | `dnf distro-sync` | `dnf -q list installed` |
| yum | `yum -q makecache` | `yum update` | `yum update` | `yum -q list installed` |
| pacman | `pacman -Sy` | `pacman -Su` | `pacman -Syu` | `pacman -Q` |
| zypper | `zypper --quiet refresh` | `zypper update` | `zypper dup` | `zypper --quiet pa -i` |
| emerge | `emerge --sync` | `emerge -u @world` | `emerge -uDN @world` | 扫描 `/var/db/pkg` |
| apk | `apk update` | `apk upgrade` | `apk upgrade` | `apk list --installed` |
| xbps | `xbps-install -S` | `xbps-install -u` | `xbps-install -Su` | `xbps-query -l` |
| opkg | `opkg update` | `opkg upgrade` | `opkg upgrade` | `opkg list-installed` |
| eopkg | `eopkg update-repo` | `eopkg upgrade` | `eopkg upgrade` | `eopkg list-installed` |
| urpm | `urpmi.update -a` | `urpmi --auto-update` | `urpmi --auto-update` | `rpm -q --all` |
| slackpkg | `slackpkg update` | `slackpkg upgrade-all` | `slackpkg upgrade-all` | 扫描 `/var/log/packages` |
| nix-env | `nix-channel --update` | `nix-env -u` | `nix-env -u` | `nix-env -q --installed` |
| prt-get | `ports -u` | `prt-get sysup` | `prt-get sysup` | `prt-get listinst` |
| guix | `guix refresh` | `guix upgrade` | `guix upgrade` | `guix package -I` |
| pkg | `pkg update` | `pkg upgrade` | `pkg upgrade` | `pkg info -a` |
| pkgman | `pkgman refresh` | `pkgman update` | `pkgman update` | `pkgman search -i -a` |
| cards | `cards sync` | `cards upgrade` | `cards upgrade` | `cards list` |

> 完整逐项映射（含 reinstall / download / clean / list --upgradable 等）见 `apt-backends.c` 中的映射表。

---

## 选项映射

| apt 选项 | 含义 | 映射方式 |
|---|---|---|
| `-y`, `--yes`, `--assume-yes` | 全部回答 yes | 追加后端的非交互标志（如 `-y`、`--noconfirm`、`--non-interactive`） |
| `-s`, `--simulate`, `--dry-run`, `--just-print` | 模拟 | 追加后端的模拟标志（如 `--assumeno`、`--print`、`--dry-run`） |
| `-d`, `--download-only` | 只下载 | 走后端 download 子命令（如 `dnf download`、`pacman -Sw`、`xbps-install -D`） |
| `--purge` / `purge` | 连配置一起删 | 用后端 purge 模板（如 `pacman -Rn`），无则退回 remove |
| `--reinstall` / `reinstall` | 重装 | 用后端 reinstall 模板，无则退回 install |
| `--only-upgrade` | 只升级已装的 | 用后端"升级指定包"模板（如 `dnf upgrade <pkg>`） |
| `--installed` | 只列已装 | `list` 走 list_installed |
| `--upgradable` | 只列可升级 | `list` 走 list_upgradable |
| `-o KEY=VALUE` | 配置项 | 消费该参数（跳过） |
| `-q`, `--quiet`, `--no-install-recommends` | — | apt 专属，忽略 |
| 其它未知选项 | — | **原样透传**给后端 |
| `--` 之后的参数 | — | **原样透传**给后端（追加到命令末尾） |

示例：

```sh
apt install vim -- --proxy=localhost:1234   # → apk add vim --proxy=localhost:1234
apt install -y vim=9.0.1                    # → apk add vim=9.0.1  （见下方版本锁定）
```

---

## 云编译（GitHub Actions）

仓库自带工作流 `.github/workflows/build.yml`，任何人 Fork 后都能在 GitHub 上一键云端编译静态二进制，无需本地工具链。

- 触发：推送到 `master`/`main`、Pull Request，或在 Actions 页面手动 `Run workflow`。
- 产物：每个架构（`x86_64`、`aarch64`）产出 `apt-<arch>-musl` 与 `apt-<arch>-glibc`。
- 发布：工作流末尾会把产物发布到 Releases。推送到主分支滚动更新一个 `latest` 预发布（始终附最新二进制）；推送形如 `v1.0.0` 的 tag 则创建正式 Release。
- 校验：工作流会 `file` + `ldd` 确认是静态链接并打印 `--version`。

---

## 注意事项

1. **不提权。**
   程序不会自动 `sudo`。装到 `/usr/bin/apt` 后请用 `sudo apt install ...` 执行需要 root 的操作；直接以普通用户运行会由后端报权限错误——这与直接运行后端命令一致。

2. **版本锁定（`pkg=version`）。**
   dnf/yum/xbps 用 `name-version`；zypper/apk 用 `name=version`；emerge 用 `=name-version`；其余后端（pacman/slackpkg/urpm/nix-env/prt-get/guix/pkg/pkgman/cards/opkg/eopkg）无此写法，会去掉版本号并打印 `W:` 告警，改为安装最新版。

3. **不支持的操作。**
   表中标为不支持（模板为 `{0}`）的操作不会臆造命令，而是打印 `W:`/`E:` 警告或退回等价操作。

4. **特殊"还原"。**
   - pacman 没有单条 autoremove 命令，改为"列出孤儿包再删除"（`pacman -Qdtq` → `pacman -R`）。
   - emerge 的"已装列表"扫描 `/var/db/pkg`；slackpkg 的扫描 `/var/log/packages`。
   - apk 无 autoremove 概念，直接告警不做。

5. **stdout/stderr 与退出码。**
   本工具只负责转发与执行后端命令，其输出与退出码原样传递。

6. **构建与安装。**

   ```sh
   make                    # 推荐
   # 或等价地：
   gcc -O2 -static -s -o apt apt-compat.c apt-backends.c
   # musl 更干净的静态链接：
   musl-gcc -O2 -static -s -o apt apt-compat.c apt-backends.c
   ```

   安装到非 apt 系统：

   ```sh
   [ -e /usr/bin/dpkg ] && { echo "Debian-family detected, abort."; exit 1; }
   sudo install -m 0755 apt /usr/bin/apt
   sudo ln -sf apt /usr/bin/apt-get
   sudo ln -sf apt /usr/bin/apt-cache
   ```

7. **参考。**
   映射规则参考：`icy/pacapt`、`Itai-Nelken/aptpac`、`rami3l/pacaptr`、`sigoden/upt`（逐操作权威表见 `reference/upt/src/vendor.rs`）。
